#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "FreeRTOS.h"
#include "stm32f412cx.h"
#include "protocol.h"

// 가상 CAN 포인터 실체 변수
CAN_TypeDef *pVirtualCAN1 = NULL;

// 💡 리눅스 커널에게 공유 메모리 서랍장을 파달라고 떼쓰고 내 포인터와 퓨전시키는 야전 함수
void vInitVirtualCANBus(uint8_t u8NodeId)
{
    // 1. 리눅스 커널 시스템에 "/vCAN_Bus" 라는 이름의 공용 파일(공유 메모리) 서랍을 개설하거나 연다!
    int fd = shm_open("/vCAN_Bus", O_CREAT | O_RDWR, 0666);
    
    // 2. 이 서랍장의 크기를 가상 CAN 버스 구조체 크기만큼 강제로 고정 확장한다.
    ftruncate(fd, sizeof(CAN_Bus_TypeDef));
    
    // 3. 내 프로세스 가상 메모리 주소 영역과 리눅스 커널 공용 RAM 영역을 동기화 퓨전(Mapping)시킨다!
    CAN_Bus_TypeDef *pBus = (CAN_Bus_TypeDef *)mmap(0, sizeof(CAN_Bus_TypeDef), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    // 4. 내 노드 ID에 맞는 서랍 배정 (Node A=1, Node B=2, Node C=3)
    // 형님이 짜신 기존의 소스코드(CAN1->...)를 그대로 살리기 위한 영리한 스위칭입니다.
    if(u8NodeId == 1)      pVirtualCAN1 = &(pBus->NodeA_CAN);
    else if(u8NodeId == 2) pVirtualCAN1 = &(pBus->NodeB_CAN);
    else                   pVirtualCAN1 = &(pBus->NodeC_CAN);
}

/* AUTOSAR CRC-8 알고리즘 (기존 버전 그대로 유지) */
uint8_t u8CalculateAUTOSARCRC8(const uint8_t *pu8Data, uint8_t u8Length)
{
    uint8_t u8Crc = 0xFF;
    for (uint8_t i = 0; i < u8Length; i++) {
        u8Crc ^= pu8Data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if ((u8Crc & 0x80) != 0) u8Crc = (uint8_t)((u8Crc << 1) ^ 0x2F);
            else u8Crc <<= 1;
        }
    }
    return (u8Crc ^ 0xFF);
}