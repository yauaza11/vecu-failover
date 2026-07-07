/* ==========================================================================
 * [Core/Src/can_driver_socketcan.c] - Can Driver(MCAL) SocketCAN 구현체
 *  - TODO: can_driver.h의 세 함수를 리눅스 SocketCAN(raw socket)으로 구현하세요.
 *  - 힌트: socket(PF_CAN, SOCK_RAW, CAN_RAW), struct can_frame, vcan0 bind
 *  - 막히면: git show main:Core/Src/can_driver_socketcan.c
 * ========================================================================== */
#include "can_driver.h"
#include "sys/socket.h"
#include <linux/can.h>
#include <linux/can/raw.h> 
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h> 


/* TODO: Can_Init / Can_Write / Can_Read 구현 */
static int fd = -1;

void Can_Init(void){

    struct ifreq ifr;
    struct sockaddr_can addr;
    int flags;      



    fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(fd<0){
        perror("socket");   // 에러 원인을 화면에 출력
        return;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ-1);

    if(ioctl(fd, SIOCGIFINDEX, &ifr)<0){
        perror("ioctl");
        return;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if(bind(fd,(struct sockaddr *)&addr, sizeof(addr))<0){
        perror("bind");
        return;
    }

    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

}

void Can_Write(uint32_t u32Id, const uint8_t *pData, uint8_t u8Dlc){
    struct can_frame frame;

    memset(&frame, 0, sizeof(frame));
    frame.can_id = u32Id & CAN_SFF_MASK;
    frame.can_dlc = u8Dlc; 
    memcpy(frame.data, pData, u8Dlc);

    write(fd, &frame, sizeof(frame));
}

bool Can_Read(CanFrame_Type *pFrame){
    struct can_frame frame;
    ssize_t iBytesRead;

    iBytesRead = read(fd, &frame, sizeof(frame));

    if(iBytesRead != (ssize_t)sizeof(frame)){
        return false;
    }

    pFrame->u32Id = frame.can_id & CAN_SFF_MASK;
    pFrame->u8Dlc = frame.can_dlc; 
    memcpy(pFrame->au8Data, frame.data, sizeof(pFrame->au8Data)); 

    return true;
}