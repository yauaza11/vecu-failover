#ifndef RTE_MOCK_H
#define RTE_MOCK_H

#include <stdint.h>

/* BSW가 호출할 가상 RTE API 명세서 */
void Rte_Write_RpPedalSensor_Val(uint8_t u8PedalVal);
uint32_t Rte_Read_PpEngineSpeed_Val(void);
void Rte_Init_ASW(void);
void Rte_Call_ASW_Step(void);
void SystemInit(void);
#endif /* RTE_MOCK_H */