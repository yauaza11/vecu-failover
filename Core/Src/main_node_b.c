/* ==========================================================================
 * [Src/main_node_b.c] - Standby(백업) ECU BSW (Can Driver는 can_driver.h 뒤로 위임)
 * ========================================================================== */
#include <stdbool.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "can_driver.h"
#include "protocol.h"
#include "Rte_mock.h"

#ifdef TARGET_MCU
#include "stm32f412cx.h"
#endif

typedef enum {
    ECU_STATE_STANDBY = 0,   // 평소 대기 상태
    ECU_STATE_ACTIVE  = 1    // 대신 제어권 잡은 상태
} EcuState_Type;

static EcuState_Type eCurrentState = ECU_STATE_STANDBY;   // 현재 상태
static uint32_t u32HeartbeatTimeoutCounter = 0;            // 하트비트 누락 카운터

#define HEARTBEAT_TIMEOUT_CYCLES 150   // 500ms 하트비트를 3회 연속(1.5초) 놓치면 페일오버

static uint8_t u8EngineSeq = 0;

/* ==========================================================================
 * 🧱 백업 제어기 메인 10ms 스케줄러 Task (제어권 강탈 킬러 메커니즘)
 * ========================================================================== */
void vBsw_BackupEngineControlTask(void *pvParameters) {
    Rte_Init_ASW(); // 격리된 쌍둥이 ASW 알고리즘 부팅

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));

        /* 📥 [Can_MainFunction_Read 방식] 페달 신호(0x200) + Node A 하트비트(0x100) 폴링 수신 */
        CanFrame_Type sRxFrame;
        while (Can_Read(&sRxFrame)) {
            if (sRxFrame.u32Id == 0x200) {
                if (sRxFrame.au8Data[3] == u8CalculateAUTOSARCRC8(sRxFrame.au8Data, 3)) {
                    Rte_Write_RpPedalSensor_Val(sRxFrame.au8Data[1]); // 검증 성공 시 RTE 주입
                } else {
                    Rte_Write_RpPedalSensor_Val(0);                  // 에러 시 안전 수비
                }
            } else if (sRxFrame.u32Id == 0x100) {
                /* TODO: 하트비트 수신 처리
                 * - 누락 카운터를 0으로 리셋
                 * - "이번 사이클에 하트비트를 받았다"는 걸 기록해둘 방법이 필요함
                 *   (ACTIVE 상태에서 STANDBY로 복귀할지 판단할 때 씀)
                 */
                u32HeartbeatTimeoutCounter = 0;
                if(eCurrentState == ECU_STATE_ACTIVE ){
                    eCurrentState = ECU_STATE_STANDBY;
                    printf("[RECOVER] Node A heartbeat resumed. Node B yields control back to STANDBY.\n");
                }
            }
        }

        /* TODO: [이중화 상태 스캔]
         * - STANDBY일 때: 매 사이클 누락 카운터++, 임계값 넘으면 ACTIVE로 전환
         *   (+ printf("[ALERT] ...") 로 로그 남기기)
         * - ACTIVE일 때: 이번 사이클에 하트비트를 받았다면 STANDBY로 복귀
         *   (+ printf("[RECOVER] ...") 로 로그 남기기)
         *   *** 이 복구 로직이 원래 코드에 없어서 한번 ACTIVE로 가면 영원히 안 돌아왔던 버그였어요 ***
         */
        if(eCurrentState == ECU_STATE_STANDBY){
            u32HeartbeatTimeoutCounter++;
            if(u32HeartbeatTimeoutCounter >= HEARTBEAT_TIMEOUT_CYCLES){
                eCurrentState = ECU_STATE_ACTIVE;
                printf("[ALERT] Node A Failure Detected! Node B Switched to ACTIVE!\n");
            }
        }

        /* ------------------------------------------------------------------
         * [제어권 행사 분기] Active 상태로 격상되면 비로소 대타 연산 및 버스 출력 개통
         * ------------------------------------------------------------------ */
        /* TODO: 여기 아래 블록을 "현재 ACTIVE 상태일 때만" 실행되도록 감싸세요 */
        
        if(eCurrentState == ECU_STATE_ACTIVE){
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
            Can_Write(0x301, txData, 4);
        }

#ifdef TARGET_MCU
        // 🔒 [하드웨어 와치독 킥] 대기 상태든 구동 상태든 루프 생존 증명 밥 주기
        IWDG->KR = 0xAAAA;
#endif
    }
}

/* ==========================================================================
 * 📦 주변장치 초기화 및 메인 엔트리 포인트
 * ========================================================================== */
void vBsw_Hardware_Peripheral_Init(void) {
    Can_Init();

#ifdef TARGET_MCU
    IWDG->KR = 0x5555;
    IWDG->PR = 0x04;
    IWDG->RLR = 625;
    IWDG->KR = 0xCCCC;
#endif
}

int main(void) {
    vBsw_Hardware_Peripheral_Init();
    printf("[Node B] AUTOSAR Standby ECU Online.\n");

    // FreeRTOS 커널 가동 및 백업 태스크 안착
    xTaskCreate(vBsw_BackupEngineControlTask, "Backup_10msTask", 256, NULL, 3, NULL);
    vTaskStartScheduler();

    while(1);
    return 0;
}
