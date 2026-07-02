/* ==========================================================================
 * [Src/main_node_a.c] - 쇳덩어리 레지스터 직접 타격형 Active ECU BSW
 * ========================================================================== */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f412cx.h"  // HAL 없이 칩셋 레지스터 직접 까기
#include "protocol.h"
#include "Rte_mock.h"     // 오직 RTE 인터페이스만 바라봄

static uint8_t u8EngineSeq = 0;

/* ==========================================================================
 * 🧱 1. [📦 생각 없이 복붙] STM32 CAN1 레지스터 다이렉트 송신 드라이버
 * ========================================================================== */
static void vBsw_Register_CAN_Write(uint32_t can_id, uint8_t *pData, uint8_t dlc) {
    // 메일박스 0번 강제 타격
    CAN1->sTxMailBox[0].TIR &= ~CAN_TI0R_TXRQ; 
    CAN1->sTxMailBox[0].TIR = (can_id << 21);  // 표준 ID 세팅
    CAN1->sTxMailBox[0].TDTR = dlc;            // 데이터 길이 세팅
    
    // 8바이트 로우/하이 레지스터 패킹 박음질
    CAN1->sTxMailBox[0].TDLR = ((uint32_t)pData[0] << 0)  |
                               ((uint32_t)pData[1] << 8)  |
                               ((uint32_t)pData[2] << 16) |
                               ((uint32_t)pData[3] << 24);
                               
    CAN1->sTxMailBox[0].TDHR = ((uint32_t)pData[4] << 0)  |
                               ((uint32_t)pData[5] << 8)  |
                               ((uint32_t)pData[6] << 16) |
                               ((uint32_t)pData[7] << 24);
                               
    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ; // QEMU 가상 버스로 전송 요청 슛!
}

/* ==========================================================================
 * 🧱 2. [🚨 내가 직접 짠 콘텐츠] Node A의 메인 10ms 스케줄러 & 하트비트 토스 Task
 * ========================================================================== */
void vBsw_ActiveEngineControlTask(void *pvParameters) {

    Rte_Init_ASW(); 
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    for(;;){
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
        Rte_Call_ASW_Step();

        // 🚨 형님 여기 uint8_t에서 uint32_t로 전면 수정했습니다. 255 오버플로우 원천 차단!
        uint32_t u32ActiveRpm = Rte_Read_PpEngineSpeed_Val(); 
        u8EngineSeq = (u8EngineSeq + 1) % 16;

        uint8_t txData[8] = {0,};
        txData[0] = (0x03 << 4) | u8EngineSeq; 
        txData[1] = (uint8_t)((u32ActiveRpm >> 8) & 0xFF); // 이제 2000 RPM 완전체 분할 패킹 가능
        txData[2] = (uint8_t)(u32ActiveRpm & 0xFF);
        txData[3] = u8CalculateAUTOSARCRC8(txData, 3); 

        vBsw_Register_CAN_Write(0x301, txData, 4); // 📡 데이터 쏠 때는 레지스터만 때리고 탈출! (인터럽트 X)

        // 🚑 [이중화 하트비트] 500ms 마다 생존 보고 신호 슛
        if (u8EngineSeq % 50 == 0) {
            uint8_t hbData[8] = {0,};
            hbData[0] = u8EngineSeq;
            vBsw_Register_CAN_Write(0x100, hbData, 1); 
        }

        // 🔒 [하드웨어 와치독 킥] 멍멍이 밥 주기
        IWDG->KR = 0xAAAA;
    }
}

/* ==========================================================================
 * 🧱 3. [🚨 내가 직접 짠 콘텐츠] Node C가 쏜 페달 신호 가로채는 RX 인터럽트 BSW
 * ========================================================================== */
void CAN1_RX0_IRQHandler(void) {
    // QEMU 가상 CAN FIFO0 레지스터가 차 있으면 패킷 인계
    if (CAN1->RF0R & CAN_RF0R_FMP0) {
        uint32_t u32RxDataLow = CAN1->sFIFOMailBox[0].RDLR;
        
        uint8_t rxData[8] = {0,};
        rxData[0] = (uint8_t)((u32RxDataLow >> 0)  & 0xFF);
        rxData[1] = (uint8_t)((u32RxDataLow >> 8)  & 0xFF); // 페달 물리 데이터
        rxData[2] = (uint8_t)((u32RxDataLow >> 16) & 0xFF);
        rxData[3] = (uint8_t)((u32RxDataLow >> 24) & 0xFF); // CRC 위치

        // 무결성 검증 
        if (rxData[3] == u8CalculateAUTOSARCRC8(rxData, 3)) {
            Rte_Write_RpPedalSensor_Val(rxData[1]); // 검증 성공 시 RTE 전선으로 토스!
        } else {
            Rte_Write_RpPedalSensor_Val(0);         // 에러 시 급발진 방지 안전 디폴트 수비
        }
        
        CAN1->RF0R |= CAN_RF0R_RFOM0; // FIFO0 버퍼 비우기 (레지스터 타격)
    }
}

/* ==========================================================================
 * 📦 [📦 생각 없이 복붙] 순수 STM32 레지스터 초기화 및 메인 진입점
 * ========================================================================== */
void vBsw_Hardware_Peripheral_Init(void) {
    // 가상 CAN 주변장치 클럭 공급 및 슬립 해제
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    CAN1->MCR &= ~CAN_MCR_SLEEP; 
    
    // CAN1 RX 인터럽트 레지스터 활성화
    CAN1->IER |= CAN_IER_FMPIE0;
    NVIC_EnableIRQ(CAN1_RX0_IRQn);

    // 하드웨어 독립 와치독(IWDG) 레지스터 다이렉트 셋업 (100ms 타임아웃)
    IWDG->KR = 0x5555;   // 레지스터 접근 권한 해제
    IWDG->PR = 0x04;     // 분주비 64 세팅
    IWDG->RLR = 625;     // 릴로드 값 주입
    IWDG->KR = 0xCCCC;   // 와치독 구동 스타트!
}

int main(void) {
    vBsw_Hardware_Peripheral_Init();
    printf("[Node A] QEMU Pure Register AUTOSAR Active ECU Online.\n");

    // FreeRTOS 오퍼레이팅 커널 위로 10ms 주기 태스크 생성 후 안착
    xTaskCreate(vBsw_ActiveEngineControlTask, "Active_10msTask", 256, NULL, 3, NULL);
    vTaskStartScheduler();
    
    while(1);
    return 0;
}