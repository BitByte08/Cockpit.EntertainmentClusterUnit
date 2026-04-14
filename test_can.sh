#!/bin/bash

# CAN 테스트 스크립트
# ClusterModel에서 정의한 CAN ID에 맞춰 데이터 전송

echo "=== CAN 테스트 시작 ==="
echo "vcan0으로 테스트 데이터를 전송합니다."
echo ""

# 0x100: 속도 (2바이트, 빅엔디안)
# 예: 100 km/h = 0x0064
echo "속도 설정: 0 -> 120 km/h"
for speed in {0..120..10}; do
    hex=$(printf "%04X" $speed)
    echo "  속도: $speed km/h (0x$hex)"
    cansend vcan0 100#$hex
    sleep 0.1
done

echo ""
echo "RPM 설정: 0 -> 6000"
# 0x200: RPM (2바이트, 빅엔디안)
# 예: 3000 RPM = 0x0BB8
for rpm in {0..6000..500}; do
    hex=$(printf "%04X" $rpm)
    echo "  RPM: $rpm (0x$hex)"
    cansend vcan0 200#$hex
    sleep 0.1
done

echo ""
echo "연료 레벨: 100% -> 20%"
# 0x300: 연료 레벨 (1바이트, 0-100%)
for fuel in {100..20..-10}; do
    hex=$(printf "%02X" $fuel)
    echo "  연료: $fuel% (0x$hex)"
    cansend vcan0 300#$hex
    sleep 0.2
done

echo ""
echo "온도 설정: 90°C -> 110°C"
# 0x400: 온도 (1바이트, 섭씨)
for temp in {90..110..5}; do
    hex=$(printf "%02X" $temp)
    echo "  온도: $temp°C (0x$hex)"
    cansend vcan0 400#$hex
    sleep 0.2
done

echo ""
echo "=== 동시 테스트: 모든 값 변경 ==="
for i in {1..20}; do
    speed=$((RANDOM % 240))
    rpm=$((RANDOM % 8000))
    fuel=$((RANDOM % 100))
    temp=$((70 + RANDOM % 40))
    
    speed_hex=$(printf "%04X" $speed)
    rpm_hex=$(printf "%04X" $rpm)
    fuel_hex=$(printf "%02X" $fuel)
    temp_hex=$(printf "%02X" $temp)
    
    cansend vcan0 100#$speed_hex
    cansend vcan0 200#$rpm_hex
    cansend vcan0 300#$fuel_hex
    cansend vcan0 400#$temp_hex
    
    echo "  [$i/20] 속도: $speed km/h, RPM: $rpm, 연료: $fuel%, 온도: $temp°C"
    sleep 0.3
done

echo ""
echo "=== 테스트 완료 ==="
echo ""
echo "개별 명령어 사용법:"
echo "  속도 100km/h:  cansend vcan0 100#0064"
echo "  RPM 3000:      cansend vcan0 200#0BB8"
echo "  연료 50%:      cansend vcan0 300#32"
echo "  온도 95°C:     cansend vcan0 400#5F"
