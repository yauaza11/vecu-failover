/* ==========================================================================
 * [Core/Inc/protocol.h] - 공용 통신 프로토콜 및 AUTOSAR CRC8 명세서
 * ========================================================================== */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/**
 * @brief  AUTOSAR 사양(SAE J1850)에 맞춘 CRC-8 통신 무결성 연산 함수
 * @param  pData: CRC 연산을 수행할 데이터 버퍼의 포인터
 * @param  length: 연산할 데이터의 바이트 길이 (CRC 필드 제외 크기)
 * @return 연산 완료된 1바이트 크기의 CRC 결과 값
 * @note   Polynomial: 0x1D, Initial Value: 0x00, Final XOR: 0x00
 */
uint8_t u8CalculateAUTOSARCRC8(const uint8_t *pData, uint16_t length);

#endif /* PROTOCOL_H */