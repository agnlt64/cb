#!/usr/bin/env bash
set -euo pipefail

A="${1:?usage: match.sh <engine_A> <engine_B> [games] [tc]}"
B="${2:?}"
GAMES="${3:-200}"
TC="${4:-10+0.1}"
LOG="matches/$(basename $A)-vs-$(basename $B)-$(date +%Y%m%d-%H%M%S).log"

OUT="matches/$(basename $A)-vs-$(basename $B)-$(date +%Y%m%d-%H%M%S).pgn"

./cutechess-cli \
    -engine cmd="$A" name="$(basename $A .bin)" \
    -engine cmd="$B" name="$(basename $B .bin)" \
    -each proto=uci tc="$TC" \
    -openings file=book/UHO_4060_v2.epd format=epd order=random \
    -games "$GAMES" -rounds 1 -repeat \
    -concurrency 4 \
    -pgnout "$OUT" \
    -ratinginterval 20 \
    2>&1 | tee "$LOG"

FINAL=$(grep "Elo difference" "$LOG" | tail -1)
TAG_A=$(basename "$A" .bin)
TAG_B=$(basename "$B" .bin)
echo "$(date -u +%Y-%m-%dT%H:%M:%SZ)  $TAG_A  vs  $TAG_B  TC=$TC  games=$GAMES  $FINAL" \
    >> ./history.tsv