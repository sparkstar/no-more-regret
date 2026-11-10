# 한국어 번역의 용어와 말투

두 게임의 단말기 글과 영상 자막을 한국어로 옮길 때 지키는 용어집과 말투 규칙이다. 이야기와 인물은 `STORY.md`에 있고, 번역 파일의 형식과
제어 문자(`^`, `*`, 조각)는 `../README.md`에 있다.

- 이 문서에는 용어와 짧은 이름만 둔다. 게임의 문장은 넣지 않는다(원문은 저장소에 두지 않는다).
- 미션 1의 번역에서 쓴 말을 기준으로 삼았다.

## 1. 정한 것

| # | 항목 | 고를 수 있던 것 | 정한 것(추천) |
|---|---|---|---|
| 1 | 별명 | 뜻을 살려 옮기기: Tin Man 양철 나무꾼, Wizard 마법사, Tax Man 세금쟁이, Weasel 족제비 / 소리대로: 틴맨, 위저드, 택스맨, 위즐 | 뜻을 살려 옮기기. 리브스가 "마법사, 나갈 표를 가져왔다"처럼 오즈의 마법사로 말장난을 한다. Yo-Yo(요요)와 Torch(토치)는 이름처럼 쓰므로 소리대로 |
| 2 | 리치의 직함(Director) | 국장(미션 1) / 소장 | 소장. 달 기지를 맡은 사람이라 기지 소장이 자연스럽다. "국장"은 정부 부처처럼 들린다 |
| 3 | Sergeant | 병장 / 중사 | 병장. 영화와 게임 번역에서 익숙하다(엄밀히는 부사관이라 중사에 가깝다) |
| 4 | 대사 속 아이템 이름 | 가) 화면의 영어 이름을 소리대로: 블라스트팩, 플라스마 실드, 래디에이션 실드, 인히비터, 스파이더 봄 / 나) 뜻으로: 플라스마 방어막, 방사선 방어막, 억제기, 거미 폭탄 / 다) 영어 그대로: PLASMA SHIELD | 가). 아이템 이름은 화면에서 영어로 남으므로 대사에서도 그 이름으로 알아볼 수 있어야 한다. 미션 1도 블라스트팩, 뎃팩으로 옮겼다. 모델명(BA-40, XP-5)은 그대로. 아이템 이름이 아닌 일반 명사(grenade launcher 유탄 발사기)는 뜻으로 |
| 5 | 데이터링크의 임무 목표 | ~한다(No Remorse 미션 1) / ~하라(No Regret 미션 1) | ~하라. 본부가 내리는 명령이다. 두 게임을 하나로 맞춘다 |
| 6 | Kiev | 키이우 / 키예프 | 키이우. 지금의 국립국어원 표기다 |
| 7 | Colonel Ely의 성 | 일리 / 일라이 | 일리. 성 Ely는 보통 "이-리"로 읽는다. 영상에서 들어 보고 다르면 고친다 |
| 8 | 바르가스가 대위에게 쓰는 말투 | 처음부터 끝까지 해요체 / 정체가 드러난 뒤(`13d`, `15e`) 반말 | 정체가 드러난 뒤 반말. 친절한 존댓말이 차가운 반말로 바뀌면 배신이 드러난다 |
| 9 | 욕설의 수위 | 원문대로(damn 젠장, bastards 개자식들) / 순화 | 원문대로 |

## 2. 표기

### 2.1 이름

- 사람과 땅 이름은 외래어 표기법을 따른다. 이미 정한 이름은 4장의 표를 따른다.
- 단말기 글에만 나오는 사람(편지의 보낸 이 등)은 표에 없어도 같은 규칙으로 옮긴다. 대문자 아이디(`DBEYER`, `MGREENE`, `DSNELL`)는
  로그인 이름이라 그대로 둔다.
