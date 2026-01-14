# hv_udp_test

간단한 POSIX 기반 UDP 에코/클라이언트 예제입니다. AGX Orin (aarch64 Linux)에서 네이티브로 빌드하고 테스트할 수 있도록 안내를 포함합니다.

## 파일
- `hv_udp_test.cpp` - UDP 서버/클라이언트 예제 소스
# AGX_Udp_test

이 저장소는 로컬(개발 머신)에서 UDP 기반 프레임 송신/수신을 빌드·검증하기 위한 예제입니다.

핵심 동작
- `tm_sender`: RAW 파일을 읽어 16바이트 `FrameHeader`를 앞에 붙여 UDP로 분할 송신합니다.
- `tm_receiver`: UDP 패킷을 수신·재조립하여 파일로 저장하고, 헤더와 원본 페이로드를 분리합니다.

요구사항
- Linux (Ubuntu 권장)
- CMake >= 3.10
- C++17 지원 컴파일러 (g++)

빌드
```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

실행 예시
1) 수신기 실행 (포트 5000):
```bash
./build/bin/tm_receiver 5000
```

백그라운드로 실행(로그 리다이렉트):
```bash
nohup ./build/bin/tm_receiver 5000 > tm_receiver_debug.log 2>&1 &
```

2) 송신기 실행 (로컬 테스트):
```bash
./build/bin/tm_sender 127.0.0.1 5000 test_data/raw/gradient_1920x1080.raw
```

생성 파일
- `sender_header.bin` : 송신측에서 기록한 16바이트 헤더
- `received_frame.bin` : 수신된 전체 프레임(헤더+페이로드)
- `received_header.bin` : 수신된 헤더(16바이트)
- `received_raw.bin` : 헤더 제거한 원본 페이로드

검증 예시
```bash
ls -l sender_header.bin received_header.bin received_raw.bin received_frame.bin
sha256sum sender_header.bin received_header.bin
cmp --silent received_raw.bin test_data/raw/gradient_1920x1080.raw && echo IDENTICAL || echo DIFFERENT
hexdump -C sender_header.bin
hexdump -C received_header.bin
```

문제 해결 포인트
- `tm_receiver_debug.log`에서 `Recv pkt:` 로그가 충분히 출력되는지 확인하세요.
- 대형 프레임은 많은 UDP 패킷으로 분할되므로, 수신 재조립 타임아웃(`FRAME_TIMEOUT_MS`)을 조정해야 할 수 있습니다.

자동화 스크립트
프로젝트 루트에는 편리한 스크립트가 포함되어 있습니다:

- `build.sh` : cmake 설정 및 빌드를 수행합니다.
- `run_test.sh` : 빌드 → 리시버 백그라운드 실행 → 송신 → 결과 검증 → (옵션) 리시버 종료/로그 보존까지 자동화합니다.

간단 사용 예:

```bash
# 빌드
./build.sh

# 전체 자동 테스트(기본 포트=5000, 기본 RAW 파일 사용)
./run_test.sh

# 리시버를 중지하지 않고 로그 보존 (테스트 후 계속 수신기 유지)
./run_test.sh --no-stop-receiver

# 리시버 중지하되 로그 삭제
./run_test.sh --no-keep-log

# 포트와 RAW 파일 지정
./run_test.sh --port 6000 --raw test_data/raw/my.raw
```

`run_test.sh` 옵션 요약:

- `--no-stop-receiver` / `--no-stop` : 테스트 후 리시버를 중지하지 않습니다.
- `--no-keep-log` / `--no-preserve-log` : 테스트 후 리시버 로그를 보존하지 않습니다.
- `--port N` / `-p N` : 수신 포트를 지정합니다.
- `--raw path` / `-r path` : 송신할 RAW 파일 경로를 지정합니다.

스크립트를 이용하면 수동으로 여러 명령을 실행하지 않아도 빠르게 E2E 확인이 가능합니다.
