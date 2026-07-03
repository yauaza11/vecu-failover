/* ==========================================================================
 * [Core/Src/can_driver_stm32.c] - Can Driver(MCAL) STM32 bxCAN 레지스터 구현체
 *  - 타겟/QEMU 빌드 전용. can_driver.h 인터페이스를 CAN1 레지스터로 구현한다.
 * ========================================================================== */
#include "can_driver.h"
#include "stm32f412cx.h"

#define CAN_RX_QUEUE_LEN 8

static volatile CanFrame_Type asRxQueue[CAN_RX_QUEUE_LEN];
static volatile uint8_t u8RxHead = 0;
static volatile uint8_t u8RxTail = 0;

/* ==========================================================================
 * 🧱 CAN1 수신 인터럽트 - 하드웨어 FIFO0에서 소프트웨어 큐로 적재만 하고 탈출
 * ========================================================================== */
void CAN1_RX0_IRQHandler(void) {
    if (CAN1->RF0R & CAN_RF0R_FMP0) {
        uint32_t u32RxHeaderTIR = CAN1->sFIFOMailBox[0].RIR;
        uint32_t u32RxDataLow   = CAN1->sFIFOMailBox[0].RDLR;
        uint32_t u32RxDataHigh  = CAN1->sFIFOMailBox[0].RDHR;
        uint8_t  u8Dlc          = (uint8_t)(CAN1->sFIFOMailBox[0].RDTR & 0x0F);

        uint8_t u8Next = (uint8_t)((u8RxHead + 1) % CAN_RX_QUEUE_LEN);
        if (u8Next != u8RxTail) { /* 큐가 꽉 찼으면 프레임 드롭 (오래된 값 보존) */
            asRxQueue[u8RxHead].u32Id = (u32RxHeaderTIR >> 21) & 0x7FF;
            asRxQueue[u8RxHead].u8Dlc = u8Dlc;
            asRxQueue[u8RxHead].au8Data[0] = (uint8_t)((u32RxDataLow  >> 0)  & 0xFF);
            asRxQueue[u8RxHead].au8Data[1] = (uint8_t)((u32RxDataLow  >> 8)  & 0xFF);
            asRxQueue[u8RxHead].au8Data[2] = (uint8_t)((u32RxDataLow  >> 16) & 0xFF);
            asRxQueue[u8RxHead].au8Data[3] = (uint8_t)((u32RxDataLow  >> 24) & 0xFF);
            asRxQueue[u8RxHead].au8Data[4] = (uint8_t)((u32RxDataHigh >> 0)  & 0xFF);
            asRxQueue[u8RxHead].au8Data[5] = (uint8_t)((u32RxDataHigh >> 8)  & 0xFF);
            asRxQueue[u8RxHead].au8Data[6] = (uint8_t)((u32RxDataHigh >> 16) & 0xFF);
            asRxQueue[u8RxHead].au8Data[7] = (uint8_t)((u32RxDataHigh >> 24) & 0xFF);
            u8RxHead = u8Next;
        }

        CAN1->RF0R |= CAN_RF0R_RFOM0;
    }
}

void Can_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    CAN1->MCR &= ~CAN_MCR_SLEEP;

    CAN1->IER |= CAN_IER_FMPIE0;
    NVIC_EnableIRQ(CAN1_RX0_IRQn);
}

void Can_Write(uint32_t u32Id, const uint8_t *pData, uint8_t u8Dlc) {
    CAN1->sTxMailBox[0].TIR &= ~CAN_TI0R_TXRQ;
    CAN1->sTxMailBox[0].TIR = (u32Id << 21);
    CAN1->sTxMailBox[0].TDTR = u8Dlc;

    CAN1->sTxMailBox[0].TDLR = ((uint32_t)pData[0] << 0)  |
                               ((uint32_t)pData[1] << 8)  |
                               ((uint32_t)pData[2] << 16) |
                               ((uint32_t)pData[3] << 24);

    CAN1->sTxMailBox[0].TDHR = ((uint32_t)pData[4] << 0)  |
                               ((uint32_t)pData[5] << 8)  |
                               ((uint32_t)pData[6] << 16) |
                               ((uint32_t)pData[7] << 24);

    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
}

bool Can_Read(CanFrame_Type *pFrame) {
    if (u8RxTail == u8RxHead) {
        return false;
    }
    *pFrame = asRxQueue[u8RxTail];
    u8RxTail = (uint8_t)((u8RxTail + 1) % CAN_RX_QUEUE_LEN);
    return true;
}
