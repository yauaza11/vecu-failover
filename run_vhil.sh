#!/bin/bash

BUILD_DIR="./build"
QEMU_STM32="/home/soo/opt/xpack-qemu-arm-2.8.0-12/bin/qemu-system-gnuarmeclipse"

echo "[vHIL Launcher] 가상 차량 네트워크 클러스터 가동 시작..."

echo "[Node A] Active 제어기 Stand Up"
$QEMU_STM32 \
    -board STM32F4-Discovery \
    -mcu STM32F407VG \
    -image ${BUILD_DIR}/node_a_active \
    -nographic \
    > node_a.log 2>&1 &

sleep 0.3

echo "[Node B] Standby 제어기 Stand Up"
$QEMU_STM32 \
    -board STM32F4-Discovery \
    -mcu STM32F407VG \
    -image ${BUILD_DIR}/node_b_standby \
    -nographic \
    > node_b.log 2>&1 &

sleep 0.3

echo "[Node C] HIL 가상 차량 환경 및 계기판 모사 노드 가동"
echo "--------------------------------------------------------------------------"
echo "🚨 시뮬레이션을 종료하려면 Ctrl+C를 누르십시오. (모든 가상 머신 프로세스 일괄 종료)"
echo "--------------------------------------------------------------------------"

$QEMU_STM32 \
    -board STM32F4-Discovery \
    -mcu STM32F407VG \
    -image ${BUILD_DIR}/node_c_env \
    -nographic \
    -monitor none \
    -serial stdio