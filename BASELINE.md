# Engine evolution overtime
All tests were executed on my M5 Macbook Pro (macOS Tahoe 26.5), unless other hardware is mentioned. Games were played using `cutechess-cli` version 1.4.0 built from [source](https://github.com/cutechess/cutechess).

## Baseline 2026-05-31

- commit `ac8e7a0`
- TC: 10+0.1
- book: `8moves_v3.pgn`
- 400 games self-play
- result: Elo difference: 3.5 +/- 33.2, LOS: 58.1 %, DrawRatio: 5.0 %
- timeout: 197 total
- concurrency: 4 games at a time

## Baseline 2026-05-31

- commit `5278ec1`
- TC: 10+0.1
- book: `8moves_v3.pgn`
- 200 games self-play
- result: Elo difference: -1.7 +/- 20.1, LOS: 43.3 %, DrawRatio: 82.5 %
- timeout: 8 total
- concurrency: 4 games at a time

## Baseline 2026-05-31

- commit `3527361`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 200 games self-play
- result: Elo difference: 27.9 +/- 20.2, LOS: 99.6 %, DrawRatio: 82.0 %
- timeout: 6 total
- concurrency: 4 games at a time

**note**: 27.9 Elo bias is most-likely caused by `-concurrency 4` in cutechess.

## Baseline 2026-06-01

- commit `3527361`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 50 games self-play
- result: Elo difference: -49.0 +/- 48.4, LOS: 2.6 %, DrawRatio: 74.0 %
- timeout: 0 total
- concurrency: 1 game at a time
