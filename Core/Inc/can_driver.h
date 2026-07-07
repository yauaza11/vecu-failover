/* ==========================================================================
 * [Core/Inc/can_driver.h] - Can Driver(MCAL) 공용 인터페이스 명세서
 *  - TODO: 여기에 BSW가 호출할 함수들을 선언하세요.
 *    (Can_Init, Can_Write, Can_Read 등 — 우리가 얘기했던 세 가지 계약)
 *  - 막히면: git show main:Core/Inc/can_driver.h
 * ========================================================================== */
#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/* TODO: CanFrame_Type 구조체 정의 */
typedef struct {
    uint32_t u32Id;       // CAN ID
    uint8_t  u8Dlc;       // 데이터 길이 (0~8)
    uint8_t  au8Data[8];  // 실제 데이터
} CanFrame_Type;

/* TODO: Can_Init / Can_Write / Can_Read 선언 */
void Can_Init(void);
void Can_Write(uint32_t u32Id, const uint8_t *pData, uint8_t u8Dlc);
bool Can_Read(CanFrame_Type *pFrame);

#endif /* CAN_DRIVER_H */
