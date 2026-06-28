#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "stm32f412cx.h"

// 💡 Node A -> Node B (가속 페달 패킷 구조)
typedef struct __attribute__((packed)){
    uint8_t sequence_counter;
    uint8_t main_aps;
    uint8_t sub_aps;
    uint8_t checksum;
} CanPedalPacket;

// 💡 Node B -> Node C (엔진 RPM 및 상태 패킷 구조)
typedef struct __attribute__((packed)){
    uint8_t status_and_counter; // [ 4비트 데이터ID | 4비트 시퀀스 ]
    uint8_t rpm_high;           // RPM 상위 8비트
    uint8_t rpm_low;            // RPM 하위 8비트
    uint8_t checksum;           // AUTOSAR CRC-8
} CanEnginePacket;

/* ========================================================================
 * 🔑 형님의 완벽무결한 진짜 전장 ID 명세서 규칙
 * ======================================================================== */
#define CAN_ID_PEDAL_DATA  0x100  // Node A가 쏘는 페달 데이터 ID
#define CAN_ID_ENGINE_DATA 0x200  // Node B가 쏘는 엔진 데이터 ID
#define CAN_ID_FAULT_ALERT 0x7F0  // Node B가 고장 확정 시 Node C로 쏘는 비상 신호 ID

#define PEDAL_DATA_ID      0x5    // 패킷 내부 데이터 검증용 매직 ID (Node A)
#define CLUSTER_DATA_ID    0x7    // 패킷 내부 데이터 검증용 매직 ID (Node B)
/* ======================================================================== */

// 💡 [가상 CAN 버스 공유 서랍 명세] 3개의 노드가 동시에 쳐다볼 단 하나의 버스 공간
typedef struct {
    CAN_TypeDef NodeA_CAN; // 페달 전용 서랍
    CAN_TypeDef NodeB_CAN; // 엔진 전용 서랍
    CAN_TypeDef NodeC_CAN; // 계기판 전용 서랍
} CAN_Bus_TypeDef;

// 외부 프로세스 연동용 가상 레지스터 포인터 전역 변수
extern CAN_TypeDef *pVirtualCAN1;

// 💡 공유 메모리 버스 개설 및 연동 함수 명찰
void vInitVirtualCANBus(uint8_t u8NodeId);
uint8_t u8CalculateAUTOSARCRC8(const uint8_t *pu8Data, uint8_t u8Length);

// ========================================================================
// 🔥 [형님 코드 수호 치트키 최종형] 
// 원래 짜놓으신 CAN1->... 코드를 100% 수호하면서 공유 메모리 주소로 강제 워프!
// ========================================================================
#ifdef CAN1
  #undef CAN1
#endif
#define CAN1 pVirtualCAN1

#endif /* PROTOCOL_H */