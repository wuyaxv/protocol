#!/bin/bash
#
# protocol - A simple header generator for network protocols (C++ edition)
#
# Regression tests: compares the ASCII output of the C++ rewrite against
# golden files produced by the original Python implementation, and performs
# sanity checks on the SVG/PNG renderers.
#
# Usage: tests/run_tests.sh [path-to-binary]
#
set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BIN="${1:-$ROOT_DIR/build/protocol}"
GOLDEN_DIR="$SCRIPT_DIR/golden"

if [ ! -x "$BIN" ]; then
    echo "error: binary not found at '$BIN' (build first: cmake -B build && cmake --build build)"
    exit 1
fi

pass=0
fail=0

check_ascii() {
    local name="$1"; shift
    local expected="$GOLDEN_DIR/$name.txt"
    local actual
    actual="$("$BIN" "$@")"
    if [ "$actual" == "$(cat "$expected")" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL: $name ($*)"
        diff <(cat "$expected") <(printf '%s' "$actual") | head -10
    fi
}

# Golden ASCII outputs (generated with the original Python tool).
for f in "$GOLDEN_DIR"/*.txt; do
    name="$(basename "$f" .txt)"
    case "$name" in
        __multi_tcp_udp)   check_ascii "$name" tcp udp ;;
        __bits16_tcp)      check_ascii "$name" tcp -b 16 ;;
        __nonumbers_ip)    check_ascii "$name" ip -n ;;
        __custom_chars)    check_ascii "$name" 'A:8,B:8?bits=16,oddchar=.,evenchar=-,startchar=<,endchar=>,sepchar=!' ;;
        __custom_spec)     check_ascii "$name" 'Type:8,Code:8,Checksum:16' ;;
        __spanning)        check_ascii "$name" 'A:24,B:48,C:8' ;;
        __big96)           check_ascii "$name" 'F:96?bits=32' ;;
        __unusual)         check_ascii "$name" 'X:33,Y:17,Z:40' ;;
        *)                 check_ascii "$name" "$name" ;;
    esac
done

# SVG sanity: well-formed XML containing svg root.
svg_out="$("$BIN" tcp ip --format svg)"
if printf '%s' "$svg_out" | head -1 | grep -q '<svg ' && printf '%s' "$svg_out" | grep -q '</svg>'; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: svg output sanity"
fi

# PNG sanity: magic bytes.
png_out="$("$BIN" udp --format png | head -c 8 | od -An -tx1 | tr -d ' \n')"
if [ "$png_out" == "89504e470d0a1a0a" ]; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: png magic bytes (got $png_out)"
fi

# Error handling: invalid specs must fail with exit code 1.
for bad in 'A:0' 'A:x' 'A:8?bits=-3' 'A:8?a?b' 'no-such-protocol'; do
    if "$BIN" "$bad" >/dev/null 2>&1; then
        fail=$((fail + 1))
        echo "FAIL: expected error for spec '$bad'"
    else
        pass=$((pass + 1))
    fi
done

echo ""
echo "passed: $pass, failed: $fail"
[ "$fail" -eq 0 ]
