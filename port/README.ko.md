# 포트의 소스

[English](README.md) | 한국어

포트의 모든 것은 `port/` 아래에 있습니다. CMake 프로젝트 하나(`CMakeLists.txt`)가 프로그램과 그 라이브러리, 테스트를 빌드합니다.
프로그램의 타깃은 `crusader`이고, 파일은 `crusader-port`(윈도우는 `crusader-port.exe`)입니다. 빌드하고 실행하는 법은 저장소의
[`README.ko.md`](../README.ko.md)에 있습니다.

## 구성

| 디렉터리 | 들어 있는 것 |
|---|---|
| `common/src/<group>` | 두 게임이 함께 쓰는 게임 코드. 서브시스템 그룹(아래)으로 나뉩니다. 게임 코드는 모두 여기에 한 번만 있습니다 |
| `regret/src/<group>`, `remorse/src/<group>` | 각 게임의 값과 자료. 공용 코드가 게임마다 다르게 받는 값의 헤더 `<name>_game.h`와, 그 게임만의 표와 배치(아래) |
| `unified/src/<group>` | 프로그램 하나가 실행 중에 고르는 게임별 값. `tools/make_unified_values.py`가 값 헤더에서 쓰는 헤더와, 각 게임의 자료 파일을 그 게임의 네임스페이스 안에서 컴파일하는 껍데기(`values_regret.cpp`, `values_remorse.cpp`) |
| `common/asylum` | 소리 모듈 ASYLUM.DLL(Asylum Sound System)의 C++판. 따로 된 라이브러리이고, 플랫폼의 소리 출력 위에서 돕니다 |
| `common/xan` | No Regret의 동영상 디코더 XANDRV32.DLL의 C++판. 따로 된 라이브러리입니다 |
| `common/fonts` | 한글 폰트 `ko.png`(Galmuri9, [`OFL.md`](common/fonts/OFL.md)). `tools/make_text.py`가 이것으로 `common/src/text/hangul_glyphs.cpp`를 씁니다 |
| `system` | 게임 밑의 PC. 게임의 라이브러리가 이 위에서 돕니다(아래) |
| `platform` | 시스템 층이 게임 코드에 주는 인터페이스: `audio.h`, `clock.h`, `files.h`, `gamestate.h`, `host.h`, `input.h`, `memory.h`, `passdump.h`, `screen.h`, `trace.h` |
| `compat` | 게임 코드가 쓰는 Borland C++ 3.1의 라이브러리와 DOS, Phar Lap의 헤더(`alloc.h`, `conio.h`, `dir.h`, `dos.h`, `io.h`, `mem.h`, `phapi.h`), 게임의 파일마다 먼저 넣는 `borland.h`, 크래시 보고서(`crash.h`), 포트의 버전(`portversion.h`) |
| `<game>/tables` | `intrinsics.tsv`: usecode 인트린직의 표(번호, 이름, 모듈, 인자와 반환값의 바이트 수). `tools/usecode.py intrinsics`가 usecode의 호출을 이 표와 맞대어 봅니다 |
| `lang` | 번역([`lang/README.md`](lang/README.md), 영어). 한국어 용어집과 줄거리는 [`lang/ko`](lang/ko) |
| `test`, `<game>/test` | 테스트([`test/README.md`](test/README.md), 영어). `unit/`은 부분마다, `scenario/`는 정해진 입력으로 도는 프로그램, `synth/`는 합성 장면, `<game>/test/expected`는 게임의 시나리오마다 보관된 결과 |
| `tools` | 생성기, 점검 도구, 보조 도구([`tools/README.md`](tools/README.md), 영어) |
| `cmake` | MinGW-w64 툴체인(`mingw-w64-x86_64.cmake`)과, 윈도우용 테스트를 wine에서 돌리는 `wine.sh` |

## 게임 코드

게임 코드는 원본 프로그램을 함수 하나하나 따릅니다. 원본의 기계어에서 되살린 소스를 C++로 옮긴 것이고, 원본의 이름을 아는
곳은 그 이름을 씁니다.

- 원본의 말투를 지킵니다. Borland C++ 3.1의 라이브러리(`compat`)를 쓰고, `char`는 부호가 있으며, 세이브에 들어가는 구조체는
  원본과 같은 배치라 세이브가 원본의 것과 같습니다.
- 게임의 전역 변수는 모두 게임의 아레나를 가리키는 참조입니다(`platform/gamestate.h`). 스냅숏과 되감기가 아레나를 통째로
  가져갑니다.

