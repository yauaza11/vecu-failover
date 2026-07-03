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

/* ==========================================================================
 * [이중화 상태 장부]
 * ========================================================================== */
typedef enum {
    ECU_STATE_STANDBY = 0, // 평소에는 숨죽여 대기하는 모드
    ECU_STATE_ACTIVE  = 1  // 메인 폭사 시 제어권을 가로채는 기동 모드
} EcuState_Type;

static EcuState_Type eCurrentState = ECU_STATE_STANDBY;
static uint32_t u32HeartbeatTimeoutCounter = 0; // 하트비트 누락 카운터 (스톱워치)
static uint8_t u8EngineSeq = 0;

/* Node A는 500ms(=50cycle)마다 하트비트를 쏜다. 3회 연속 누락(1.5s)까지는
 * 정상 지터로 보고 봐주고, 그 이상이면 진짜 장애로 판단해 페일오버한다. */
#define HEARTBEAT_TIMEOUT_CYCLES 150

/* ==========================================================================
 * 🧱 백업 제어기 메인 10ms 스케줄러 Task (제어권 강탈 킬러 메커니즘)
 * ========================================================================== */
void vBsw_BackupEngineControlTask(void *pvParameters) {
    Rte_Init_ASW(); // 격리된 쌍둥이 ASW 알고리즘 부팅

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));

        /* 📥 [Can_MainFunction_Read 방식] 페달 신호(0x200) + Node A 하트비트(0x100) 폴링 수신 */
        bool bHeartbeatReceivedThisCycle = false;
        CanFrame_Type sRxFrame;
        while (Can_Read(&sRxFrame)) {
            if (sRxFrame.u32Id == 0x200) {
                if (sRxFrame.au8Data[3] == u8CalculateAUTOSARCRC8(sRxFrame.au8Data, 3)) {
                    Rte_Write_RpPedalSensor_Val(sRxFrame.au8Data[1]); // 검증 성공 시 RTE 주입
                } else {
                    Rte_Write_RpPedalSensor_Val(0);                  // 에러 시 안전 수비
                }
            } else if (sRxFrame.u32Id == 0x100) {
                // 메인 형님이 살아있음을 확인했으므로, 폭사 감시용 스톱워치 카운터를 0으로 초기화!
                u32HeartbeatTimeoutCounter = 0;
                bHeartbeatReceivedThisCycle = true;
            }
        }

        /* ------------------------------------------------------------------
         * [이중화 상태 스캔] 평소 대기 모드(STANDBY)일 때의 감시 매니지먼트
         * ------------------------------------------------------------------ */
        if (eCurrentState == ECU_STATE_STANDBY) {
            // 10ms 마다 스톱워치 카운터를 1씩 올립니다.
            u32HeartbeatTimeoutCounter++;

            // ⚠️ [타임아웃 감지] 하트비트 3회 연속(1.5s) 누락되면 진짜 장애로 판단
            if (u32HeartbeatTimeoutCounter >= HEARTBEAT_TIMEOUT_CYCLES) {
                // 🚨 "메인 형님 뒈졌다!! 내가 제어권 뺏는다!!" Active 상태 강제 스위칭
                eCurrentState = ECU_STATE_ACTIVE;
                printf("[ALERT] Node A Failure Detected! Node B Switched to ACTIVE!\n");
            }
        } else {
            // ✅ [제어권 반납] Active 상태에서도 Node A 하트비트가 되살아나면 즉시 대기 모드로 복귀
            if (bHeartbeatReceivedThisCycle) {
                eCurrentState = ECU_STATE_STANDBY;
                printf("[RECOVER] Node A heartbeat resumed. Node B yields control back to STANDBY.\n");
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
