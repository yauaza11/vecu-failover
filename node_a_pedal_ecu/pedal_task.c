#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "../common/protocol.h"
// #include "stm32f412cx.h"


void vVehiclePedalTask(void *pvParameters){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);

    uint8_t u8DataBuffer[3];
    setvbuf(stdout, NULL, _IONBF, 0);

    // 🏆 [기동 메시지]
    printf("🟢 [NODE A - PEDAL] SYSTEM READY: Connected to Virtual CAN Bus.\n");

    while(1){
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        static uint8_t u8SeqCounter = 0;
        static uint8_t u8VirtualPedal = 0;
        
        u8VirtualPedal = (u8VirtualPedal + 1) % 101;
        u8SeqCounter = (u8SeqCounter + 1) % 16;

        u8DataBuffer[0] = (PEDAL_DATA_ID << 4) | u8SeqCounter;
        u8DataBuffer[1] = u8VirtualPedal; // MainAps
        u8DataBuffer[2] = u8VirtualPedal; // SubAps

        // 💡 [오타 완벽 패치!] uDataBuffer에서 'u8DataBuffer'로 8자를 정확히 채워 넣었습니다.
        uint8_t CRC8Result = u8CalculateAUTOSARCRC8(u8DataBuffer, 3);

        // 🔌 공유 메모리를 통해 데이터 패킹 송신
        CAN1->sTxMailBox[0].TDLR =  (u8DataBuffer[0] << 0)  |
                                    (u8DataBuffer[1] << 8)  |
                                    (u8DataBuffer[2] << 16) |
                                    (CRC8Result      << 24);
        CAN1->sTxMailBox[0].TDHR = 0x00000000;
        CAN1->sTxMailBox[0].TDTR = (4 << 0);
        CAN1->sTxMailBox[0].TIR = (CAN_ID_PEDAL_DATA << 21); 
        CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
    }
}