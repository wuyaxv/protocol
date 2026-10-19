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
        # --- New features (golden files verified by hand, C++ edition) ---
        __varlen_basic)        check_ascii "$name" 'Payload:32~' ;;
        __varlen_mid)          check_ascii "$name" 'Type:8,Data:16~,Code:8' ;;
        __varlen_wrap)         check_ascii "$name" 'A:24,B:48~,C:8' ;;
        __varlen_big)          check_ascii "$name" 'Header:16,Big:96~,Tail:16' ;;
        __varlen_star)         check_ascii "$name" 'Payload:32*' ;;
        __escape_comma)        check_ascii "$name" 'Seq\, Nr:8,Code:8' ;;
        __escape_colon_question) check_ascii "$name" 'What\:ify\?:8,End:16' ;;
        __escape_backslash)    check_ascii "$name" 'Back\\slash:16,End:16' ;;
        __escape_sepchar)      check_ascii "$name" 'A:8,B:8?bits=16,sepchar=\,' ;;
        __pointer_row)         check_ascii "$name" tcp --pointer 'Pointer@3' ;;
        __pointer_field)       check_ascii "$name" udp --pointer 'Len@length' ;;
        __pointer_elbow)       check_ascii "$name" udp --pointer 'SP@Source Port' --pointer 'Ck@Checksum' ;;
        __pointer_elbow_mixed) check_ascii "$name" tcp --pointer 'Top@1' --pointer 'Fl@Flags' ;;
        __pointer_clamp)       check_ascii "$name" udp --pointer 'End@99' ;;
        __pointer_merge)       check_ascii "$name" udp --pointer 'A@1' --pointer 'B@1' --pointer 'C@2' ;;
        __pointer_specopt)     check_ascii "$name" 'Type:8,Data:16,Code:8?pointer=Ptr@Data' ;;
        __pointer_nonumbers)   check_ascii "$name" tcp -n --pointer 'Seq@3' ;;
        __combo_escape_varlen_pointer) check_ascii "$name" 'Hdr\, v1:8,Payload:32~,CRC:16' --pointer 'P@Payload' ;;
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

# SVG sanity with new features: dashed edges and pointer labels.
svg_varlen="$("$BIN" 'Payload:64~' --format svg)"
if printf '%s' "$svg_varlen" | grep -q 'stroke-dasharray'; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: svg variable-length dashed edges"
fi
# Dashed edges use a gap larger than the dash (dash cell_h/6, gap cell_h/4).
dash_pair="$(printf '%s' "$svg_varlen" | grep -o 'stroke-dasharray="[0-9.]* [0-9.]*"' | head -1)"
dash_len="${dash_pair#*\"}"; dash_len="${dash_len%% *}"
gap_len="${dash_pair##* }"; gap_len="${gap_len%\"}"
if [ -n "$dash_len" ] && [ -n "$gap_len" ] && \
   [ "$(awk "BEGIN{print ($gap_len > $dash_len) ? 1 : 0}")" == "1" ]; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: dashed gap should exceed dash length (got '$dash_pair')"
fi
svg_ptr="$("$BIN" tcp --format svg --pointer 'Pointer@3')"
if printf '%s' "$svg_ptr" | grep -q '>Pointer<' && printf '%s' "$svg_ptr" | grep -q '</svg>'; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: svg pointer rendering"
fi
# Elbow arrows in SVG: arrowhead '^' and corner '+' rendered as pointer glyphs.
svg_elbow="$("$BIN" udp --format svg --pointer 'Ck@Checksum')"
if printf '%s' "$svg_elbow" | grep -q '>\^<' && printf '%s' "$svg_elbow" | grep -q '>Ck<'; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: svg elbow pointer rendering"
fi

# SVG ASCII mode: borders are emitted as text characters, not line rects.
svg_ascii="$("$BIN" udp --format svg --ascii)"
if printf '%s' "$svg_ascii" | grep -q '>[-+]*<' && \
   ! printf '%s' "$svg_ascii" | grep -q 'stroke-dasharray'; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: svg ascii mode"
fi

# SVG with --no-fill and --transparent and --ascii: no <rect> elements at all.
svg_nofill="$("$BIN" udp --format svg --no-fill --transparent --ascii)"
if printf '%s' "$svg_nofill" | grep -q '<rect'; then
    fail=$((fail + 1))
    echo "FAIL: svg --no-fill --transparent --ascii should have no <rect>"
else
    pass=$((pass + 1))
fi

# SVG default font stack is modern; --font-family is prepended.
svg_ff="$("$BIN" udp --format svg --font-family 'My Mono')"
if printf '%s' "$svg_ff" | grep -q "font-family=\"'My Mono','JetBrains Mono'"; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: svg font stack"
fi

# --fg recolors all ink; --bg recolors the canvas; hex and names both work.
svg_fg="$("$BIN" udp --format svg --transparent --fg white)"
if printf '%s' "$svg_fg" | grep -q 'fill="#ffffff"' && \
   ! printf '%s' "$svg_fg" | grep -q '<rect x="0"'; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: --fg white with --transparent"
