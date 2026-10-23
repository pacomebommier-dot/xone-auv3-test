#!/bin/sh
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cc -std=c11 -Wall -Wextra -Werror -I "$ROOT/Shared" "$ROOT/Shared/XoneMidiTransform.c" "$ROOT/Tests/test_transform.c" -o /tmp/xone_engine_test_c
/tmp/xone_engine_test_c
