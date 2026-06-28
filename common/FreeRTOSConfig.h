#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

// ⚠️ [가상 빌드 환경 최종 패치]
// 하드웨어 의존성 헤더는 차단하되, 컴파일러가 문법 오류를 내지 않도록 
// 표준 인라인 및 가속 관련 핵심 매크로 명세만 가상으로 강제 이식합니다.
#ifndef __CMSIS_GCC_H
#define __CMSIS_GCC_H

  #ifndef __STATIC_INLINE
    #define __STATIC_INLINE static inline
  #endif
  #ifndef __NO_RETURN
    #define __NO_RETURN __attribute__((noreturn))
  #endif
  #ifndef __RESTRICT
    #define __RESTRICT __restrict
  #endif

  // 하드웨어 장벽 함수들 공백(NOP) 처리로 무력화
  #define __COMPILER_BARRIER() do { } while(0)
  #define __DSB()              do { } while(0)
  #define __ISB()              do { } while(0)
  #define __NOP()              do { } while(0)
  #define __DMB()              do { } while(0)
//   typedef int IRQn_Type; // 가상 인터럽트 타입 매핑
#endif

// ====================================================================
// 하단은 기존 형님의 완벽한 FreeRTOS 제어 스펙 코드 (그대로 유지)
// ====================================================================
#define configUSE_PREEMPTION                    1  
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      ((unsigned long)168000000) 
#define configTICK_RATE_HZ                      ((TickType_t)1000)         

#define configUSE_IDLE_HOOK                     0  
#define configUSE_TICK_HOOK                     0  

#define configMAX_PRIORITIES                    5  
#define configMINIMAL_STACK_SIZE                ((unsigned short)128)
#define configTOTAL_HEAP_SIZE                   ((size_t)(64 * 1024))      
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_MUTEXES                       1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8

#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1  
#define INCLUDE_vTaskDelay                      1

#endif /* FREERTOS_CONFIG_H */