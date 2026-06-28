#include <stdio.h>
#include "FreeRTOS.h"
// #include "stm32f412cx.h"
#include "task.h"
#include "../common/protocol.h"

void vVehicleEngineTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10); // 10ms 주기 연산

    setvbuf(stdout, NULL, _IONBF, 0);
    
    // 🏆 [기동 메시지]
    printf("🔥 [NODE B - ENGINE] SYSTEM READY: Shared Memory Link Established.\n");

    while(1) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // 🔌 공유 메모리 버스에서 Node A(페달)의 사랍장을 훔쳐봅니다.
        // (실제 완벽한 IPC 연동을 위해 Node A의 TxMailBox 데이터를 엔진의 가상 FIFO로 매핑)
        uint32_t u32RxData = pVirtualCAN1[0].sTxMailBox[0].TDLR; // 포인터 추적

        uint8_t u8RxBuffer[3];
        u8RxBuffer[0] = (uint8_t)((u32RxData >> 0)  & 0xFF);
        u8RxBuffer[1] = (uint8_t)((u32RxData >> 8)  & 0xFF);
        u8RxBuffer[2] = (uint8_t)((u32RxData >> 16) & 0xFF);
        uint8_t u8RxCRC = (uint8_t)((u32RxData >> 24) & 0xFF);

        if(u8RxCRC == u8CalculateAUTOSARCRC8(u8RxBuffer, 3)) {
            uint8_t u8PedalVal = u8RxBuffer[1];
            uint32_t u32Rpm = u8PedalVal * 60; // 페달 % 기반 RPM 변환 알고리즘

            static uint8_t u8EngineSeq = 0;
            u8EngineSeq = (u8EngineSeq + 1) % 16;

            uint8_t u8TxBuffer[3];
            u8TxBuffer[0] = (CLUSTER_DATA_ID << 4) | u8EngineSeq;
            u8TxBuffer[1] = (uint8_t)((u32Rpm >> 8) & 0xFF);
            u8TxBuffer[2] = (uint8_t)(u32Rpm & 0xFF);

            uint8_t u8TxCRC = u8CalculateAUTOSARCRC8(u8TxBuffer, 3);

            // 계기판(Node C)이 읽어갈 수 있도록 내 서랍장에 갱신
            CAN1->sTxMailBox[0].TDLR =  (u8TxBuffer[0] << 0)  |
                                        (u8TxBuffer[1] << 8)  |
                                        (u8TxBuffer[2] << 16) |
                                        (u8TxCRC       << 24);
            CAN1->sTxMailBox[0].TIR = (CAN_ID_ENGINE_DATA << 21);
            CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
        }
    }
}