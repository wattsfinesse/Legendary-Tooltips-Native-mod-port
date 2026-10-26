#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DIST_DIR="${SCRIPT_DIR}/dist"
LEVILAMROID_ROOT="${LEVILAMROID_ROOT:-$(cd "${SCRIPT_DIR}/../LeviLamroid-main" 2>/dev/null && pwd || true)}"

if [[ -z "${LEVILAMROID_ROOT}" || ! -f "${LEVILAMROID_ROOT}/include/levilamroid/LeviLamroid.h" ]]; then
  echo "Set LEVILAMROID_ROOT to the extracted LeviLamroid source tree." >&2
  exit 2
fi

CMAKE_ARGS=(-DLEVILAMROID_ROOT="${LEVILAMROID_ROOT}")
if [[ -n "${ANDROID_NDK_HOME:-}" && -f "${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" ]]; then
  CMAKE_ARGS+=( -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 )
fi
if [[ -n "${CMAKE_TOOLCHAIN_FILE:-}" ]]; then
  CMAKE_ARGS+=( -DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}" )
fi

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" "${CMAKE_ARGS[@]}" 
cmake --build "${BUILD_DIR}" --parallel

mkdir -p "${DIST_DIR}"
cp "${BUILD_DIR}/liblegendary_tooltips_native.so" "${DIST_DIR}/"
cp "${SCRIPT_DIR}/manifest.json" "${DIST_DIR}/"
cp -R "${SCRIPT_DIR}/config" "${DIST_DIR}/"
cp "${SCRIPT_DIR}/README.md" "${DIST_DIR}/"

python3 - "${DIST_DIR}" <<'PY'
import pathlib, sys, zipfile
root = pathlib.Path(sys.argv[1])
manifest = root / 'manifest.json'
lib = root / 'liblegendary_tooltips_native.so'
if not manifest.is_file() or not lib.is_file():
    raise SystemExit('Required artifact missing; refusing to make a fake .levipack')
out = root / 'LegendaryTooltipsNative-0.1.0.levipack'
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
    z.write(manifest, 'manifest.json')
    z.write(lib, lib.name)
    for p in (root / 'config').rglob('*'):
        if p.is_file(): z.write(p, p.relative_to(root).as_posix())
    z.write(root / 'README.md', 'README.md')
print(f'Created {out}')
PY
