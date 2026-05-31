#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

if [[ -n "$(git status --porcelain)" ]]; then
    echo "❌ working tree dirty - commit before archiving"
    exit 1
fi

TAG="${1:?usage: archive.sh <tag>}"
HASH=$(git rev-parse --short HEAD)
DATE=$(date +%Y%m%d)
DEST="testing/builds/cb-${TAG}-${HASH}-${DATE}.bin"

make clean >/dev/null
make release >/dev/null
cp ./bin/uci_o3_release "$DEST"
echo "✅ archived $DEST"
