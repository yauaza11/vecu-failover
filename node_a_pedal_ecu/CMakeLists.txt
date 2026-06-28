cmake_minimum_required(VERSION 3.10)

# (main.c)과 페달 핵심 로직(pedal_task.c) 묶기
set(NODE_A_SOURCES
    main.c
    pedal_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/protocol.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/portable/ThirdParty/GCC/Posix/port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/portable/MemMang/heap_4.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
)

# 가속 페달 실행 파일 생성 지시
add_executable(node_a_pedal_ecu ${NODE_A_SOURCES})