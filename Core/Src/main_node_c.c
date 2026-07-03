/* ==========================================================================
 * [Src/main_node_c.c] - 가상 차량 환경 모사(HIL) 노드 BSW (Can Driver는 can_driver.h 뒤로 위임)
 * ========================================================================== */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "can_driver.h"
#include "protocol.h"

#ifdef TARGET_MCU
#include "stm32f412cx.h"
#endif

static uint8_t u8VirtualPedalSeq = 0;
static uint32_t u32CapturedEngineRpm = 0;
static uint8_t u8CapturedNodeSeq = 0;

/* ==========================================================================
 * 🧱 가상 주행 센서 공급 및 실시간 계기판 로깅 태스크
 * ========================================================================== */
void vBsw_EnvironmentSimulatorTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t u8FakePedalValue = 60; // 가상으로 가속 페달을 60% 지속 투약한다고 가정

    for (;;) {
        // ⏱️ 센서 신호 주기는 제어 주기(10ms)보다 약간 여유 있게 20ms 스케줄링
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(20));

        /* 📥 [Can_MainFunction_Read 방식] Node A/B가 계산해 쏜 RPM 결과 폴링 수신 */
        CanFrame_Type sRxFrame;
        while (Can_Read(&sRxFrame)) {
            if (sRxFrame.u32Id == 0x301) {
                // 프로토콜 무결성 CRC8 장부 검증 완료 시에만 계기판 데이터로 수용
                if (sRxFrame.au8Data[3] == u8CalculateAUTOSARCRC8(sRxFrame.au8Data, 3)) {
                    u32CapturedEngineRpm = ((uint32_t)sRxFrame.au8Data[1] << 8) | (uint32_t)sRxFrame.au8Data[2];
                    u8CapturedNodeSeq = sRxFrame.au8Data[0] & 0x0F;
                }
            }
        }

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
        Can_Write(0x200, txData, 4);

        /* ------------------------------------------------------------------
         * 📺 [HIL 최종 모니터링 출력] 실시간 가상 계기판 UI 화면 렌더링
         * ------------------------------------------------------------------ */
        printf("\r[vHIL CLUSTER UI] TX Pedal: %d%% | RX Engine Speed: %u RPM (Seq: %2d)",
               u8FakePedalValue, u32CapturedEngineRpm, u8CapturedNodeSeq);
        fflush(stdout);

#ifdef TARGET_MCU
        // 🔒 [와치독 킥] 환경 노드도 정상 주기 완료 시 하드웨어 멍멍이 밥 주기
        IWDG->KR = 0xAAAA;
#endif
    }
}

/* ==========================================================================
 * 📦 주변장치 초기화 및 메인 엔트리
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
    printf("[Node C] HIL Environment Simulation Node Stand Up.\n");

    // FreeRTOS 커널 위로 가상 차량 환경 태스크 기동 안착
    xTaskCreate(vBsw_EnvironmentSimulatorTask, "EnvSim_20msTask", 256, NULL, 3, NULL);
    vTaskStartScheduler();

    while(1);
    return 0;
}