### 서브시스템 그룹

게임 코드의 파일은 저마다 그룹 하나에 속하고, 그룹은 모든 트리에서 같습니다(`tools/overlay.py`의 `GROUPS`).

| 그룹 | 들어 있는 것 |
|---|---|
| `kernel` | 프로세스 커널과 프로세스, 목록들, 타이머, 기다림 |
| `input` | 키보드, 마우스, 조이스틱, 입력의 분배, 포트의 키 설정 |
| `usecode` | usecode 인터프리터, 인트린직, 메모리, 검색, 프로세스 |
| `item` | 아이템, NPC, 알(egg), 아이템 캐시, 충돌, 거리, 중력 |
| `game` | 컨트롤러, 전투, 애니메이션, 활동, 공격, 미사일, 길찾기, 스프라이트 |
| `world` | 세계, 게임의 시작과 메인 루프, 게임의 시간, 맵 이동, 옵션, 세이브, 포트의 `enhanced.cfg` |
| `gump` | 검프(창): 대화 상자, 버튼, 메뉴, 불러오기와 저장 창, 단말기, 크레디트. 포트의 설정 화면과 키 설정 화면, 그 배치 |
| `display` | 표시 목록, 스프라이트 관리자, 카메라, 와이드 화면의 양옆 |
| `graph` | 팔레트, 페이드, 색 순환, 셰이프와 그 그리기, 뷰포트, VGA와 VESA 화면, BMP와 Targa |
| `sound` | 소리와 음악의 관리자, 소리 캐시, 소리 모듈의 인터페이스 |
| `movie` | 동영상(No Remorse의 RRV, No Regret의 Xan AVI)과 자막, 게임 속 flic(`FLICPLAY.C`) |
| `text` | 언어, 번역, 한글 |
| `file` | 파일과 FLEX 아카이브 |
| `cache` | 캐시와 그 핸들, 셰이프 캐시 |
| `system` | 게임 자신의 시스템 부분: 정지(halt), 난수, 유틸리티, 호스트의 힙 위에서 도는 메모리 관리자(`CPROTMEM.C`) |

원본의 TASM 모듈(`CMakeLists.txt`의 `ASM_MODULES`: 셰이프 그리기들, `view32`, `svgaprim`, `fastfind`, `framehit`, `cacheblk`,
`random`, `bitfield`, `xanthunk`)은 각 그룹 안의 C++ 파일입니다. 이것들은 따로 된 라이브러리(`asm_unified`)로 빌드해서,
테스트가 게임의 나머지 없이 링크할 수 있습니다.

### 두 게임, 프로그램 하나

두 게임의 차이는 `#ifdef` 같은 전처리기로 나누지 않습니다. 프로그램 하나가 실행 중에 게임을 고르기 때문입니다.

- 코드는 `common/src`에 한 번만 있습니다. 게임이 다른 곳에서는 그 게임의 값을 읽습니다.
- 게임의 값은 그 게임의 값 헤더(`<game>/src/**/<name>_game.h`, `system/profile.h`, `usecode/searches.h`)에 있는 `constexpr`
  선언입니다. `tools/make_unified_values.py`가 두 게임에 모두 있는 값마다 같은 이름의 변수를 `unified/src`에 쓰고, 시작할 때
  고른 게임이 그 변수를 채웁니다(`unified/src/system/values.cpp`). 값 헤더를 고치면 이 도구를 다시 돌립니다(ctest
  `unified_values_current`가 확인합니다).
- 게임의 자료는 그 게임 트리의 `.cpp` 파일입니다.
  - usecode 인트린직의 표(`intrinsics.cpp`)
  - 세이브의 배치(`savefields.cpp`)
  - 키의 기본값(`bindings_game.cpp`)
  - 번역(`translations.cpp`, `tools/make_text.py`가 씀)
  - 컨트롤러의 세이브되는 전역 변수(`control_game.cpp`)
  - 무기, 탄약, 인벤토리 아이템의 표(`dtable_game.cpp`), 무기 오버레이의 표(`wpncache_game.cpp`), 사격 종류의 표
    (`firetype_game.cpp`)
- 게임은 다른 무엇보다 먼저 고릅니다. 순서는 `CRUSADER_GAME`, 없으면 프로그램이 있는 폴더의 게임(거기 있는 원본 실행 파일),
  없으면 마지막에 한 게임(`enhanced.cfg`의 `game=`), 그것도 없으면 No Remorse입니다. `--game`은 그 게임으로 프로그램을 다시
  실행합니다(`system/restart.h`).

