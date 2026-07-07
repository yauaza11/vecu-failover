/* ==========================================================================
 * [Src/main_node_a.c] - Active ECU BSW (Can Driver는 can_driver.h 뒤로 위임)
 * ========================================================================== */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "can_driver.h"
#include "protocol.h"
#include "Rte_mock.h"     // 오직 RTE 인터페이스만 바라봄

#ifdef TARGET_MCU
#include "stm32f412cx.h"  // 하드웨어 와치독(IWDG)은 타겟 빌드에만 존재
#endif

static uint8_t u8EngineSeq = 0;
static uint8_t u8HeartBeat = 0;
/* TODO: 하트비트 전용 카운터를 여기에 선언하세요.
 * 힌트: u8EngineSeq는 4bit 프로토콜 시퀀스 필드라 0~15만 돔 - 500ms(=50cycle)를
 *       세는 용도로 못 씁니다. 별도 카운터가 필요해요.
 * 막히면: git show main:Core/Src/main_node_a.c
 */

/* ==========================================================================
 * 🧱 Node A의 메인 10ms 스케줄러 & 하트비트 토스 Task
 * ========================================================================== */
void vBsw_ActiveEngineControlTask(void *pvParameters) {

    Rte_Init_ASW();
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for(;;){
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));

        /* 📥 [Can_MainFunction_Read 방식] Node C가 쏜 페달 신호 폴링 수신 */
        CanFrame_Type sRxFrame;
        while (Can_Read(&sRxFrame)) {
            if (sRxFrame.u32Id == 0x200) {
                if (sRxFrame.au8Data[3] == u8CalculateAUTOSARCRC8(sRxFrame.au8Data, 3)) {
                    Rte_Write_RpPedalSensor_Val(sRxFrame.au8Data[1]); // 검증 성공 시 RTE 주입
                } else {
                    Rte_Write_RpPedalSensor_Val(0);                  // 에러 시 안전 디폴트 수비
                }
            }
        }

        Rte_Call_ASW_Step();

        uint32_t u32ActiveRpm = Rte_Read_PpEngineSpeed_Val();
        u8EngineSeq = (u8EngineSeq + 1) % 16;

        uint8_t txData[8] = {0,};
        txData[0] = (0x03 << 4) | u8EngineSeq;
        txData[1] = (uint8_t)((u32ActiveRpm >> 8) & 0xFF);
        txData[2] = (uint8_t)(u32ActiveRpm & 0xFF);
        txData[3] = u8CalculateAUTOSARCRC8(txData, 3);

        Can_Write(0x301, txData, 4);

        /* TODO: 여기에 하트비트 전송 로직을 짜세요.
         * - 500ms(=50 사이클)마다 한 번 Can_Write(0x100, ...)로 하트비트 전송
         * - u8EngineSeq랑 절대 같은 변수를 재사용하지 마세요 (그게 원래 버그였음)
         */

        uint8_t hbData[8]={0,};
        hbData[0] = u8EngineSeq;

        u8HeartBeat = (u8HeartBeat+1) % 50;

        if(u8HeartBeat==0){
            Can_Write(0x100, hbData, 1);
        }

#ifdef TARGET_MCU
        // 🔒 [하드웨어 와치독 킥] 멍멍이 밥 주기
        IWDG->KR = 0xAAAA;
#endif
    }
}

/* ==========================================================================
 * 📦 주변장치 초기화 및 메인 진입점
 * ========================================================================== */
void vBsw_Hardware_Peripheral_Init(void) {
    Can_Init();

#ifdef TARGET_MCU
    // 하드웨어 독립 와치독(IWDG) 레지스터 다이렉트 셋업 (100ms 타임아웃)
    IWDG->KR = 0x5555;   // 레지스터 접근 권한 해제
    IWDG->PR = 0x04;     // 분주비 64 세팅
    IWDG->RLR = 625;     // 릴로드 값 주입
    IWDG->KR = 0xCCCC;   // 와치독 구동 스타트!
#endif
}

int main(void) {
    vBsw_Hardware_Peripheral_Init();
    printf("[Node A] AUTOSAR Active ECU Online.\n");

    // FreeRTOS 오퍼레이팅 커널 위로 10ms 주기 태스크 생성 후 안착
    xTaskCreate(vBsw_ActiveEngineControlTask, "Active_10msTask", 256, NULL, 3, NULL);
    vTaskStartScheduler();

    while(1);
    return 0;
}