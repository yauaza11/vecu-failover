/* ==========================================================================
 * [Src/main_node_b.c] - 쇳덩어리 레지스터 직접 타격형 Standby(백업) ECU BSW
 * ========================================================================== */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f412cx.h"
#include "protocol.h"
#include "Rte_mock.h"

/* ==========================================================================
 * [🚨 원칙 3: 내가 찐으로 짜야 하는 이중화 상태 장부]
 * ========================================================================== */
typedef enum {
    ECU_STATE_STANDBY = 0, // 평소에는 숨죽여 대기하는 모드
    ECU_STATE_ACTIVE  = 1  // 메인 폭사 시 제어권을 가로챈 기동 모드
} EcuState_Type;

static EcuState_Type eCurrentState = ECU_STATE_STANDBY;
static uint32_t u32HeartbeatTimeoutCounter = 0; // 하트비트 누락 카운터 (스톱워치)
static uint8_t u8EngineSeq = 0;

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
 * 🧱 1. [🚨 원칙 3] CAN 수신 복합 인터럽트 (페달 수신 + Node A 하트비트 감시)
 *  - 이 함수가 바로 Node B 가상 안테나의 핵심 센서입니다.
 * ========================================================================== */
void CAN1_RX0_IRQHandler(void) {
    if (CAN1->RF0R & CAN_RF0R_FMP0) {
        uint32_t u32RxHeaderTIR = CAN1->sFIFOMailBox[0].RIR;
        uint32_t u32RxDataLow   = CAN1->sFIFOMailBox[0].RDLR;
        uint32_t u32RxId = (u32RxHeaderTIR >> 21) & 0x7FF; // 인입된 CAN ID 추출

        uint8_t rxData[8] = {0,};
        rxData[0] = (uint8_t)((u32RxDataLow >> 0)  & 0xFF);
        rxData[1] = (uint8_t)((u32RxDataLow >> 8)  & 0xFF); 
        rxData[2] = (uint8_t)((u32RxDataLow >> 16) & 0xFF);
        rxData[3] = (uint8_t)((u32RxDataLow >> 24) & 0xFF);

        /* ------------------------------------------------------------------
         * [시나리오 A] 외부 환경(Node C)이 쏜 페달 신호인 경우 (ID: 0x200 가정)
         * ------------------------------------------------------------------ */
        if (u32RxId == 0x200) {
            if (rxData[3] == u8CalculateAUTOSARCRC8(rxData, 3)) {
                Rte_Write_RpPedalSensor_Val(rxData[1]); // 검증 성공 시 RTE 주입
            } else {
                Rte_Write_RpPedalSensor_Val(0);         // 에러 시 안전 수비
            }
        }
        /* ------------------------------------------------------------------
         * [시나리오 B] 🌟 메인 노드 A가 생존 증명으로 쏜 하트비트인 경우 (ID: 0x100)
         * ------------------------------------------------------------------ */
        else if (u32RxId == 0x100) {
            // 메인 형님이 살아있음을 확인했으므로, 폭사 감시용 스톱워치 카운터를 0으로 초기화!
            u32HeartbeatTimeoutCounter = 0;
        }
        
        CAN1->RF0R |= CAN_RF0R_RFOM0; // FIFO0 버퍼 비우기 (레지스터 타격)
    }
}

/* ==========================================================================
 * 🧱 2. [🚨 원칙 3] 백업 제어기 메인 10ms 스케줄러 Task (제어권 강탈 킬러 메커니즘)
 * ========================================================================== */
void vBsw_BackupEngineControlTask(void *pvParameters) {
    Rte_Init_ASW(); // 격리된 쌍둥이 ASW 알고리즘 부팅
    
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));

        /* ------------------------------------------------------------------
         * [이중화 상태 스캔] 평소 대기 모드(STANDBY)일 때의 감시 매니지먼트
         * ------------------------------------------------------------------ */
        if (eCurrentState == ECU_STATE_STANDBY) {
            // 10ms 마다 스톱워치 카운터를 1씩 올립니다.
            u32HeartbeatTimeoutCounter++;

            // ⚠️ [타임아웃 감지] 카운터가 5가 되었다? (10ms * 5 = 50ms 동안 하트비트가 실종됨)
            if (u32HeartbeatTimeoutCounter >= 5) {
                // 🚨 "메인 형님 뒈졌다!! 내가 제어권 뺏는다!!" Active 상태 강제 스위칭
                eCurrentState = ECU_STATE_ACTIVE;
                printf("[🚨 ALERT] Node A Failure Detected! Node B Switched to ACTIVE!\n");
            }
        }

        /* ------------------------------------------------------------------
         * [제어권 행사 분기] Active 상태로 격상되면 비로소 대타 연산 및 버스 출력 개통
         * ------------------------------------------------------------------ */
        if (eCurrentState == ECU_STATE_ACTIVE) {
            
            // 🧠 [ASW 수식 구동] 꽁꽁 잠겨있던 백업컴의 시뮬링크 두뇌를 가동시킵니다!
            Rte_Call_ASW_Step(); 

            // 📤 계산 결과 정정당당하게 uint32_t 풀 체급으로 수거 완료
            uint32_t u32BackupRpm = Rte_Read_PpEngineSpeed_Val();
            u8EngineSeq = (u8EngineSeq + 1) % 16;

            // [대타 RPM 발송 패킹]
            uint8_t txData[8] = {0,};
            txData[0] = (0x03 << 4) | u8EngineSeq; 
            txData[1] = (uint8_t)((u32BackupRpm >> 8) & 0xFF);
            txData[2] = (uint8_t)(u32BackupRpm & 0xFF);
            txData[3] = u8CalculateAUTOSARCRC8(txData, 3); 

            // 📡 메인 노드가 쏘던 ID: 0x301 자리를 기가 막히게 인터셉트해서 쏴줍니다!
            vBsw_Register_CAN_Write(0x301, txData, 4); 
        }

        // 🔒 [하드웨어 와치독 킥] 대기 상태든 구동 상태든 루프 생존 증명 밥 주기
        IWDG->KR = 0xAAAA; 
    }
}

/* ==========================================================================
 * 📦 [원칙 1: 복붙] 주변장치 초기화 및 메인 엔트리 포인트
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
    printf("[Node B] QEMU Pure Register AUTOSAR Standby ECU Online.\n");

    // FreeRTOS 커널 가동 및 백업 태스크 안착
    xTaskCreate(vBsw_BackupEngineControlTask, "Backup_10msTask", 256, NULL, 3, NULL);
    vTaskStartScheduler();
    
    while(1);
    return 0;
}