- 약어는 그대로 둔다: WEC, LMC, SORC, MELF(M.E.L.F.), PHIR, OPTS, OPC, SATARG, EEOD, MFH, VIR, TGT, UNK, WECTS, WECHC, OWP, HQD.
- 모델명과 번호는 그대로 둔다: T-300, 1701-D, ZC90201, RT-5, Z-01, MST 3000, XP-5.

### 2.2 숫자, 시간, 층

- 층: level 4, fourth level, level four → 4층.
- 시각: 1330, 13:30 → 13시 30분. 0300 hours → 03시. 6:00am → 오전 6시.
- 화폐: credits → 크레디트.
- 암호와 코드의 숫자는 원문의 자리 그대로 둔다(usecode가 끼우는 숫자 포함). 숫자 바로 뒤에 조사를 두지 않는다(받침이 숫자마다
  달라진다): "암호: " + 숫자 + "."처럼 쓴다.

### 2.3 글자와 문장부호

- 쓸 수 있는 글자는 출력 가능한 ASCII와 KS X 1001 완성형 한글 2,350자다. 말줄임표는 `...`, 따옴표는 `"`와 `'`, 가운뎃점(`·`)과
  둥근 따옴표, 줄표(`—`)는 쓰지 않는다. 생성기가 글꼴에 없는 글자에서 멈춘다.
- 단말기 한 줄은 한글 16자 안팎(160픽셀), 한 쪽은 11줄이다. 줄은 단말기가 어절로 나누므로 원문이 20칸에 맞춰 넣은 줄바꿈은
  따르지 않는다. 머리말과 문단, 목록에만 `^`를 쓴다.
- 자막은 한 줄이다. 한 줄에 한글 약 60자가 들어가지만, 읽을 시간을 생각해 원문 길이 안팎으로 짧게 쓴다. 원문이 한 문장을 여러
  대사에 나눴으면 번역도 그 대사들에 나눠 이어지게 쓴다.

### 2.4 농담, 인용, 비속어

- 개발자들의 농담은 살린다: 치즈("The Cheese Man Cometh", 연극 제목 "The Iceman Cometh"의 말장난), 스타 트렉의 1701-D, DOS의
  "Bad command or file name"(한국어 MS-DOS의 "잘못된 명령이나 파일 이름"), 쥐와 고양이의 시(운율을 우리말로 살린다), "Love, Mom...".
- 오즈의 마법사: 양철 나무꾼(Tin Man)과 마법사(Wizard)는 짝이다(1장 1).
- 욕설은 원문의 수위를 따른다(1장 9).
- 원문의 오타(NETLIMK, securty, medecines)는 바르게 옮긴다. 일부러 깨진 글(`exp*erien`, `$#@**%`)은 깨진 모양을 살린다.

## 3. 말투

### 3.1 인물이 대위에게

| 인물 | 말투 | 예 |
|---|---|---|
| 맥시스 장군 | 하게체. 점잖은 원로 | 환영하네, 대위. / 행운을 비네. 이상 맥시스. |
| 일리 대령 | 해라체. 퉁명한 지휘관. 화가 나면 거칠다 | 여긴 내 기지다. 잘 들어, 대위. |
| 바르가스 소령 | 해요체. 다정한 척. 정체가 드러난 뒤 반말(1장 8) | 안녕하세요, 대위. / 잘 가, 대위. |
| 브룩스 병장 | 반말. 적의가 가득하다 | 내 눈앞에서 꺼져. |
| 카도바(요요) | 반말. 시비조, 나중엔 조금 풀린다 | 날 요요라고 불러도 좋아. |
| 앤드루스(세금쟁이) | 합니다체, 대위님. 공손하다 | 만나서 영광입니다, 대위님. |
| 리브스(마법사) | 반말. 수다스럽고 장난스럽다. 해커의 은어 | 잘했어, 양철 나무꾼! / 이상 마법사. |
| 윌마 교수 | 해요체. 겁 많고 투덜거린다 | 전 그냥 과학자라고요. |
| 족제비(Weasel) | 해요체와 합쇼체 섞인 장사꾼 말투, 손님 대접 | 어서 오세요, 대위님! 뭘 드릴까요? |
| 스넬 의원 | 하오체. 거드름 피우며 회유한다 | 값을 불러 보시오. |
| 호프먼 박사 | 하게체. 깔보는 학자 | 자네, 레지스탕스로군. |
| 드레이건 의장 | 하오체. 오만하고 냉정하다 | 잊지 않겠소, 사일런서. |
| 데닝 | 반말. 거칠고 유쾌하다 | 이봐, 사일런서! / 이상 데닝. |
| 토치 | 반말. 더 거칠다 | 지옥에 온 걸 환영해! / 이상 토치. |
| 셰퍼드 중령 | 해라체. 냉소적인 상관 | 반갑다, 대위. 난 맷 셰퍼드 중령이다. |
| 세카다 교수 | 해요체. 차분하다 | 대위, 조심하세요. / 이상 세카다. |

