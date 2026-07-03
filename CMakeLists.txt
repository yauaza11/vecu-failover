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

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nosys.specs")

set(LINKER_SCRIPT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/STM32F412Vx_FLASH.ld")

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
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/can_driver_stm32.c
)

# ==========================================================================
# 🚨 5. 독립 노드 분기 빌드선 세팅 (CMake 공식 프로퍼티 주입 구조)
#  - TARGET_MCU 정의로 STM32 전용 코드(IWDG 등)를 활성화한다.
#    같은 소스가 SIL(sil/CMakeLists.txt) 빌드에서는 이 정의 없이 재사용된다.
# ==========================================================================
# [Node A] Active
add_executable(node_a_active ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/main_node_a.c ${AUTOSAR_COMMON_SRC} ${FREERTOS_SRC} ${STM32_STARTUP_SRC})
target_compile_definitions(node_a_active PRIVATE TARGET_MCU)
set_target_properties(node_a_active PROPERTIES LINK_FLAGS "-T ${LINKER_SCRIPT_PATH}")
target_link_libraries(node_a_active m)

# [Node B] Standby
add_executable(node_b_standby ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/main_node_b.c ${AUTOSAR_COMMON_SRC} ${FREERTOS_SRC} ${STM32_STARTUP_SRC})
target_compile_definitions(node_b_standby PRIVATE TARGET_MCU)
set_target_properties(node_b_standby PROPERTIES LINK_FLAGS "-T ${LINKER_SCRIPT_PATH}")
target_link_libraries(node_b_standby m)

# [Node C] Environment
add_executable(node_c_env ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/main_node_c.c ${AUTOSAR_COMMON_SRC} ${FREERTOS_SRC} ${STM32_STARTUP_SRC})
target_compile_definitions(node_c_env PRIVATE TARGET_MCU)
set_target_properties(node_c_env PROPERTIES LINK_FLAGS "-T ${LINKER_SCRIPT_PATH}")
target_link_libraries(node_c_env m)