#!/bin/bash
# ==========================================================================
# [sil/test_failover_latency.sh] - Node A -> Node B 페일오버 감지 지연시간 실측
#
#  방법: Node A를 kill -9로 죽인 정확한 시각(T_kill)과, Node B 로그에
#  "[ALERT]"가 찍힌 시각(T_alert)의 차이를 측정한다. N회 반복해서 평균을 낸다.
#
#  사용법: ./test_failover_latency.sh [반복횟수(기본 5)]
# ==========================================================================
set -uo pipefail

BUILD_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/build" && pwd)"
cd "$BUILD_DIR"

TRIALS="${1:-5}"

# 이전 실행에서 남은 프로세스가 있으면 같은 vcan0에 겹쳐 쏴서 결과가 오염된다.
# (세션 초반에 겪었던 좀비 프로세스 문제 - pkill -f 대신 pgrep으로 정확한 PID를 찾아 kill -9)
cleanup() {
    # "$BUILD_DIR/$name"으로 찾으면 안 됨: ./node_x처럼 상대경로로 띄우면
    # 프로세스의 실제 커맨드라인엔 절대경로가 안 남아서 pgrep -f가 매치를 못 한다.
    for name in node_a_active_sil node_b_standby_sil node_c_env_sil; do
        for pid in $(pgrep -f "$name" 2>/dev/null); do
            kill -9 "$pid" 2>/dev/null
        done
    done
    sleep 0.3
}
trap cleanup EXIT

echo "=== Node A -> Node B 페일오버 감지 지연시간 측정 (${TRIALS}회) ==="
echo "(이론상 상한: 1.5초 = 하트비트 3회 연속 누락 기준)"
echo ""

latencies=()

for i in $(seq 1 "$TRIALS"); do
    cleanup
    rm -f node_a_run.log node_b_run.log

    stdbuf -oL ./node_a_active_sil > node_a_run.log 2>&1 &
    node_a_pid=$!

    # Node B 출력 한 줄 나올 때마다 그 즉시 벽시계 타임스탬프를 앞에 붙여서 기록
    stdbuf -oL ./node_b_standby_sil 2>&1 | while IFS= read -r line; do
        echo "$(date +%s.%N) $line"
    done > node_b_run.log &

    sleep 2   # 안정화: Node A가 하트비트를 최소 몇 번 보낼 시간을 준다

    t_kill=$(date +%s.%N)
    kill -9 "$node_a_pid" 2>/dev/null

    # Node B 로그에서 [ALERT]가 나타날 때까지 최대 5초 대기
    t_alert=""
    for _ in $(seq 1 500); do
        line=$(grep "\[ALERT\]" node_b_run.log 2>/dev/null | head -1)
        if [ -n "$line" ]; then
            t_alert=$(echo "$line" | awk '{print $1}')
            break
        fi
        sleep 0.01
    done

    if [ -z "$t_alert" ]; then
        echo "  [$i] ALERT 못 찾음 (5초 타임아웃) - 이번 회차 스킵"
        continue
    fi

    latency=$(awk -v a="$t_kill" -v b="$t_alert" 'BEGIN{printf "%.3f", b-a}')
    printf "  [%d] 감지 지연시간: %s 초\n" "$i" "$latency"
    latencies+=("$latency")
done

cleanup

echo ""
if [ "${#latencies[@]}" -eq 0 ]; then
    echo "측정된 값이 없습니다 (전부 타임아웃)."
    exit 1
fi

sum=0
for l in "${latencies[@]}"; do
    sum=$(awk -v s="$sum" -v v="$l" 'BEGIN{printf "%.6f", s+v}')
done
count=${#latencies[@]}
avg=$(awk -v s="$sum" -v c="$count" 'BEGIN{printf "%.3f", s/c}')

echo "=== 결과 (${count}/${TRIALS}회 유효) ==="
echo "평균 감지 지연시간: ${avg}초 (이론상 상한: 1.5초)"
