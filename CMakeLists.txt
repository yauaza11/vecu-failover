cmake_minimum_required(VERSION 3.10)
project(VirtualHILPlatform C)

set(CMAKE_C_STANDARD 99)

# 💡 모든 ECU가 같이 쓸 공용 프로토콜/오픈소스 헤더 경로 주소록 등록
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freertos/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmsis_core/CMSIS/Core/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmsis_f4/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freertos/portable/ThirdParty/GCC/Posix
)

# 하위 3개 ECU 제어기 빌드 폴더 연동
add_subdirectory(node_a_pedal_ecu)
add_subdirectory(node_b_engine_ecu)
add_subdirectory(node_c_cluster_ecu)