### 3.2 인물끼리

| 누가 → 누구에게 | 말투 |
|---|---|
| 드레이건 → 의원, 리치, 바르가스, 중위 | 하오체(미션 1과 같음) |
| 의원, 리치, 바르가스, 호프먼 → 드레이건 | 합니다체, "의장님" |
| 드레이건 → 대통령 | 합니다체, "대통령님", "각하" |
| 대통령 → 드레이건, 리치 | 하게체 |
| 리치 → 중위 | 해라체 |
| 중위 → 리치, 바르가스 | 합니다체, "소장님"(1장 2), "소령님" |
| 일리 ↔ 세카다 | 서로 해요체 |
| 인트로의 사일런서 분대 | 반말 |
| 화물선 승무원, 방송 | 무전은 짧은 해라체(여기는 화물칸 13. 뭔가 찾았다!), 방송은 합니다체 |
| 뉴스 진행자 | 합니다체. 밝은 선전 말투 |

### 3.3 단말기 글

- 공지, 시스템 메시지, 경고: 합니다체와 하십시오체(즉시 업무에 복귀하십시오). 짧은 상태 표시는 명사로 끝낸다(4.7).
- 보안팀의 내부 메모: 해라체(작전실 암호를 바꿨다. 꼭 지울 것!).
- 직원끼리의 쪽지: 관계에 따라 반말이나 해요체. 윗사람에게는 해요체, 동료와 연인에게는 반말.
- 데이터링크의 임무 목표: 1장 5.
- 머리글: From 보낸 이, To 받는 이, NOTICE 공지, WARNING 경고, ALERT 경보, ATTENTION 알림. 맺음: End of Message 메시지 끝,
  Have a nice day 좋은 하루 되십시오.

### 3.4 호칭과 무전

- Captain → 대위(앤드루스와 아랫사람은 대위님). Captain J. Banks처럼 배의 선장은 선장.
- Silencer → 사일런서. Tin Man → 양철 나무꾼. killer → 킬러, tough guy → 터프 가이, buddy → 친구.
- Mr. Chairman → 의장님, Senator X → X 의원, Doctor → 박사, Professor → 교수, Director → 소장님(1장 2), Mr. President → 대통령님,
  your excellency → 각하, Sir → (계급)님.
- "X, out." → "이상 X." / "I'm Torch and I'm out." → "난 토치, 이상."

## 4. 용어

표의 "확정"은 미션 1에서 썼거나 이견이 없는 것, "1장 N"은 1장에서 정한 것이다.

### 4.1 인물

