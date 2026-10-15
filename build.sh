#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
CC="${CC:-x86_64-w64-mingw32-gcc}"
mkdir -p dist
"$CC" src/garden.c -o dist/FlowerGarden.exe -std=c11 -Os -Wall -Wextra -Werror -municode -mwindows -static -s -lgdi32 -luser32 -lm
