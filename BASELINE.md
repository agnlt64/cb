# Engine evolution overtime
All tests were executed on my M5 Macbook Pro (macOS Tahoe 26.5), unless other hardware is mentioned. Games were played using `cutechess-cli` version 1.4.0 built from [source](https://github.com/cutechess/cutechess).

## Baseline 2026-05-31

- commit `ac8e7a0`
- TC: 10+0.1
- book: `8moves_v3.pgn`
- 400 games self-play
- result: Elo difference: 3.5 +/- 33.2, LOS: 58.1 %, DrawRatio: 5.0 %
- illegal moves: 0
- timeout: 197 total
- concurrency: 4 games at a time

## Baseline 2026-05-31

- commit `5278ec1`
- TC: 10+0.1
- book: `8moves_v3.pgn`
- 200 games self-play
- result: Elo difference: -1.7 +/- 20.1, LOS: 43.3 %, DrawRatio: 82.5 %
- illegal moves: 0
- timeout: 8 total
- concurrency: 4 games at a time
