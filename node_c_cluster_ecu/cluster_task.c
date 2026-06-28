#include "../common/protocol.h"
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "FreeRTOS.h"
#include "task.h"


void vVehicleClusterRxTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20);

    uint8_t u8RxBuffer[3];
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("📺 [NODE C - CLUSTER] SYSTEM READY: Virtual Display Active. Waiting for CAN Stream...\n");

    while(1){
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        // 💡 [포인터 억까 패치] 전체 공유 버스 원본 주소를 명확하게 오픈
        int fd = shm_open("/vCAN_Bus", O_RDWR, 0666);
        CAN_Bus_TypeDef *pBus = (CAN_Bus_TypeDef *)mmap(0, sizeof(CAN_Bus_TypeDef), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        // 🔌 Node B(엔진 제어기)의 서랍장을 명확하게 다이렉트로 조준 사격해서 긁어옵니다!
        uint32_t u32TdlrVal = pBus->NodeB_CAN.sTxMailBox[0].TDLR; 

        u8RxBuffer[0] = (uint8_t)((u32TdlrVal >> 0)  & 0xFF); 
        u8RxBuffer[1] = (uint8_t)((u32TdlrVal >> 8)  & 0xFF); 
        u8RxBuffer[2] = (uint8_t)((u32TdlrVal >> 16) & 0xFF); 
        uint8_t u8RxCRC       = (uint8_t)((u32TdlrVal >> 24) & 0xFF); 

        uint8_t u8CalcCRC = u8CalculateAUTOSARCRC8(u8RxBuffer, 3);

        if(u8RxCRC == u8CalcCRC){
            uint8_t u8RxID = (u8RxBuffer[0] >> 4) & 0x0F;

            if (u8RxID == CLUSTER_DATA_ID){
                uint8_t u8RpmHigh = u8RxBuffer[1];
                uint8_t u8RpmLow  = u8RxBuffer[2];

                uint32_t u32Rpm = 0;
                u32Rpm = (uint32_t)(u8RpmHigh << 8) | (u8RpmLow);
                
                printf("🚗 [CLUSTER] Real-time Engine Speed: %d RPM\n", u32Rpm);
                fflush(stdout);
            }
        }
    }
}