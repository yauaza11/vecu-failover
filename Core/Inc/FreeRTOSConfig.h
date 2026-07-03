#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ---------------------------------------------------------------------------
 * 🧠 가상 HIL 베어메탈 클러스터 구동용 FreeRTOS 커널 기본 플래그 명세
 * ---------------------------------------------------------------------------*/
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0  /* 💡 [추가] 아이들 훅 함수 미사용 */
#define configUSE_TICK_HOOK                     0  /* 💡 [추가] 틱 훅 함수 미사용 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      ( 168000000UL ) /* STM32F4 기본 클럭 가상 매핑 */
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 ) /* 1ms 정밀 해상도 틱 */
#define configMAX_PRIORITIES                    ( 5 )
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 128 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 46 * 1024 ) )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configQUEUE_REGISTRY_SIZE               8
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_APPLICATION_TASK_TAG          0
#define configUSE_COUNTING_SEMAPHORES           1

/* 태스크 제어 API 활성화 수준 정리 */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1

/* ---------------------------------------------------------------------------
 * 🚨 [핵심 교정] Cortex-M4 인터럽트 중첩 제어를 위한 우선순위 매크로 정의 섹션
 * ---------------------------------------------------------------------------*/
/* ARM Cortex-M4의 우선순위 비트 수 (STM32F4는 보통 4비트 하드웨어 구현) */
#define configPRIO_BITS                         4

/* 시스템 예외 및 일반 인터럽트 최하위 레벨 주소 지정 */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15

/* 💡 컴파일러 에러의 찐 범인 원천 해결! 커널이 관리할 최대 인터럽트 차단 마스킹 경계 레벨 */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* 하드웨어 NVIC 레지스터 직결용 내부 비트 시프트 매핑 변환 */
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* 가상 부팅 시 인터럽트 핸들러 매핑 명세 */
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTickHandler

#endif /* FREERTOS_CONFIG_H */