| 영어 | 한국어 | 메모 |
|---|---|---|
| the Captain | 대위 | 주인공. 이름 없음 |
| Quentin Maxis, General Maxis | 쿠엔틴 맥시스, 맥시스 장군 | |
| Colonel Ely (Stephon Ely) | 일리 대령 | 1장 7 |
| Major Jo Anne Vargas | 조앤 바르가스 소령 | |
| Sergeant Shannon Brooks | 섀넌 브룩스 병장 | 1장 3. 나중에 중위 |
| Corporal Troy Reaves, Wizard | 트로이 리브스 상병, 마법사 | 1장 1 |
| Cardova, Yo-Yo | 카도바, 요요 | PFC 일병, Corporal 상병 |
| Private Derrich Andrews, Tax Man | 데릭 앤드루스 이병, 세금쟁이 | 1장 1. 데이터링크의 PFC. Andrews는 앤드루스 일병 |
| Professor Willmar | 윌마 교수 | |
| Weasel | 족제비 | 1장 1 |
| Chairman Nathaniel Draygan | 너새니얼 드레이건 의장 | |
| Senator Everett Snell | 에버렛 스넬 의원 | |
| Dr. Gregor Hoffman | 그레고르 호프먼 박사 | Greggy(애칭) 그레기 |
| President Gauthier | 고티에 대통령 | |
| Doyle | 도일 | 호프먼의 조수 |
| Vittek, Owens | 비텍, 오언스 | 인트로 |
| Trina, Josh | 트리나, 조시 | 뉴스 진행자 |
| Chuck Denning | 척 데닝 | |
| Torch | 토치 | |
| Lieutenant Colonel Matt Shepherd | 맷 셰퍼드 중령 | "Colonel Shepherd"도 셰퍼드 중령 |
| Professor Sekada | 세카다 교수 | 여성 |
| Director Judith Leach | 주디스 리치 소장 | 1장 2. 여성 |
| Captain J. Banks | J. 뱅크스 선장 | 화물선 |
| 단말기 글의 사람(No Remorse) | 스토크스, 브랜트, 필버트, M. 골웨이, R. 렌테리아, 콜슨 교수, 세브 도렌, 맷, 왓슨 박사, L. 프롭스트, 애시크래프트, 라슨, 수스 박사, 템플린·맨더펠드·셰필드·블레인·스펙터·B. 피트 의원, 칼, 대런, 필, 토니, 리처드, 재시카, 존, 테드, 메어, 로라 B., 데클런, 알렉, 로시요스 박사, 세일러 박사, 퀸스 대위, 에이어스 대위, 마커스 워튼, 블레이스 오초아, 티렐 박사, 얼, 노마, 오드리, T. 루니 교수, J.W. 골트 3세 의원 | 2.1의 규칙대로 옮긴 것. No Regret에 같은 이름이 나오면 따른다 |
| 단말기 글의 사람(No Regret) | 제이, 설리번, 노마, 미샤, 유리, 플리즈, 에브, JG, 카일, 숀, 루크, 요르기, 요르그, 토니, 카일라, 알렉스, 에리카, 제노, 앨런, 척, 아이작, 브래드버리, 검프, P. 짐버, 델로스, 스티븐, 드미트리, 알렉시스, 헨리, 마크, 마틸다, 월트, 오트마르, 킴, 퍼거스, 깁슨, 제프, 린다, 멜, 터틀리, 짐, 마커스 샬츠, 캐런, 드니즈, 헤더, 케빈, 호미, 원타버거, 예르겐, 제시카, 칸투 | 같은 규칙 |

### 4.2 조직, 직함, 계급

