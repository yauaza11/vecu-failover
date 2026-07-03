#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ---------------------------------------------------------------------------
 * 🧠 SIL(리눅스 네이티브 POSIX 포트) 전용 FreeRTOS 커널 설정
 *  - MCU 빌드의 FreeRTOSConfig.h(Core/Inc)와는 별개다. Cortex-M 인터럽트
 *    우선순위 매크로(configPRIO_BITS 등)는 POSIX 포트에 존재하지 않으므로 넣지 않는다.
 * ---------------------------------------------------------------------------*/
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      ( 1000000UL )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 ) /* 1ms 정밀 해상도 틱 */
#define configMAX_PRIORITIES                    ( 5 )
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 512 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 1024 * 1024 ) )
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

#endif /* FREERTOS_CONFIG_H */
