#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

COMPILER=""
OPT_FLAG=""

for arg in "$@"; do
    case "$arg" in
        -O*|--opt-level=*)
            if [[ "$arg" == --opt-level=* ]]; then
                OPT_FLAG="-O${arg#*=}"
            else
                OPT_FLAG="$arg"
            fi
            ;;
        *)
            if [ -z "$COMPILER" ]; then
                COMPILER="$arg"
            fi
            ;;
    esac
done

if [ -z "$COMPILER" ]; then
    COMPILER="../chibicc"
fi
ROOT_DIR="$(cd .. && pwd)"
INCLUDE_FLAGS="-I$ROOT_DIR -I$ROOT_DIR/include -I$ROOT_DIR/compiler_include"

echo "=========================================="
echo "       C11 Conformance Test Suite         "
echo "=========================================="
echo "Compiler : $COMPILER"
if [ -n "$OPT_FLAG" ]; then
    echo "Opt Level: $OPT_FLAG"
fi

PASSED=0
FAILED=0
FAILED_LIST=()

# Positive tests
for cat_dir in $(find . -maxdepth 1 -mindepth 1 -type d ! -name "negative" | sort); do
    cat_name=$(basename "$cat_dir")
    echo ""
    echo "[$cat_name]"
    for test_file in $(find "$cat_dir" -maxdepth 1 -name "*.c" | sort); do
        test_name="$cat_name/$(basename "$test_file")"
        obj_file="${test_file%.c}.o"
        exe_file="${test_file%.c}.exe"

        EXTRA_LINK_FLAGS=""
        if [[ "$test_name" =~ thread_local|atomic ]]; then
            EXTRA_LINK_FLAGS="-lpthread"
        fi

        if "$COMPILER" -c "$test_file" -o "$obj_file" $INCLUDE_FLAGS $OPT_FLAG 2>/dev/null; then
            if gcc "$obj_file" -o "$exe_file" $EXTRA_LINK_FLAGS 2>/dev/null; then
                if "$exe_file" >/dev/null 2>&1; then
                    echo "  [PASS] $test_name"
                    PASSED=$((PASSED + 1))
                else
                    echo "  [FAIL] $test_name (Execution failed)"
                    FAILED=$((FAILED + 1))
                    FAILED_LIST+=("$test_name (exec)")
                fi
            else
                echo "  [FAIL] $test_name (Link failed)"
                FAILED=$((FAILED + 1))
                FAILED_LIST+=("$test_name (link)")
            fi
        else
            echo "  [FAIL] $test_name (Compilation failed)"
            FAILED=$((FAILED + 1))
            FAILED_LIST+=("$test_name (compile)")
        fi
        rm -f "$obj_file" "$exe_file"
    done
done

# Negative tests
echo ""
echo "[negative (compile rejection tests)]"
for test_file in $(find negative -maxdepth 1 -name "*.c" | sort); do
    test_name="negative/$(basename "$test_file")"
    obj_file="${test_file%.c}.o"
    if "$COMPILER" -c "$test_file" -o "$obj_file" $INCLUDE_FLAGS $OPT_FLAG 2>/dev/null; then
        echo "  [FAIL] $test_name (Incorrectly accepted invalid C11 code)"
        FAILED=$((FAILED + 1))
        FAILED_LIST+=("$test_name (should fail compile)")
        rm -f "$obj_file"
    else
        echo "  [PASS] $test_name (Correctly rejected)"
        PASSED=$((PASSED + 1))
    fi
done

echo ""
echo "=========================================="
echo "C11 Conformance Suite Results"
echo "Passed: $PASSED"
if [ $FAILED -gt 0 ]; then
    echo "Failed: $FAILED"
    for f in "${FAILED_LIST[@]}"; do
        echo "  - $f"
    done
    exit 1
else
    echo "All tests passed successfully!"
    exit 0
fi