| 영어 | 한국어 | 메모 |
|---|---|---|
| WEC, World Economic Consortium | WEC, 세계 경제 컨소시엄 | |
| the Consortium | 컨소시엄 | Consortium scum → 컨소시엄 쓰레기 |
| the Resistance | 레지스탕스 | 조직 이름 |
| rebel(s) | 반군 | rebel base 반군 기지, rebel scum 반군 놈들 |
| Central | 본부 | 레지스탕스의 본부 |
| Command | 사령부 | WEC의 지휘부 |
| Security Cartel | 보안 카르텔 | |
| Lunar Mining Cartel (LMC) | 달 채굴 카르텔(LMC) | 보통 LMC |
| MilOps | 군작전부 | |
| Revenue Assessment Division | 세입 평가국 | |
| Department of Global Health and Education | 세계 보건 교육부 | |
| Department of Global Communications | 세계 통신부 | |
| Regulatory Commission | 규제 위원회 | |
| WEC information bureau | WEC 정보국 | |
| Lunar Research Division | 달 연구부 | |
| World Congress, Congress | 세계 의회, 의회 | |
| Senate, Senator | 상원, 상원 의원(부를 때 의원) | |
| Congressmen | 하원 의원 | |
| Appropriations Committee | 예산 위원회 | |
| President, Chairman | 대통령, 의장 | |
| Chernobyl Panel | 체르노빌 위원회 | |
| Elite Corps | 정예 군단 | 사일런서의 군단 |
| General, Colonel, Lieutenant Colonel, Major | 장군, 대령, 중령, 소령 | |
| Captain, Lieutenant | 대위, 중위 | |
| Sergeant, Corporal, PFC, Private | 병장, 상병, 일병, 이병 | 1장 3 |
| Guardsman, guard(s) | 경비병 | |
| trooper(s), soldier(s) | 병사 | WEC troopers → WEC 병사들 |
| Storm troopers, Elite troopers | 돌격병, 정예병 | 처치 기록 |
| Tech (2nd Class Tech) | 기술자(2급 기술자) | |
| Trooper (카도바가 대위를), trooper (브룩스) | 병정 양반, 병정 | 컨소시엄 군인을 비꼬는 말 |
| Consortium scum, Consortium slug | 컨소시엄 쓰레기, 컨소시엄 버러지 | |
| Branch Leaders | 지부장 | |
| Security Chief, Chief of Security | 보안 책임자 | |

### 4.3 장소와 시설

| 영어 | 한국어 | 메모 |
|---|---|---|
| Vigilance Platform, Vigilance Program | 비질런스 플랫폼, 비질런스 계획 | 확정 |
| Echo Sector | 에코 구역 | Sector는 구역(A-17 구역) |
| Petra, Geneva, Philadelphia, Kiev | 페트라, 제네바, 필라델피아, 키이우 | 키이우는 1장 6 |
| New York, Madrid, Houston | 뉴욕, 마드리드, 휴스턴 | |
| Allotment Center | 배급소 | 확정 |
| Powell Military Base | 파월 군사 기지 | |
| Farpark Data Processing Center | 파파크 데이터 처리 센터 | |
| Reilly Military Info Installation | 라일리 군사 정보 시설 | |
| Hawking Space Center | 호킹 우주 센터 | |
| John Enders Orbital Facility | 존 엔더스 궤도 시설 | |
| Kimball Research/Security Facility | 킴볼 연구 시설, 킴볼 보안 시설 | |
| OWP Research Facility | OWP 연구 시설 | |
| Draygan Correctional Institution | 드레이건 교정 시설 | |
| Snell Memorial Science Center | 스넬 기념 과학 센터 | |
| Orbital Industrial Platforms | 궤도 산업 플랫폼 | |
| Asylum BBS | 어사일럼 BBS | |
| the Moon, Lunar Base | 달, 달 기지 | LMC Lunar Mining Base → LMC 달 채굴 기지 |
| LMC Headquarters | LMC 본부 | |
| DarkSide Base | 다크사이드 기지 | 반군의 기지 |
| Lunar Tubeway, tram | 달 튜브웨이, 트램 | Tubeway station → 튜브웨이 역, transit station → 환승역(데이터링크) |
| Endymion | 엔디미온 | 화물선 이름. 확정 |
| Europa, Sheffield Industrial Center | 유로파, 셰필드 산업 센터 | |
| Pacific Rim | 환태평양 | |
| Gregor Hoffman Memorial Institute for Advanced Genetic Research | 그레고르 호프먼 기념 첨단 유전학 연구소 | |
| refinery | 정제소 | 확정 |
| sickbay, kitchenette | 의무실, 간이 부엌 | |
| Cloning Studio, Incubation Room, Hatching Chamber, Chrysalis Chambers | 복제 작업실, 배양실, 부화실, 번데기실 | 호프먼의 연구소 |
| Schmooze Room, Detention Hall, Courtroom, Clerical Division | 사교실, 구금 시설, 법정, 서무과 | 정부 단지 |
| BioWaste Reclamation Center | 생물 폐기물 재생 센터 | |

