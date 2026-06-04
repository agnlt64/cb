#!/usr/bin/env bash
set -euo pipefail

A="${1:?}"
B="${2:?}"
TC="${3:-10+0.1}"
LOG="matches/$(basename $A)-vs-$(basename $B)-$(date +%Y%m%d-%H%M%S).log"

./cutechess-cli \
    -engine cmd="$A" name="A" \
    -engine cmd="$B" name="B" \
    -each proto=uci tc="$TC" \
    -openings file=book/8moves_v3.pgn format=pgn order=random \
    -games 40000 -rounds 1 -repeat \
    -concurrency 4 \
    -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 \
    -pgnout "matches/sprt-$(date +%Y%m%d-%H%M%S).pgn" \
    -ratinginterval 20 \
    2>&1 | tee "$LOG"

FINAL=$(grep "Elo difference" "$LOG" | tail -1)
TAG_A=$(basename "$A" .bin)
TAG_B=$(basename "$B" .bin)
echo "$(date -u +%Y-%m-%dT%H:%M:%SZ)  $TAG_A  vs  $TAG_B  TC=$TC  games=$GAMES  $FINAL" \
    >> ./history.tsv