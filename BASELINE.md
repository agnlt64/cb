# Engine evolution overtime
All tests were executed on my M5 Macbook Pro (macOS Tahoe 26.5), unless other hardware is mentioned. Games were played using `cutechess-cli` version 1.4.0 built from [source](https://github.com/cutechess/cutechess).

## Baseline 2026-06-04

- commit `cf5790f`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 200 games previous version
- result: Elo difference: -5.2 +/- 44.7, LOS: 40.9 %, DrawRatio: 14.5 %
- timeout: 0 total
- concurrency: 4 games at a time

## Baseline 2026-06-04

- commit `206567c`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 200 games vs Stockfish Skill Level 2
- result: Elo difference: -31.4 +/- 47.9, LOS: 9.8 %, DrawRatio: 3.0 %
- timeout: 0 total
- concurrency: 4 games at a time

## Baseline 2026-06-04

- commit `206567c`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 200 games vs Stockfish Skill Level 1
- result: Elo difference: 70.4 +/- 49.4, LOS: 99.8 %, DrawRatio: 0.0 %
- timeout: 0 total
- concurrency: 4 games at a time

## Baseline 2026-06-04

- commit `206567c`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 200 games vs previous version
- result: Elo difference: -284.9 +/- 63.4, LOS: 0.0 %, DrawRatio: 5.5 %
- timeout: 0 total
- concurrency: 4 games at a time

**note**: -284 means that previous version is -284 Elo below current version

## Baseline 2026-06-03

- commit `8fc1b0e`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 200 games vs Stockfish Skill Level 1
- result: Elo difference: -15.6 +/- 47.6, LOS: 25.9 %, DrawRatio: 3.5 %
- timeout: 0 total
- concurrency: 4 games at a time

## Baseline 2026-06-03

- commit `0ca4f51`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 200 games vs Stockfish Skill Level 0
- result: Elo difference: 181.7 +/- 53.7, LOS: 100.0 %, DrawRatio: 5.0 %
- timeout: 0 total
- concurrency: 4 games at a time

## Baseline 2026-06-03

- commit `3d5c556`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 200 games self-play
- result: Elo difference: -inf +/- nan, LOS: 0.0 %, DrawRatio: 0.0 %
- timeout: 0 total
- concurrency: 4 games at a time

**note**: this match doesn't reflect anything, previous versions of the engine had a bug in the search function.

## Baseline 2026-06-03

- commit `8e064c7`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 200 games self-play
- result: Elo difference: -1.7 +/- 21.8, LOS: 43.8 %, DrawRatio: 79.5 %
- timeout: 2 total
- concurrency: 4 games at a time

## Baseline 2026-06-01

- commit `d521caf`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 200 games self-play
- result: Elo difference: 0.0 +/- 19.3, LOS: 50.0 %, DrawRatio: 84.0 %
- timeout: 7 total
- concurrency: 4 games at a time

## Baseline 2026-06-01

- commit `825753f`
- TC: depth 6, unlimited time
- book: `UHO_4060_v2.epd`
- 50 games self-play
- result: Elo difference: 0.0 +/- 83.2, LOS: 50.0 %, DrawRatio: 28.0 %
- timeout: 0 total
- concurrency: 4 games at a time

**note**: as we can see, the bot is perfectly symmetrical. previously observed Elo biases were just noise.

## Baseline 2026-06-01

- commit `3527361`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 50 games self-play
- result: Elo difference: -49.0 +/- 48.4, LOS: 2.6 %, DrawRatio: 74.0 %
- timeout: 0 total
- concurrency: 1 game at a time

## Baseline 2026-05-31

- commit `3527361`
- TC: 10+0.1
- book: `UHO_4060_v2.epd`
- 200 games self-play
- result: Elo difference: 27.9 +/- 20.2, LOS: 99.6 %, DrawRatio: 82.0 %
- timeout: 6 total
- concurrency: 4 games at a time

**note**: 27.9 Elo bias is most-likely caused by `-concurrency 4` in cutechess.

## Baseline 2026-05-31

- commit `5278ec1`
- TC: 10+0.1
- book: `8moves_v3.pgn`
- 200 games self-play
- result: Elo difference: -1.7 +/- 20.1, LOS: 43.3 %, DrawRatio: 82.5 %
- timeout: 8 total
- concurrency: 4 games at a time

## Baseline 2026-05-31

- commit `ac8e7a0`
- TC: 10+0.1
- book: `8moves_v3.pgn`
- 400 games self-play
- result: Elo difference: 3.5 +/- 33.2, LOS: 58.1 %, DrawRatio: 5.0 %
- timeout: 197 total
- concurrency: 4 games at a time
