# YJJJump Tools

모두 에디터를 끈 상태에서 실행한다. 엔진 경로 기본값은 `D:\source\repos\UnrealEngine\Engine` (`-EngineDir`로 변경).

| 스크립트 | 하는 일 |
|---|---|
| `Art/BuildArt.ps1` | `Art/generate_art.py`로 픽셀아트 PNG 생성 → `Art/import_assets.py`로 텍스처·스프라이트·몹 데이터·스폰 테이블·테스트 맵 생성/갱신 |
| `RunTests.ps1` | `YJJJump.*` 자동화 테스트(로프 반동, 판정, 결정성, 전투)를 헤드리스로 실행 |
| `RunSmoke.ps1` | 게임을 창 모드로 띄워 오토플레이 봇이 플레이 → 로그 요약 + `Saved/Screenshots/AutoPlay_*.png` |

## 콘솔 변수
- `yjj.AutoPlay 1` — 봇이 드래그 발사와 퍼펙트 타이밍을 대신 입력 (PIE에서도 사용 가능)
- `yjj.AutoPlayDuration N` — N초 플레이 후 스크린샷 3장을 남기고 종료

## 아트 수정
`Art/generate_art.py`의 `make_*` 함수를 고치고 `BuildArt.ps1`을 다시 실행하면 에셋이 갱신된다.
몹 수치는 `import_assets.py`의 `MOBS`, 스폰 비율은 `SKY_SPAWNS`.
