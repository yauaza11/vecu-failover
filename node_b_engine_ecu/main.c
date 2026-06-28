#include "FreeRTOS.h"
#include "task.h"
// #include "stm32f412cx.h"
#include "../common/protocol.h" 

void vHardwareInit(void);
// 💡 [정밀 타겟팅] 엔진 전용 태스크 함수 명찰 선언
void vVehicleEngineTask(void *pvParameters); 

int main()
{
    vInitVirtualCANBus(2); // 🔌 가상 전선 링크 (Node B: 엔진 배정)
    vHardwareInit();

    // 💡 [링킹 억까 패치 완료!] 
    // 기존에 엉뚱한 가상 함수 이름(vVehicleClusterTask)으로 적혀있던 부분을 
    // 진짜 알맹이인 'vVehicleEngineTask'로 정확하게 교체했습니다.
    xTaskCreate(vVehicleEngineTask, "EngineTask", 256, NULL, 3, NULL);
    vTaskStartScheduler();

    while(1) {
        // safe state
    }
    return 0;
}

void vHardwareInit(void) {
}