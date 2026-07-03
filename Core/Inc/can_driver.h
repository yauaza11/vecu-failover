/* ==========================================================================
 * [Core/Inc/can_driver.h] - Can Driver(MCAL) 공용 인터페이스 명세서
 *  - BSW는 이 헤더만 보고 호출한다. 구현체가 STM32 레지스터인지
 *    리눅스 SocketCAN인지는 빌드 타깃(can_driver_stm32.c / can_driver_socketcan.c)
 *    선택으로만 결정된다.
 * ========================================================================== */
#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t u32Id;
    uint8_t  u8Dlc;
    uint8_t  au8Data[8];
} CanFrame_Type;

/**
 * @brief  Can Driver 및 하위 버스 초기화 (클럭/레지스터 또는 소켓 오픈)
 */
void Can_Init(void);

/**
 * @brief  CAN 프레임 1개 즉시 송신
 */
void Can_Write(uint32_t u32Id, const uint8_t *pData, uint8_t u8Dlc);

/**
 * @brief  수신 큐에서 프레임 1개 폴링 (AUTOSAR Can_MainFunction_Read 방식)
 * @return 수신한 프레임이 있으면 true, 큐가 비었으면 false
 */
bool Can_Read(CanFrame_Type *pFrame);

#endif /* CAN_DRIVER_H */