`tools/overlay.py --check`(ctest `overlay`)가 다음 규칙을 지킵니다.

- 파일은 자기 그룹의 디렉터리에 있습니다.
- 파일은 한 곳에만 있습니다. `#include "x.h"`는 포함하는 파일 옆을 먼저 찾기 때문입니다.
- 두 게임의 트리에 바이트가 같은 파일을 두지 않습니다. 그런 파일은 `common`에 있어야 합니다.
- 게임의 이름에 거는 전처리기 조건이 없습니다.
- include 경로의 다른 디렉터리에 있는 헤더와 이름이 같은 헤더가 없습니다.
- 두 헤더가 같은 include 가드를 쓰지 않습니다.

## 시스템 층

`system/`은 게임이 그 위에서 도는 PC이고, `platform/`의 인터페이스 아래에 있습니다.

| 파일 | 하는 일 |
|---|---|
| `pc.h`, `pcstate.*`, `interrupts.cpp`, `timer.cpp`, `memory.cpp` | 가상 PC: 가상 시간과 게임의 타이머 틱, 인터럽트 컨트롤러가 정하는 이벤트의 순서, 메모리(DOSBox처럼 16MB), 값으로 다루는 PC의 상태 |
| `keyboard.cpp`, `mouse.cpp`, `mousedrv.h` | 키보드의 스캔 코드, 마우스와 그 드라이버(INT 33h) |
| `video.*`, `console.cpp`, `audio.cpp` | 화면의 모드와 DAC의 색, 시작할 때의 텍스트 화면(표준 출력으로), 소리 출력 |
| `window.*`, `nowindow.cpp` | SDL 창: 그림, 소리의 스트림, 입력 이벤트, 게임패드. 창 없는 빌드에서는 스크립트로만 돕니다 |
| `files.cpp`, `filestate.h`, `iso9660.*`, `gamefolder.*`, `hostpath.h` | 호스트 위의 DOS 파일, 이미지(`GAME.GOG`, `GAME.BIN`)나 디렉터리에서 읽는 CD, 게임 폴더 |
| `commandline.*`, `main.cpp`, `end.*`, `restart.*` | 프로그램의 옵션, 시작과 끝, 다른 게임으로 다시 실행하기 |
| `replay.*`, `script.*`, `rewind.*`, `snapshot.*`, `capture.*`, `nut.*`, `png.*` | 녹화와 그 재생, 테스트 스크립트, 되감기와 상태, 재생하는 그림과 소리의 캡처(NUT), PNG 쓰기 |
| `report.*`, `unimplemented.*` | 실행이 끝나는 방식: 메시지, 창 위의 상자, `regret.log`(크래시 보고서 자체는 `compat/crash.h`가 씀), 포트가 이어 갈 수 없는 것 |
| `trace.cpp`, `statelog.cpp`, `movieframes.cpp`, `passdump.cpp`, `passstate.cpp` | 테스트가 읽는 것: usecode의 추적, 패스마다의 상태, 동영상의 프레임, 원본과 대조할 때 쓰는 패스 덤프 |
| `gamememory.*`, `newdelete.cpp` | 게임의 아레나, 프로그램의 `new`와 `delete` |

## 빌드 타깃

| 타깃 | 무엇인가 |
|---|---|
| `game_unified`, `asm_unified` | 두 게임의 게임 코드(`common/src`와 `unified/src`)와 그 TASM 모듈 |
| `asylum`, `xan` | 소리 모듈과 Xan 디코더 |
| `compat`, `game_memory` | Borland의 라이브러리, 크래시 보고서, 버전, 그리고 게임의 아레나 |
| `port_system` | 시스템 층. 프로그램의 main, 창, 아레나는 빠집니다 |
| `crusader` | 프로그램 `crusader-port` |
| `boundary` | 게임의 경계 검사(아래). `crusader`가 링크되기 전에 돕니다 |
| 테스트 | `test/CMakeLists.txt`와 `<game>/test/CMakeLists.txt`. 게임마다의 빌드 디렉터리는 `<game>-test`입니다 |