fi
svg_hex="$("$BIN" udp --format svg --fg '#00ff88' --bg '#101010')"
if printf '%s' "$svg_hex" | grep -q 'fill="#00ff88"' && \
   printf '%s' "$svg_hex" | grep -q 'fill="#101010"'; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: --fg/--bg hex colors"
fi
png_fg="$("$BIN" udp --format png --transparent --fg white --no-fill | head -c 8 | od -An -tx1 | tr -d ' \n')"
if [ "$png_fg" == "89504e470d0a1a0a" ]; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: png --fg white (got $png_fg)"
fi
ansi_fg="$("$BIN" udp --fg lime --color always)"
if printf '%s' "$ansi_fg" | grep -q '38;2;0;255;0'; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: ansi --fg lime"
fi
if "$BIN" udp --fg notacolor >/dev/null 2>&1; then
    fail=$((fail + 1))
    echo "FAIL: --fg with unknown color must fail"
else
    pass=$((pass + 1))
fi
if "$BIN" --list-colors | grep -q white; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: --list-colors"
fi

# ANSI text output with --no-fill must not emit background colors.
ansi_nofill="$("$BIN" udp --theme dark --color always --no-fill)"
if printf '%s' "$ansi_nofill" | grep -q '48;2;'; then
    fail=$((fail + 1))
    echo "FAIL: ansi --no-fill should have no background colors"
else
    pass=$((pass + 1))
fi

# PNG sanity: magic bytes.
png_out="$("$BIN" udp --format png | head -c 8 | od -An -tx1 | tr -d ' \n')"
if [ "$png_out" == "89504e470d0a1a0a" ]; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: png magic bytes (got $png_out)"
fi

# PNG sanity with pointers and variable-length fields enabled.
png_feat="$("$BIN" 'Type:8,Payload:32~,CRC:16' --format png --pointer 'P@Payload' | head -c 8 | od -An -tx1 | tr -d ' \n')"
if [ "$png_feat" == "89504e470d0a1a0a" ]; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: png with pointer/varlen (got $png_feat)"
fi

# PNG ASCII mode (terminal look) with no-fill.
png_ascii="$("$BIN" udp --format png --ascii --no-fill | head -c 8 | od -An -tx1 | tr -d ' \n')"
if [ "$png_ascii" == "89504e470d0a1a0a" ]; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL: png ascii mode (got $png_ascii)"
fi

# An explicit missing font file must fail; a real one must work (if present).
if "$BIN" udp --font /nonexistent/font.ttf --format png >/dev/null 2>&1; then
    fail=$((fail + 1))
    echo "FAIL: --font with a missing file must fail"
else
    pass=$((pass + 1))
fi
SYSFONT=""
for c in /System/Library/Fonts/Menlo.ttc /System/Library/Fonts/SFNSMono.ttf \
         /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf \
         "C:/Windows/Fonts/consola.ttf"; do
    [ -f "$c" ] && SYSFONT="$c" && break
done
if [ -n "$SYSFONT" ]; then
    png_ttf="$("$BIN" udp --format png --font "$SYSFONT" | head -c 8 | od -An -tx1 | tr -d ' \n')"
    if [ "$png_ttf" == "89504e470d0a1a0a" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL: png with --font $SYSFONT"
    fi
else
    echo "SKIP: no known system font found for the --font test"
fi

# Pointer clamping emits a warning on stderr but succeeds.
if "$BIN" udp --pointer 'End@99' >/dev/null 2>&1; then
    warn="$("$BIN" udp --pointer 'End@99' 2>&1 >/dev/null)"
    if printf '%s' "$warn" | grep -q 'clamped'; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL: pointer clamping warning missing"
    fi
else
    fail=$((fail + 1))
    echo "FAIL: pointer clamping should not fail"
fi

# Error handling: invalid specs must fail with exit code 1.
for bad in 'A:0' 'A:x' 'A:8?bits=-3' 'A:8?a?b' 'no-such-protocol' 'A,B:8'; do
    if "$BIN" "$bad" >/dev/null 2>&1; then
        fail=$((fail + 1))
        echo "FAIL: expected error for spec '$bad'"
    else
        pass=$((pass + 1))
    fi
done

# Error handling: invalid variable-length / pointer syntax must fail.
if "$BIN" 'A:~' >/dev/null 2>&1; then fail=$((fail + 1)); echo "FAIL: 'A:~' must fail"; else pass=$((pass + 1)); fi
if "$BIN" 'A:x~' >/dev/null 2>&1; then fail=$((fail + 1)); echo "FAIL: 'A:x~' must fail"; else pass=$((pass + 1)); fi
if "$BIN" tcp --pointer 'X@Nope' >/dev/null 2>&1; then fail=$((fail + 1)); echo "FAIL: unknown pointer field must fail"; else pass=$((pass + 1)); fi
if "$BIN" tcp --pointer 'bad' >/dev/null 2>&1; then fail=$((fail + 1)); echo "FAIL: malformed pointer must fail"; else pass=$((pass + 1)); fi
if "$BIN" tcp --pointer 'A@' >/dev/null 2>&1; then fail=$((fail + 1)); echo "FAIL: empty pointer target must fail"; else pass=$((pass + 1)); fi

echo ""
echo "passed: $pass, failed: $fail"
[ "$fail" -eq 0 ]
