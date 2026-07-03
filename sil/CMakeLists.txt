cmake_minimum_required(VERSION 3.10)
project(VirtualHILPlatform_SIL C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -O0")

set(FREERTOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos)
set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Core)

# ==========================================================================
# 📂 SIL 전용 헤더 + FreeRTOS POSIX 포트 경로
#  - Core/Inc를 그대로 재사용하므로 can_driver.h, protocol.h, Rte_mock.h,
#    hil.h 등은 타겟 빌드와 동일한 파일을 그대로 가리킨다.
# ==========================================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
    ${CORE_DIR}/Inc
    ${FREERTOS_ROOT}/include
    ${FREERTOS_ROOT}/portable/ThirdParty/GCC/Posix
    ${FREERTOS_ROOT}/portable/ThirdParty/GCC/Posix/utils
)

set(FREERTOS_SRC
    ${FREERTOS_ROOT}/tasks.c
    ${FREERTOS_ROOT}/queue.c
    ${FREERTOS_ROOT}/list.c
    ${FREERTOS_ROOT}/portable/ThirdParty/GCC/Posix/port.c
    ${FREERTOS_ROOT}/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
    ${FREERTOS_ROOT}/portable/MemMang/heap_4.c
)

# ==========================================================================
# 🧠 BSW/ASW/RTE 공용 소스 - 타겟 빌드와 100% 동일한 파일
#  - MCAL(Can Driver)만 SocketCAN 구현체로 교체
# ==========================================================================
set(AUTOSAR_COMMON_SRC
    ${CORE_DIR}/Src/hil.c
    ${CORE_DIR}/Src/Rte_mock.c
    ${CORE_DIR}/Src/protocol.c
    ${CORE_DIR}/Src/can_driver_socketcan.c
)

find_package(Threads REQUIRED)

# [Node A] Active
add_executable(node_a_active_sil ${CORE_DIR}/Src/main_node_a.c ${AUTOSAR_COMMON_SRC} ${FREERTOS_SRC})
target_link_libraries(node_a_active_sil Threads::Threads)

# [Node B] Standby
add_executable(node_b_standby_sil ${CORE_DIR}/Src/main_node_b.c ${AUTOSAR_COMMON_SRC} ${FREERTOS_SRC})
target_link_libraries(node_b_standby_sil Threads::Threads)

# [Node C] Environment
add_executable(node_c_env_sil ${CORE_DIR}/Src/main_node_c.c ${AUTOSAR_COMMON_SRC} ${FREERTOS_SRC})
target_link_libraries(node_c_env_sil Threads::Threads)
