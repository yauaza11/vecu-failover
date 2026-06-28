cmake_minimum_required(VERSION 3.10)

# (main.c)과 계기판 수신 로직(cluster_task.c) 묶기
set(NODE_C_SOURCES
    main.c
    cluster_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/protocol.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/portable/ThirdParty/GCC/Posix/port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/portable/MemMang/heap_4.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
)

# 🚀 계기판 제어기 실행 파일 생성 지시
add_executable(node_c_cluster_ecu ${NODE_C_SOURCES})