# Cockpit Entertainment Cluster Unit

Raspberry Pi 기반 자동차 콕핏 디스플레이 프로젝트입니다. 이 저장소는 두 개의 Qt6/C++
앱을 같이 관리하지만, 실제 차량 구성에서는 **서로 다른 Raspberry Pi 두 대**에 각각
설치해서 실행하는 것을 기준으로 합니다.

| 장치 | 앱 | 설치 경로 | 릴리스 바이너리 | systemd 서비스 |
|------|-----|-----------|-----------------|----------------|
| Cluster Pi | 계기판 | `/opt/cluster` | `cluster-arm64` | `cluster-kiosk`, `cluster-update` |
| Entertainment Pi | 내비/엔터테인먼트 | `/opt/entertainment` | `entertainment-arm64` | `entertainment-kiosk`, `entertainment-update` |

두 Pi 모두 SocketCAN의 `can0`을 기본 입력으로 사용하며, CAN bitrate는 500 Kbps입니다.

---

## 프로젝트 구조

```text
Cockpit.EntertainmentClusterUnit/
├── apps/
│   ├── cluster/              # 계기판 앱
│   └── entertainment/        # 내비/엔터테인먼트 앱
├── core/can_interface/       # SocketCAN / Stub CAN 공통 인터페이스
├── scripts/
│   ├── install.sh            # Cluster Pi 최초 설치
│   ├── update.sh             # Cluster Pi OTA 업데이트
│   ├── cluster-*.service
│   ├── entertainment-install.sh
│   ├── entertainment-update.sh
│   └── entertainment-*.service
├── tests/
├── tools/
├── .github/workflows/
│   ├── ci.yml
│   └── release.yml
└── VERSION
```

---

## 하드웨어 요구사항

- Raspberry Pi 4 / 5, 64-bit Raspberry Pi OS
- 800 x 480 디스플레이
- Seengreat RS485 Dual CAN I HAT
  - MCP2515 x 2
  - CAN0: SPI0, CS=BCM8, INT=BCM25
  - CAN1: SPI1, CS=BCM17, INT=BCM24
  - 로직 전압 슬라이드 스위치: **3.3V**
  - DIP 스위치 기본값: **P11, P6, P0, P5 ON**

설치 스크립트는 SPI, MCP2515 overlay, `can-setup.service`, X11 키오스크 자동 시작을
설정합니다. 각 Pi에는 자기 역할의 설치 스크립트만 실행하세요.

---

## Raspberry Pi 설치

### Cluster Pi

릴리스에서 설치:

```bash
curl -L https://github.com/BitByte08/Cockpit.EntertainmentClusterUnit/releases/latest/download/install.sh \
  -o install.sh
curl -L https://github.com/BitByte08/Cockpit.EntertainmentClusterUnit/releases/latest/download/cluster-arm64 \
  -o cluster-arm64

sudo bash install.sh ./cluster-arm64
sudo reboot
```

저장소를 클론해서 설치:

```bash
git clone https://github.com/BitByte08/Cockpit.EntertainmentClusterUnit.git
cd Cockpit.EntertainmentClusterUnit
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target cluster --parallel
sudo bash scripts/install.sh build/apps/cluster/cluster
sudo reboot
```

### Entertainment Pi

릴리스에서 설치:

```bash
curl -L https://github.com/BitByte08/Cockpit.EntertainmentClusterUnit/releases/latest/download/entertainment-install.sh \
  -o entertainment-install.sh
curl -L https://github.com/BitByte08/Cockpit.EntertainmentClusterUnit/releases/latest/download/entertainment-arm64 \
  -o entertainment-arm64

sudo bash entertainment-install.sh ./entertainment-arm64
sudo reboot
```

저장소를 클론해서 설치:

```bash
git clone https://github.com/BitByte08/Cockpit.EntertainmentClusterUnit.git
cd Cockpit.EntertainmentClusterUnit
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target entertainment --parallel
sudo bash scripts/entertainment-install.sh build/apps/entertainment/entertainment
sudo reboot
```

지도 타일과 도로 그래프는 Entertainment Pi에 별도로 복사합니다.

```bash
scp -r tiles/ pi@<ENTERTAINMENT_PI_IP>:/opt/entertainment/tiles/
scp road_graph.json pi@<ENTERTAINMENT_PI_IP>:/opt/entertainment/road_graph.json
```

`road_graph.json`이 없으면 앱은 위성 타일 모드로 실행되고, 파일이 있으면 내비게이션
경로 모드가 활성화됩니다.

---

## 수동 업데이트

Cluster Pi:

```bash
curl -L https://github.com/BitByte08/Cockpit.EntertainmentClusterUnit/releases/latest/download/cluster-arm64 \
  -o /tmp/cluster-arm64
sudo install -m 755 /tmp/cluster-arm64 /opt/cluster/cluster
sudo systemctl restart cluster-kiosk
```

Entertainment Pi:

```bash
curl -L https://github.com/BitByte08/Cockpit.EntertainmentClusterUnit/releases/latest/download/entertainment-arm64 \
  -o /tmp/entertainment-arm64
sudo install -m 755 /tmp/entertainment-arm64 /opt/entertainment/entertainment
sudo systemctl restart entertainment-kiosk
```

OTA 스크립트 직접 실행:

```bash
sudo /opt/cluster/update.sh
sudo /opt/entertainment/update.sh
```

---

## CAN 메시지 포맷

| CAN ID | 이름 | 데이터 | 사용 앱 |
|--------|------|--------|---------|
| `0x300` | `SWITCH_STATUS` | `uint16` little-endian bitfield | Cluster |
| `0x301` | `GEAR_STATUS` | `byte0`: `0=N`, `1..6`, `7=R` | Cluster |
| `0x400` | `INFO_SPEED_RPM` | `[speed_x10 u16 BE][rpm u16 BE]` | Cluster, Entertainment |
| `0x401` | `INFO_WARNING` | `uint16` little-endian bitfield | Cluster |
| `0x500` | `VEHICLE_STATE` | `[speed_x10 u16 BE][rpm u16 BE][gear][flags]` | Cluster, Entertainment |
| `0x501` | `ENGINE_STATE` | `[coolant C][oil 0..100][fuel 0..100]` | Cluster |
| `0x600` | `POSITION` | `[x int32 BE x100][z int32 BE x100]` | Entertainment |
| `0x601` | `HEADING` | `[heading_x10 u16 BE]` | Entertainment |

`0x300` switch bitfield:

| Bit | 의미 |
|-----|------|
| 0 | Ignition |
| 1 | Engine |
| 3 | High beam |
| 4 | Hazard |
| 8 | Turn left |
| 9 | Turn right |

`0x401` warning bitfield:

| Bit | 의미 |
|-----|------|
| 0 | Check engine |
| 1 | Oil pressure |
| 4 | Fuel low |

예시:

```bash
# speed 100.0 km/h, rpm 3000
cansend can0 400#03E80BB8

# coolant 95 C, oil 60%, fuel 72%
cansend can0 501#5F3C48

# x=12.34 m, z=-5.00 m
cansend can0 600#000004D2FFFFFE0C
```

---

## CAN 상태 확인

```bash
ip link show can0
ip -details link show can0
candump can0
```

dual CAN HAT에서 `can0`과 `can1`을 물리적으로 연결해 송수신을 확인할 수도 있습니다.

```bash
candump can0
cansend can1 400#03E80BB8
```

---

## 개발 환경

의존성:

```bash
sudo apt-get install -y \
  qt6-base-dev qt6-tools-dev cmake ninja-build can-utils
```

빌드:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

실행:

```bash
CLUSTER_CAN_IF=vcan0 ./build/apps/cluster/cluster
ENTERTAINMENT_CAN_IF=vcan0 ./build/apps/entertainment/entertainment

CLUSTER_KIOSK=1 ./build/apps/cluster/cluster
ENTERTAINMENT_KIOSK=1 ./build/apps/entertainment/entertainment
```

테스트:

```bash
cmake -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

통합 테스트는 `vcan0`을 준비한 뒤 실행합니다.

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
tests/integration/run_test.sh
```

---

## 릴리스

`v*` 태그를 푸시하면 GitHub Actions가 ARM64에서 빌드하고 다음 릴리스 에셋을 업로드합니다.

- `cluster-arm64`
- `entertainment-arm64`
- `install.sh`
- `update.sh`
- `cluster-kiosk.service`
- `cluster-update.service`
- `entertainment-install.sh`
- `entertainment-update.sh`
- `entertainment-kiosk.service`
- `entertainment-update.service`
- `VERSION`
- `build-info.txt`

```bash
echo "1.2.0" > VERSION
git add VERSION
git commit -m "chore: bump version to 1.2.0"
git tag v1.2.0
git push origin main --tags
```

---

## 서비스 관리

Cluster Pi:

```bash
sudo systemctl status cluster-kiosk
sudo systemctl status cluster-update
journalctl -u cluster-kiosk -f
sudo systemctl restart cluster-kiosk
```

Entertainment Pi:

```bash
sudo systemctl status entertainment-kiosk
sudo systemctl status entertainment-update
journalctl -u entertainment-kiosk -f
sudo systemctl restart entertainment-kiosk
```

공통 CAN 서비스:

```bash
sudo systemctl status can-setup
sudo systemctl restart can-setup
```