`-DPORT_EXTRA_TESTS=FILE`을 주면 포트의 테스트 뒤에 CMake 파일 하나를 더 읽어 테스트를 더합니다. 원본 프로그램과의 대조가
여기에 붙습니다([`test/README.md`](test/README.md#against-the-originals), 영어).

## 게임의 경계와 관리하는 표

### 게임의 경계(`tools/boundary.py`)

게임의 라이브러리(`game_unified`, `asm_unified`, `asylum`, `xan`)가 자기 밖에서 받아 쓰는 심볼을 규칙에 대 봅니다. 빌드가
프로그램을 링크하기 전에 돌리고(`CMakeLists.txt`의 타깃 `boundary`), 규칙을 어기면 빌드가 멈춰 `crusader-port`가 링크되지
않습니다. GCC, Clang, 새니타이저, 커버리지, MinGW-w64, macOS의 모든 빌드에서 돕니다.

| 허용되는 것 | 뜻 | 지금(GCC) |
|---|---|---|
| platform | `port/platform`의 헤더가 선언하는 함수. 시스템 층이 줍니다. 게임은 이것으로만 호스트에 닿습니다 | 125 |
| library | 포트가 지니는 `compat`의 함수(`KEPT`: Borland의 문자열 함수와 `rand`, 크래시 보고서) | 7 |
| runtime | 호스트에 부작용이 없는 C와 C++ 런타임의 함수(`PURE`: 메모리, 문자열, 문자, 버퍼로의 형식화, 파싱, `new`와 `delete`. `PURE_STD`: libc++가 라이브러리 밖에 두는 C++의 문자열과 표준 예외) | 27 |
| 컴파일러의 것 | 예외 처리, RTTI, 스택 보호, 스레드 지역 저장소, 새니타이저와 커버리지의 계측, C++ 라이브러리의 내부 함수 | 세지 않음 |

- 그 밖의 것은 모두 위반입니다. platform에 선언되지 않은 시스템 층의 심볼(게임이 가상 PC에 직접 닿음), `KEPT`에 없는
  `compat` 함수, 부작용이 없다고 알려지지 않은 런타임 함수(파일, 표준 출력, 환경 변수, 시계, 프로세스)가 여기 듭니다.
  위반하면 심볼과 그것을 쓰는 게임의 오브젝트를 알려 줍니다.
- 고치는 법: 게임에 platform 함수를 주어 그것을 거치게 합니다. 부작용이 없는 함수라면 `boundary.py`의 `PURE`에 더합니다.
- 툴체인마다 같은 것이 다른 이름으로 나와서, 규칙에 대기 전에 이름을 하나로 맞춥니다. c++filt로 C++ 이름을 풀고, Mach-O의 앞
  밑줄, `std::__1`과 `std::__cxx11`, MinGW-w64의 `__imp_`와 `__mingw_`를 떼고, `__memcpy_chk`나 `__isoc23_sscanf`는 원래
  함수로, `puts`는 `printf`로, Clang의 `bcmp`는 `memcmp`의 것으로 봅니다. 빌드도 오브젝트도 바꾸지 않고 비교할 때만 합니다.
- 다른 호스트(libretro, 브라우저)로 옮길 때는 platform 함수들이 새 호스트가 줘야 하는 것의 목록입니다.
  `python3 port/tools/boundary.py --list`가 모든 심볼을 상태와 함께 보입니다.
- 디버깅하며 게임 코드에 `printf`를 잠깐 넣을 때는 `-DPORT_BOUNDARY_CHECK=OFF`로 검사를 끕니다.

### `<game>/tables/intrinsics.tsv`

usecode 인트린직의 표입니다. 번호, 이름, 모듈, 인자와 반환값의 바이트 수가 있습니다. ctest `usecode_intrinsics`가 게임 자료의
usecode에서 인트린직을 부르는 곳마다 이 표와 맞는지 봅니다(게임 자료가 있을 때만). 원본에서 정해진 표라 거의 바뀌지 않습니다.

### 손으로 고치지 않는 파일

| 파일 | 만드는 것 | 확인 |
|---|---|---|
| `unified/src/**`(`system/values.cpp`, `system/values.h` 빼고) | `tools/make_unified_values.py` | ctest `unified_values_current` |
| `common/src/text/hangul_glyphs.cpp`, `<game>/src/text/translations.cpp` | `tools/make_text.py`(폰트와 `lang/`의 번역에서) | ctest `text` |
| `tools/exceptions.tsv` | `tools/exceptions.py --write`(규칙의 예외 목록) | ctest `exceptions` |
| `<game>/src/world/savefields.cpp`, `<game>/src/usecode/searches.h`, `<game>/src/usecode/intrinsics.cpp` | 원본의 자료(BCC의 구조체 배치, 원본의 vtable, `UNKCOFF.DAT`)에서 만든 것. 생성기는 원본이 있어야 돌아서 이 저장소에 없습니다 | ctest `savefields`, `usecode_intrinsics` |
