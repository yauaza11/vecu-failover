#include "FreeRTOS.h"
#include "task.h"

extern void vVehicleClusterRxTask(void *pvParameters);
void vHardwareInit(void);
int main()
{
    vInitVirtualCANBus(3);
    vHardwareInit();

    xTaskCreate(&vVehicleClusterRxTask, "ClusterRxTask", 256, NULL, 3, NULL);

    vTaskStartScheduler();

    while(1){
        //safe state
    }
    return 0;
}

void vHardwareInit(){

}