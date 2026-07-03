/* ==========================================================================
 * [Core/Src/can_driver_socketcan.c] - Can Driver(MCAL) SocketCAN 구현체
 *  - SIL(리눅스 네이티브) 빌드 전용. can_driver.h 인터페이스를 리눅스
 *    SocketCAN raw socket으로 구현해 vcan0을 진짜 CAN 버스처럼 쓴다.
 * ========================================================================== */
#include "can_driver.h"

#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef CAN_IFACE_NAME
#define CAN_IFACE_NAME "vcan0"
#endif

static int s_iSocketFd = -1;

void Can_Init(void) {
    struct sockaddr_can sAddr;
    struct ifreq sIfr;
    int iFlags;

    s_iSocketFd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s_iSocketFd < 0) {
        perror("[CanDriver] socket");
        return;
    }

    memset(&sIfr, 0, sizeof(sIfr));
    strncpy(sIfr.ifr_name, CAN_IFACE_NAME, IFNAMSIZ - 1);
    if (ioctl(s_iSocketFd, SIOCGIFINDEX, &sIfr) < 0) {
        fprintf(stderr,
                "[CanDriver] '%s' 인터페이스를 찾을 수 없습니다. 먼저 아래 명령으로 가상 CAN 버스를 올려주세요:\n"
                "  sudo modprobe vcan\n"
                "  sudo ip link add dev %s type vcan\n"
                "  sudo ip link set up %s\n",
                CAN_IFACE_NAME, CAN_IFACE_NAME, CAN_IFACE_NAME);
        close(s_iSocketFd);
        s_iSocketFd = -1;
        return;
    }

    memset(&sAddr, 0, sizeof(sAddr));
    sAddr.can_family = AF_CAN;
    sAddr.can_ifindex = sIfr.ifr_ifindex;

    if (bind(s_iSocketFd, (struct sockaddr *)&sAddr, sizeof(sAddr)) < 0) {
        perror("[CanDriver] bind");
        close(s_iSocketFd);
        s_iSocketFd = -1;
        return;
    }

    /* Can_Read()가 태스크 주기마다 논블로킹 폴링으로 동작하도록 설정 */
    iFlags = fcntl(s_iSocketFd, F_GETFL, 0);
    fcntl(s_iSocketFd, F_SETFL, iFlags | O_NONBLOCK);
}

void Can_Write(uint32_t u32Id, const uint8_t *pData, uint8_t u8Dlc) {
    struct can_frame sFrame;

    if (s_iSocketFd < 0) {
        return;
    }

    memset(&sFrame, 0, sizeof(sFrame));
    sFrame.can_id = u32Id & CAN_SFF_MASK;
    sFrame.can_dlc = u8Dlc;
    memcpy(sFrame.data, pData, u8Dlc);

    write(s_iSocketFd, &sFrame, sizeof(sFrame));
}

bool Can_Read(CanFrame_Type *pFrame) {
    struct can_frame sFrame;
    ssize_t iBytesRead;

    if (s_iSocketFd < 0) {
        return false;
    }

    iBytesRead = read(s_iSocketFd, &sFrame, sizeof(sFrame));
    if (iBytesRead != (ssize_t)sizeof(sFrame)) {
        return false; /* EAGAIN 등 - 새로 도착한 프레임 없음 */
    }

    pFrame->u32Id = sFrame.can_id & CAN_SFF_MASK;
    pFrame->u8Dlc = sFrame.can_dlc;
    memcpy(pFrame->au8Data, sFrame.data, sizeof(pFrame->au8Data));
    return true;
}
