#include "Rte_mock.h"
#include "hil.h"
#include "hil_types.h"

extern ExtU_hil_T hil_U; 
extern ExtY_hil_T hil_Y;

void Rte_Write_RpPedalSensor_Val(uint8_t u8PedalVal) {
    hil_U.RpPedalSensor_Val = (real_T)u8PedalVal; // ASW 입력단 주입
}

uint32_t Rte_Read_PpEngineSpeed_Val(void) {
    return (uint32_t)hil_Y.PpEngineSpeed_Val;    // ASW 출력단 탈취
}

void Rte_Init_ASW(void) {
    hil_initialize();
}

void Rte_Call_ASW_Step(void) {
    hil_step();
}

void SystemInit(void){
    
}