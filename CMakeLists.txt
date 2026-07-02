cmake_minimum_required(VERSION 3.10)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
project(VirtualHILPlatform C ASM)

set(CMAKE_C_STANDARD 99)

set(STM32_STARTUP_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmsis_f4/Source/Templates/gcc/startup_stm32f412cx.s
)

# ==========================================================================
# 🛠️ 1. ARM 크로스 컴파일러 툴체인 및 하드웨어 플래그 설정
# ==========================================================================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)

set(ARM_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -g3 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARM_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${ARM_FLAGS}") 

# 원래 있던 순정 링커 스크립트 연결
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nosys.specs -T ${CMAKE_CURRENT_SOURCE_DIR}/STM32F412Vx_FLASH.ld")

# ==========================================================================
# 📂 2. third_party 디바이스 헤더 정조준 패스 매핑
# ==========================================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/common                                     
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc                                  
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freertos/include              
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmsis_core/CMSIS/Core/Include 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmsis_f4 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmsis_f4/Include              
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freertos/portable/GCC/ARM_CM4F 
)

# ==========================================================================
# 📦 3. FreeRTOS 커널 소스코드 정의
# ==========================================================================
set(FREERTOS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freertos/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freertos/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freertos/list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freertos/portable/GCC/ARM_CM4F/port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freertos/portable/MemMang/heap_4.c
)

# ==========================================================================
# 🧠 4. Core/Src 밑에 격리된 AUTOSAR 공용 레이어 소스
# ==========================================================================
set(AUTOSAR_COMMON_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/hil.c          
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Rte_mock.c     
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/protocol.c     
)

# ==========================================================================
# 🚨 5. 독립 노드 분기 빌드선 세팅
# ==========================================================================
add_executable(node_a_active ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/main_node_a.c ${AUTOSAR_COMMON_SRC} ${FREERTOS_SRC} ${STM32_STARTUP_SRC})
target_link_libraries(node_a_active m) 

add_executable(node_b_standby ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/main_node_b.c ${AUTOSAR_COMMON_SRC} ${FREERTOS_SRC} ${STM32_STARTUP_SRC})
target_link_libraries(node_b_standby m)

add_executable(node_c_env ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/main_node_c.c ${AUTOSAR_COMMON_SRC} ${FREERTOS_SRC} ${STM32_STARTUP_SRC})
target_link_libraries(node_c_env m)