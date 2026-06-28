#include "FreeRTOS.h"
#include "task.h"
// #include "stm32f412cx.h"

// 왜 여기서 호출하지? 그냥 실행시키는 건가?
extern void vVehiclePedalTask(void *pvParameters);

void vHardwareInit(void);

int main()
{
    vInitVirtualCANBus(1);
    vHardwareInit();

    xTaskCreate(vVehiclePedalTask, 
                "PedalTask", 
                256,// 스택 사이즈 256 Byte?
                NULL, 
                3,  // Priority
                NULL
    );

    vTaskStartScheduler();

    while(1){
        //safe State: 메모리/스케줄러 터지면 진입하는 락 루프
    }
    return 0;
}

void vHardwareInit(void){
    // 실무에서는 채워야함
}