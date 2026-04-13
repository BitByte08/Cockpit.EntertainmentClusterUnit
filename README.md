# Car Entertainment Cluster Unit

Raspberry Pi 기반 자동차 디지털 클러스터. Qt6 + SocketCAN으로 CAN 버스 데이터를 실시간 렌더링하며, 부팅 시 OTA 자동 업데이트를 지원합니다.

---

## 화면 구성

```
┌──────────────────────────────────────────────────┐
│                                                  │
│   ┌──────────────┐   ┌────────────────────────┐  │
│   │              │   │  SPEED                 │  │
│   │   RPM        │   │                        │  │
│   │   게이지      │   │            87          │  │
│   │   (아날로그)  │   │           km/h         │  │
│   │              │   │ ─────────────────────  │  │
│   │              │   │  FUEL          72%     │  │
│   │              │   │ ─────────────────────  │  │
│   └──────────────┘   │  ENGINE TEMP   92°C    │  │
│                      └────────────────────────┘  │
└──────────────────────────────────────────────────┘
```

- **좌측**: RPM 아날로그 게이지 (0 – 8,000 RPM, 애니메이션 바늘)
- **우측**: 속도 / 연료 / 엔진 온도 텍스트 패널 (임계값 초과 시 색상 경고)

---

## 프로젝트 구조

```
CarEntertainmentClusterUnit/
├── apps/
│   └── cluster/
│       ├── models/          # ClusterModel — CAN 프레임 파싱
│       ├── widgets/         # GaugeWidget — 아날로그 RPM 게이지
│       ├── UpdateManager    # OTA 업데이트 클라이언트
│       ├── mainwindow.*
│       └── main.cpp
├── core/
│   └── can_interface/       # CANInterface / SocketCANInterface / StubCANInterface
├── tests/                   # QTest 유닛 테스트
├── scripts/
│   ├── install.sh           # RPi 최초 설치 스크립트
│   ├── update.sh            # 부팅 시 OTA 업데이트 스크립트
│   ├── cluster-kiosk.service
│   └── cluster-update.service
├── .github/workflows/
│   └── ci-release.yml       # GitHub Actions CI/CD
├── CMakeLists.txt
└── VERSION
```

---

## CAN 메시지 포맷

| CAN ID | 데이터 | 단위 |
|--------|--------|------|
| `0x100` | `[MSB, LSB]` | 속도 (km/h) |
| `0x200` | `[MSB, LSB]` | RPM |
| `0x300` | `[byte0]` | 연료 (0 – 100%) |
| `0x400` | `[byte0]` | 엔진 온도 (°C) |

---

## 하드웨어 요구사항

- Raspberry Pi 4 / 5 (ARM64)
- [Seengreat RS485 Dual CAN I HAT](https://seengreat.com/wiki/83/rs485-dual-can-i)
  - MCP2515 × 2 (SPI 연결)
  - CAN0: SPI0, CS=BCM8, INT=BCM25
  - CAN1: SPI1, CS=BCM17, INT=BCM24
  - 로직 전압 슬라이드 스위치 → **3.3V**
  - DIP 스위치 기본값 → **P11, P6, P0, P5 ON**

---

## 빠른 설치 (Raspberry Pi)

### 1. 최초 설치

저장소를 클론하거나 릴리스에서 `install.sh`를 바로 받아 실행합니다.

```bash
# 릴리스에서 설치 스크립트 + 바이너리 직접 다운로드
curl -L https://github.com/bitbyte08/CarEntertainmentClusterUnit/releases/latest/download/install.sh \
  -o install.sh

curl -L https://github.com/bitbyte08/CarEntertainmentClusterUnit/releases/latest/download/cluster-arm64 \
  -o cluster-arm64

sudo bash install.sh ./cluster-arm64
sudo reboot
```

또는 저장소 클론 후 설치:

```bash
git clone https://github.com/bitbyte08/CarEntertainmentClusterUnit.git
cd CarEntertainmentClusterUnit
sudo bash scripts/install.sh
sudo reboot
```

### 2. 바이너리만 수동 업데이트

```bash
curl -L https://github.com/bitbyte08/CarEntertainmentClusterUnit/releases/latest/download/cluster-arm64 \
  -o /tmp/cluster-arm64

sudo install -m 755 /tmp/cluster-arm64 /opt/cluster/cluster
sudo systemctl restart cluster-kiosk
```

### 3. 현재 설치 버전 확인

```bash
cat /opt/cluster/VERSION
```

### 4. OTA 업데이트 수동 실행

```bash
sudo /opt/cluster/update.sh
```

### 5. CAN 인터페이스 상태 확인

```bash
ip link show can0
ip link show can1

# 수신 테스트
candump can0

# 송신 테스트 (can1 → can0)
cansend can1 100#0000
```

---

## 빌드 (개발 환경)

### 의존성

```bash
# Ubuntu / Raspberry Pi OS
sudo apt-get install -y \
  qt6-base-dev qt6-tools-dev libqt6network6-dev \
  cmake ninja-build can-utils
```

### 빌드

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### 테스트 포함 빌드

```bash
cmake -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### 실행

```bash
# 일반 창 모드
./build/apps/cluster/cluster

# 키오스크 풀스크린
./build/apps/cluster/cluster --fullscreen

# 환경변수로도 가능
CLUSTER_KIOSK=1 ./build/apps/cluster/cluster
```

---

## CI/CD

GitHub Actions (`.github/workflows/ci-release.yml`)

| 트리거 | 동작 |
|--------|------|
| PR / `main` 푸시 | ARM64 빌드 + 유닛 테스트 |
| `v*` 태그 푸시 | 릴리스 빌드 + GitHub Release 자동 생성 |

### 릴리스 태그 방법

```bash
# VERSION 파일 수정 후
echo "1.2.0" > VERSION
git add VERSION
git commit -m "chore :: bump version to 1.2.0"
git tag v1.2.0
git push origin main --tags
```

태그가 푸시되면 Actions가 자동으로 `cluster-arm64` 바이너리를 빌드해 GitHub Release에 업로드합니다. 이후 RPi는 다음 부팅 시 자동으로 업데이트됩니다.

---

## OTA 업데이트 흐름

```
RPi 부팅
  │
  ├─ [1] can-setup.service
  │       can0 / can1 → 500Kbps UP
  │
  ├─ [2] cluster-update.service  (oneshot)
  │       /opt/cluster/update.sh 실행
  │       └─ GitHub API 조회 → 신규 버전 있으면 다운로드 & 교체
  │
  └─ [3] cluster-kiosk.service
          Qt 앱 풀스크린 실행
          └─ 앱 내부에서도 5초 후 버전 체크
             신규 버전 감지 시 자동 다운로드 → 재시작
```

---

## 서비스 관리

```bash
# 상태 확인
sudo systemctl status cluster-kiosk
sudo systemctl status cluster-update
sudo systemctl status can-setup

# 로그 확인
journalctl -u cluster-kiosk -f
journalctl -u cluster-update --no-pager

# 수동 재시작
sudo systemctl restart cluster-kiosk
```