### 4.4 장비와 기술

| 영어 | 한국어 | 메모 |
|---|---|---|
| DataLink, Datalink | 데이터링크 | 확정 |
| ComLink | 컴링크 | |
| terminal, Network Access Terminal | 단말기, 네트워크 접속 단말기 | |
| Tubeway Guidance Terminal | 튜브웨이 유도 단말기 | |
| Trojan Horse program | 트로이 목마 프로그램 | |
| telepad, teleport pad, jump pad, teleporter | 텔레패드, 텔레포트 패드, 점프 패드, 텔레포터 | 확정(원문의 말을 따라 나눔) |
| teleport (동사) | 순간 이동하다 | |
| transport grid | 전송망 | 확정 |
| teleport screen, defense screens | 텔레포트 차단막, 방어막 | |
| Insertion Reconnaissance | 침투 정찰 | insertion coordinates 침투 좌표 |
| extraction, extraction specialist | 탈출, 탈출 전문가 | 확정 |
| sniffers | 추적꾼 | 리브스의 은어. WEC의 해커 |
| gadget jockeys, gadget brigade | 기계쟁이 | 리브스의 은어 |
| hazing filter, cross-relays | 교란 필터, 교차 중계기 | |
| insertion pad | 침투 패드 | |
| security card, keycard, access card | 보안 카드, 키카드, 출입 카드 | |
| passcode, code, security code | 암호, 코드, 보안 코드 | 숫자를 누르는 것은 암호 |
| Cypher chip | 사이퍼 칩 | |
| thermal coupler | 열 커플러 | 확정 |
| Thresher Cannon | 스레셔 포 | 확정 |
| force field, forcefield | 역장 | 확정 |
| laser field, laser barrier | 레이저 방벽 | 확정 |
| barrier wall | 차단벽 | |
| forcebridge, force bridge | 역장 다리 | |
| electric bridge, electronic bridge | 전기 다리 | |
| lightbridge | 라이트브리지 | 확정 |
| blast door | 방폭문 | 확정 |
| shield(s) (포탑, 보안) | 방어막 | 확정 |
| life pod, lifepod, escape pod | 탈출 캡슐 | 확정 |
| shuttle, troop transport | 셔틀, 병력 수송선 | |
| freighter | 화물선 | 확정 |
| Speed-Tele | 스피드텔레 | |
| EEOD nerve gas, MFH nerve gas | EEOD 신경가스, MFH 신경가스 | |
| lithium power core, coolant system | 리튬 동력로, 냉각 장치 | super-critical → 임계 초과 |
| SATARG (satellite targeting) | SATARG(위성 조준) | |
| Plasma Turbine, Plasma Talon | 플라스마 터빈, 플라스마 탤런 | 확정 |
| Ore Separator | 광석 분리기 | |
| Fusion Reactor | 핵융합로 | |
| Central Computer Core | 중앙 컴퓨터 코어 | |
| Oxygen Regenerator, O2 regeneration system | 산소 재생기, 산소 재생 장치 | |
| Lunar Defense Battery | 달 방어 포대 | |
| Di-Cor, Di-Correallium | 다이코, 다이코리얼리움 | 확정 |
| Radix | 래딕스 | 복수 radices도 래딕스 |
| Health Recharge Station | 체력 충전소 | |
| holo-map, net-tap, comm-bands | 홀로맵, 넷탭, 통신 대역 | |
| mind probe | 정신 탐사기 | |
| Embryotic Rescue process | 배아 구조법 | |
| Zero Tolerance policy | 무관용 정책 | |
| Strebor Disease | 스트레버병 | |
| RESISTNET | 레지스트넷 | 확정 |
| Kill Board | 처치 기록 | |
| WorldView | 월드뷰 | 뉴스 프로그램 |
| Consortium Express Card | 컨소시엄 익스프레스 카드 | |

