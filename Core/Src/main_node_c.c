/* ==========================================================================
 * [Src/main_node_c.c] - 쇳덩어리 레지스터 직접 타격형 가상 차량 환경 모사(HIL) 노드
 * ========================================================================== */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f412cx.h"
#include "protocol.h"

static uint8_t u8VirtualPedalSeq = 0;
static uint32_t u32CapturedEngineRpm = 0;
static uint8_t u8CapturedNodeSeq = 0;

/* ==========================================================================
 * 📦 [원칙 1: 복붙] STM32 CAN1 레지스터 다이렉트 송신 드라이버
 * ========================================================================== */
static void vBsw_Register_CAN_Write(uint32_t can_id, uint8_t *pData, uint8_t dlc) {
    CAN1->sTxMailBox[0].TIR &= ~CAN_TI0R_TXRQ; 
    CAN1->sTxMailBox[0].TIR = (can_id << 21);  
    CAN1->sTxMailBox[0].TDTR = dlc;            
    
    CAN1->sTxMailBox[0].TDLR = ((uint32_t)pData[0] << 0)  |
                               ((uint32_t)pData[1] << 8)  |
                               ((uint32_t)pData[2] << 16) |
                               ((uint32_t)pData[3] << 24);
                               
    CAN1->sTxMailBox[0].TDHR = ((uint32_t)pData[4] << 0)  |
                               ((uint32_t)pData[5] << 8)  |
                               ((uint32_t)pData[6] << 16) |
                               ((uint32_t)pData[7] << 24);
                               
    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ; 
}

/* ==========================================================================
 * 🧱 1. [🚨 원칙 3] CAN 수신 인터럽트 (제어기들이 계산해 쏜 RPM 결과 가로채기)
 *  - 이 인터럽트가 돌 때가 진짜 계기판 바늘이 튕기는 타이밍입니다.
 * ========================================================================== */
void CAN1_RX0_IRQHandler(void) {
    if (CAN1->RF0R & CAN_RF0R_FMP0) {
        uint32_t u32RxHeaderTIR = CAN1->sFIFOMailBox[0].RIR;
        uint32_t u32RxDataLow   = CAN1->sFIFOMailBox[0].RDLR;
        uint32_t u32RxId = (u32RxHeaderTIR >> 21) & 0x7FF;

        uint8_t rxData[8] = {0,};
        rxData[0] = (uint8_t)((u32RxDataLow >> 0)  & 0xFF);
        rxData[1] = (uint8_t)((u32RxDataLow >> 8)  & 0xFF); 
        rxData[2] = (uint8_t)((u32RxDataLow >> 16) & 0xFF);
        rxData[3] = (uint8_t)((u32RxDataLow >> 24) & 0xFF);

        /* ------------------------------------------------------------------
         * [계기판 모사] Node A 또는 Node B가 연산해서 버스에 올린 찐 RPM 패킷 가로채기
         * ------------------------------------------------------------------ */
        if (u32RxId == 0x301) {
            // 프로토콜 무결성 CRC8 장부 검증 완료 시에만 계기판 데이터로 수용
            if (rxData[3] == u8CalculateAUTOSARCRC8(rxData, 3)) {
                // 상위 8비트와 하위 8비트 조립하여 찐 RPM 갱신 복원 (오버플로우 방지 uint32_t 조립)
                u32CapturedEngineRpm = ((uint32_t)rxData[1] << 8) | (uint32_t)rxData[2];
                u8CapturedNodeSeq = rxData[0] & 0x0F; // 제어기가 실어 보낸 시퀀스 번호 탈취
            }
        }
        
        CAN1->RF0R |= CAN_RF0R_RFOM0; // FIFO0 레지스터 타격 후 비우기
    }
}

/* ==========================================================================
 * 🧱 2. [🚨 원칙 3] 가상 주행 센서 공급 및 실시간 계기판 로깅 태스크
 * ========================================================================== */
void vBsw_EnvironmentSimulatorTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t u8FakePedalValue = 60; // 가상으로 가속 페달을 60% 지속 투약한다고 가정

    for (;;) {
        // ⏱️ 센서 신호 주기는 제어 주기(10ms)보다 약간 여유 있게 20ms 스케줄링링
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(20));

        u8VirtualPedalSeq = (u8VirtualPedalSeq + 1) % 16;

        /* ------------------------------------------------------------------
         * 🚗 [가상 차량 센서 슛] 제어기들(Node A, B) 밥 먹으라고 페달 데이터 발송
         * ------------------------------------------------------------------ */
        uint8_t txData[8] = {0,};
        txData[0] = 0x02;               // PEDAL_SENSOR_DATA_ID (0x02)
        txData[1] = u8FakePedalValue;   // 60% 페달 데이터 주입
        txData[2] = u8VirtualPedalSeq;  // 시퀀스 롤링 카운트
        txData[3] = u8CalculateAUTOSARCRC8(txData, 3); // 무결성 보증 수표 박음질

        // 📡 가상 CAN 버스로 페달 신호 브로드캐스팅 (ID: 0x200)
        vBsw_Register_CAN_Write(0x200, txData, 4);

        /* ------------------------------------------------------------------
         * 📺 [HIL 최종 모니터링 출력] 실시간 가상 계기판 UI 화면 렌더링
         * ------------------------------------------------------------------ */
        // QEMU 창 갱신을 위해 이전 글자 지우고 고정 로깅
        printf("\r[vHIL CLUSTER UI] TX Pedal: %d%% | 📈 RX Engine Speed: %4d RPM (Seq: %2d)", 
               u8FakePedalValue, u32CapturedEngineRpm, u8CapturedNodeSeq);
        fflush(stdout);

        // 🔒 [와치독 킥] 환경 노드도 정상 주기 완료 시 하드웨어 멍멍이 밥 주기
        IWDG->KR = 0xAAAA;
    }
}

/* ==========================================================================
 * 📦 [원칙 1: 복붙] 레지스터 초기화 및 메인 엔트리
 * ========================================================================== */
void vBsw_Hardware_Peripheral_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    CAN1->MCR &= ~CAN_MCR_SLEEP; 
    
    CAN1->IER |= CAN_IER_FMPIE0;
    NVIC_EnableIRQ(CAN1_RX0_IRQn);

    IWDG->KR = 0x5555;   
    IWDG->PR = 0x04;     
    IWDG->RLR = 625;     
    IWDG->KR = 0xCCCC;   
}

int main(void) {
    vBsw_Hardware_Peripheral_Init();
    printf("[Node C] QEMU Pure Register HIL Environment Simulation Node Stand Up.\n");

    // FreeRTOS 커널 위로 가상 차량 환경 타스크 기동 안착
    xTaskCreate(vBsw_EnvironmentSimulatorTask, "EnvSim_20msTask", 256, NULL, 3, NULL);
    vTaskStartScheduler();
    
    while(1);
    return 0;
}