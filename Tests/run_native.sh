#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
compiler=${CXX:-clang++}
out=${TEST_OUTPUT:-evidence/ci}
mkdir -p "$out"
"$compiler" --version | tee "$out/compiler.txt"
flags=(-pthread -std=c++17 -O2 -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
if [[ "${SANITIZE:-0}" == 1 ]]; then
  flags+=(-O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
fi
for src in NativeTests/test_*.cpp Tests/test_host_*.cpp Tests/test_s8_*.cpp; do
  name=$(basename "$src" .cpp)
  test_flags=("${flags[@]}")
  # SOC-06 diagnostic: get a precise ASan/UBSan trace for the MEMORY crash
  # without changing the historic test's behavior.
  if [[ "$name" == test_host_memory && "${SANITIZE:-0}" != 1 ]]; then
    test_flags+=(-O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
  fi
  "$compiler" "${test_flags[@]}" "$src" -o "$out/$name" >"$out/$name.compile.log" 2>&1
  "$out/$name" >"$out/$name.run.log" 2>&1
  tail -1 "$out/$name.run.log"
done
# Preserve and execute V0.1's C transform tests as a separate regression.
${CC:-clang} -std=c11 -O2 -UNDEBUG -Wall -Wextra -Werror -I Shared Tests/test_transform.c Shared/XoneMidiTransform.c -o "$out/test_transform"
"$out/test_transform" | tee "$out/test_transform.run.log"