### 4.5 로봇과 적

| 영어 | 한국어 | 메모 |
|---|---|---|
| Mech(s) | 메크 | 확정 |
| droid, service droid, android | 드로이드, 정비 드로이드, 안드로이드 | 확정 |
| servomech, servos | 서보메크, 서보 모터 | |
| Vetron, Thermatron, Avatron | 베트론, 서마트론, 아바트론 | 확정 |
| Solartron, Cryotron | 솔라트론, 크라이오트론 | |
| Cametron, Camtron | 캐메트론, 캠트론 | 원문이 둘 다 쓴다 |
| Roaming Susan | 로밍 수잔 | 확정 |
| Enforcer(s) | 인포서 | 호프먼이 만든 병사, 정예 부대 |
| MST 3000 Rail Gun, Floor Blaster | MST 3000 레일건, 플로어 블래스터 | |
| turret, wallgun, sentry | 포탑, 벽포, 경계포 | |
| ServoMech access station | 서보메크 접근 스테이션 | |
| Medichamber | 치료실 | |
| Morphing Security System | 변신 보안 시스템 | |

### 4.6 대사 속 아이템(1장 4의 가)

| 영어(화면 이름) | 한국어 |
|---|---|
| Blast PAC, BlastPac (BLAST PAC) | 블라스트팩 |
| Fusion PAC, FusionPac (FUSION PAC) | 퓨전팩 |
| DetPac (DET PAC) | 뎃팩 |
| PAC | 팩 |
| radiation shield (RADIATION SHIELD) | 래디에이션 실드 |
| plasma shield (PLASMA SHIELD) | 플라스마 실드 |
| Inhibitor (INHIBITOR) | 인히비터 |
| spider bomb (SPIDER BOMB) | 스파이더 봄 |
| Fusion Battery (FUSION BATTERY) | 퓨전 배터리 |
| Infrared Imager | 적외선 영상기(화면 이름과 다름: 일반 명사로) |
| grenade launcher | 유탄 발사기 |
| XP-5, BK-40 등 | 그대로 |

### 4.7 단말기의 상투어

| 영어 | 한국어 |
|---|---|
| ACTIVE, NOW ACTIVE | 작동, 작동 중 |
| ACTIVATED | 작동됨 |
| DEACTIVATED, FIELD DEACTIVATED | 해제됨, 방벽 해제됨 |
| INACTIVE | 꺼짐 |
| ONLINE, OFFLINE | 가동, 정지 |
| OPERATIONAL | 작동 중 |
| OUT OF SERVICE, NON-FUNCTIONING, DESTROYED | 사용 불가, 작동 불능, 파괴됨 |
| ACCESS GRANTED, ACCESS DENIED, APPROVED | 접근 허가, 접근 거부, 승인 |
| Accessing X | X 접속 중 |
| Now interfacing with remote droid | 원격 드로이드와 연결 중 |
| Unit is ONLINE | 장치 가동 |
| Please wait, Please stand by | 기다려 주십시오, 잠시 기다려 주십시오 |
| Remote Camera | 원격 카메라 |
| Watch Station | 감시 초소 |
| Red Alert, Yellow alert, Code Red | 적색 경보, 황색 경보, 적색 경보 |
| Alpha level, Security Level Alpha | 알파 경계 단계 |
| safe, floor safe, wall safe | 금고, 바닥 금고, 벽 금고 |
| security system, security panel | 보안 시스템, 보안 패널 |
| Override | 강제 제어 |

## 5. 새로 정한 말을 더하는 법

번역하다 표에 없는 말이 나오면 같은 갈래의 표에 더하고, 두 게임에 같은 말이 있으면 같은 번역을 쓴다. 이미 쓴 말을 바꾸면 그 말을 쓴
번역을 모두 고친다(`grep`으로 `lang/ko/`를 찾는다).
