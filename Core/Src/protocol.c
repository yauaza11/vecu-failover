/* ==========================================================================
 * [Core/Src/protocol.c] - AUTOSAR CRC-8 데이터 무결성 검증 하부 엔진 구현부
 * ========================================================================== */
#include "protocol.h"

/**
 * @brief  AUTOSAR 표준 사양(다항식 0x1D) 비트 시프트 방식 연산 엔진
 *  - 가상 CAN 버스를 타고 흐르는 패킷의 데이터 오염 및 단선 결함을 원천 감시합니다.
 */
uint8_t u8CalculateAUTOSARCRC8(const uint8_t *pData, uint16_t length) {
    // AUTOSAR 사양: 초기값(Initial Value)은 0x00으로 시작
    uint8_t crc = 0x00; 
    uint16_t i;
    uint8_t j;

    for (i = 0; i < length; i++) {
        // 입력 데이터 바이트와 현재 CRC 값을 XOR 믹싱
        crc ^= pData[i];

        // 8비트를 한 비트씩 전진시키며 다항식(0x1D) 주입 및 비트 타격
        for (j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0) {
                // 최상위 비트(MSB)가 1이면 다항식 0x1D와 XOR 연산 후 좌시프트
                crc = (uint8_t)((crc << 1) ^ 0x1D);
            } else {
                // 최상위 비트가 0이면 그냥 좌측으로 1비트 시프트
                crc = (uint8_t)(crc << 1);
            }
        }
    }

    // AUTOSAR 사양: 최종 결과에 대한 XOR 처리 값 역시 0x00 (그대로 반환)
    return crc;
}