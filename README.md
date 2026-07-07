# vecu-failover

Active-Standby 이중화 엔진 ECU를 AUTOSAR 스타일 계층 구조로 구현하고, **Can Driver(MCAL)를 교체 가능한 인터페이스로 분리**해서 두 가지 방식으로 검증하는 프로젝트입니다.

- **타겟/QEMU 빌드**: STM32F412 레지스터를 직접 제어하는 실제 펌웨어를 QEMU 위에서 실행 (PIL)
- **SIL(vECU) 빌드**: 동일한 BSW/ASW 코드를 리눅스 네이티브 프로세스로 컴파일하고, Linux SocketCAN(`vcan0`)을 실제 CAN 버스처럼 사용해 노드 간 통신을 검증 (SIL)

두 빌드는 `Core/Src/main_node_*.c`(BSW/ASW 로직)를 **완전히 동일하게 공유**하고, `can_driver_stm32.c` / `can_driver_socketcan.c`(MCAL)만 교체됩니다.

## 구성

```
node_a_active   — Active 엔진 ECU. 페달 신호로 RPM 계산, 결과 송신 + 하트비트 송신
node_b_standby  — Standby 엔진 ECU. 평소엔 감시만 하다가 Node A 장애 시 대신 계산·송신
node_c_env      — 가상 차량 환경. 페달 신호 생성 + 계기판(RPM 표시)
```

| CAN ID | 의미 | 송신 | 수신 |
|---|---|---|---|
| `0x200` | 페달 센서값 (20ms 주기) | Node C | Node A, Node B |
| `0x301` | 엔진 RPM 계산 결과 | 현재 Active인 쪽 (A 또는 B) | Node C |
| `0x100` | Node A 하트비트 (500ms 주기) | Node A | Node B |

모든 페이로드는 AUTOSAR 스타일 CRC-8로 무결성 검증됩니다 (`Core/Src/protocol.c`).

## 아키텍처

```
ASW  (hil.c, Simulink 생성 코드)
 ↓
RTE  (Rte_mock.c)
 ↓
BSW  (main_node_*.c — 페달 처리, 하트비트, 페일오버 FSM)
 ↓
MCAL (can_driver.h 인터페이스)
 ├─ can_driver_stm32.c      — 타겟/QEMU 빌드용 (CAN1 레지스터)
 └─ can_driver_socketcan.c  — SIL 빌드용 (Linux SocketCAN raw socket)
```

BSW/RTE/ASW는 두 빌드에서 한 글자도 바뀌지 않고, MCAL 구현체만 빌드 타깃에 따라 교체됩니다.

## 빌드

### 타겟 / QEMU

```bash
mkdir build && cd build
cmake .. && make -j4
./run_vhil.sh   # QEMU로 노드 3개 실행
```

### SIL (vECU)

```bash
cd sil
mkdir build && cd build
cmake .. && make -j4
```

SIL 빌드는 Linux SocketCAN을 사용하므로, 실행 전에 가상 CAN 인터페이스가 필요합니다:

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

## 실행 / 테스트

3개 터미널에서 각각 실행하고, 한 곳에서 `candump vcan0`로 버스를 관찰할 수 있습니다.

```bash
cd sil/build
./node_c_env_sil
./node_a_active_sil
./node_b_standby_sil
```

`node_a_active_sil`을 `Ctrl+C`로 죽이면 `node_b_standby_sil`에 `[ALERT]`가 찍히며 제어권을 이어받고, 다시 살리면 `[RECOVER]`로 제어권을 돌려줍니다.

### 페일오버 감지 지연시간 자동 측정

```bash
cd sil
./test_failover_latency.sh 5
```

Node A를 강제 종료한 시각과 `[ALERT]`가 찍힌 시각을 비교해 N회 반복 측정합니다.

**실측 결과 (5회 반복)**: 평균 감지 지연시간 **1.294초** (이론상 상한 1.5초 = 하트비트 500ms 3회 연속 누락 기준)

## 개발 중 발견한 버그

초기 아키텍처는 QEMU 프로세스 3개를 완전히 격리된 상태로 실행했는데, 이 경우 노드 간에 CAN 프레임이 물리적으로 전달될 방법이 없어 이중화 로직 자체를 검증할 수 없었습니다. SIL(SocketCAN) 방식으로 전환해 실제 노드 간 통신을 확보하자, 다음 버그가 즉시 드러났습니다:

1. 하트비트 전송 주기 카운터가 4bit 프로토콜 시퀀스 필드(`% 16`)를 재사용하고 있어, 의도한 500ms가 아니라 실제로는 160ms마다 하트비트가 전송됨
2. Standby 노드의 페일오버 타임아웃(50ms)이 그 160ms보다 짧아, Active 노드가 정상이어도 항상 페일오버가 발생함
3. 한 번 Active로 전환되면 다시 Standby로 복귀하는 로직이 없어, 두 노드가 영구적으로 같은 CAN ID(`0x301`)에 충돌 송신함

이 버그는 각 파일을 개별적으로 리뷰해서는 발견할 수 없고, **두 파일의 타이밍 상수를 실제로 같이 실행시켜봐야만** 드러나는 통합 테스트 전용 버그였습니다. 하트비트 주기를 별도 카운터로 분리하고, 타임아웃을 500ms보다 충분히 크게(1.5초) 조정하고, Active→Standby 복구 경로를 추가해 해결했습니다.

## 한계

- AUTOSAR NM(Network Management) 같은 표준화된 프로토콜이 아니라 단순화된 커스텀 하트비트 방식
- "응답이 없음(장애)"만 감지하며, "잘못된 값을 계속 계산하는" 고장 모드는 감지하지 못함 (다수결 검증 없음)
- 타임아웃 값(1.5초)은 실험적으로 정한 값이며, FMEA/FTTI 기반 정식 안전 분석에서 도출된 값이 아님
- CanIf 등 ECU Abstraction Layer 없이 BSW가 MCAL을 직접 호출 (레이어 단순화)
