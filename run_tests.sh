#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

COMPILER="${1:-./chibicc}"
if [ ! -f "$COMPILER" ]; then
    if [ -f "./cmake-build-debug/chibicc-cli.exe" ]; then
        COMPILER="./cmake-build-debug/chibicc-cli.exe"
    elif [ -f "./chibicc.exe" ]; then
        COMPILER="./chibicc.exe"
    elif [ -f "./chibicc" ]; then
        COMPILER="./chibicc"
    fi
fi

INCLUDE_FLAGS="-I. -Iinclude -Icompiler_include -Itests"

echo "========================================"
echo "       chibicc Test Suite Runner        "
echo "========================================"
echo "Compiler: $COMPILER"

PASSED=0
FAILED=0
FAILED_LIST=()

# ==============================================================================
# 1. Unit & Extension Tests (in tests/*.c)
# ==============================================================================
echo ""
echo "[Setup] Compiling tests/common helper..."
gcc -x c -c "tests/common" -o "tests/common.o"

echo ""
echo "========================================"
echo "Section 1: Unit & Extension Tests"
echo "========================================"

for c_file in $(find tests -maxdepth 1 -name "*.c" ! -name "abi_*" | sort); do
    test_name=$(basename "$c_file" .c)
    obj_file="tests/$test_name.o"
    exe_file="tests/$test_name.exe"

    EXTRA_LINK_FLAGS=""
    if [ "$test_name" = "atomic" ]; then
        EXTRA_LINK_FLAGS="-lpthread"
    fi
    if [ "$test_name" = "win_test" ]; then
        EXTRA_LINK_FLAGS="-lkernel32"
    fi

    if "$COMPILER" -c "$c_file" -o "$obj_file" $INCLUDE_FLAGS -D_WIN32 -D__GNUC__ 2>/dev/null; then
        if gcc "$obj_file" "tests/common.o" -o "$exe_file" $EXTRA_LINK_FLAGS 2>/dev/null; then
            if "./$exe_file" >/dev/null 2>&1; then
                echo "  [PASS] $test_name.c"
                PASSED=$((PASSED + 1))
            else
                echo "  [FAIL] $test_name.c (Execution failed)"
                FAILED=$((FAILED + 1))
                FAILED_LIST+=("unit/$test_name.c (exec)")
            fi
        else
            echo "  [FAIL] $test_name.c (Link failed)"
            FAILED=$((FAILED + 1))
            FAILED_LIST+=("unit/$test_name.c (link)")
        fi
    else
        echo "  [FAIL] $test_name.c (Compilation failed)"
        FAILED=$((FAILED + 1))
        FAILED_LIST+=("unit/$test_name.c (compile)")
    fi
    rm -f "$obj_file" "$exe_file"
done

rm -f "tests/common.o"

# ==============================================================================
# 2. ABI Test Suites
# ==============================================================================
echo ""
echo "========================================"
echo "Section 2: ABI Test Suites"
echo "========================================"

for c_file in $(find tests -maxdepth 1 -name "abi_test_all.c" | sort); do
    test_name=$(basename "$c_file" .c)
    obj_file="tests/$test_name.o"
    exe_file="tests/$test_name.exe"

    if "$COMPILER" -c "$c_file" -o "$obj_file" -Iinclude -D_WIN32 -D__GNUC__ 2>/dev/null; then
        if gcc -o "$exe_file" "$obj_file" 2>/dev/null; then
            if "./$exe_file" >/dev/null 2>&1; then
                echo "  [PASS] $test_name.c"
                PASSED=$((PASSED + 1))
            else
                echo "  [FAIL] $test_name.c (Execution failed)"
                FAILED=$((FAILED + 1))
                FAILED_LIST+=("abi/$test_name.c (exec)")
            fi
        else
            echo "  [FAIL] $test_name.c (Link failed)"
            FAILED=$((FAILED + 1))
            FAILED_LIST+=("abi/$test_name.c (link)")
        fi
    else
        echo "  [FAIL] $test_name.c (Compilation failed)"
        FAILED=$((FAILED + 1))
        FAILED_LIST+=("abi/$test_name.c (compile)")
    fi
    rm -f "$obj_file" "$exe_file"
done

# ==============================================================================
# 3. C11 Conformance Test Suite
# ==============================================================================
echo ""
echo "========================================"
echo "Section 3: C11 Conformance Test Suite"
echo "========================================"

CONF_DIR="tests/conformance-tests"
if [ -d "$CONF_DIR" ]; then
    # Positive tests
    for cat_dir in $(find "$CONF_DIR" -maxdepth 1 -mindepth 1 -type d ! -name "negative" | sort); do
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

            if "$COMPILER" -c "$test_file" -o "$obj_file" $INCLUDE_FLAGS -D_WIN32 -D__GNUC__ 2>/dev/null; then
                if gcc "$obj_file" -o "$exe_file" $EXTRA_LINK_FLAGS 2>/dev/null; then
                    if "$exe_file" >/dev/null 2>&1; then
                        echo "  [PASS] $test_name"
                        PASSED=$((PASSED + 1))
                    else
                        echo "  [FAIL] $test_name (Execution failed)"
                        FAILED=$((FAILED + 1))
                        FAILED_LIST+=("conformance/$test_name (exec)")
                    fi
                else
                    echo "  [FAIL] $test_name (Link failed)"
                    FAILED=$((FAILED + 1))
                    FAILED_LIST+=("conformance/$test_name (link)")
                fi
            else
                echo "  [FAIL] $test_name (Compilation failed)"
                FAILED=$((FAILED + 1))
                FAILED_LIST+=("conformance/$test_name (compile)")
            fi
            rm -f "$obj_file" "$exe_file"
        done
    done

    # Negative tests
    echo ""
    echo "[negative (compile rejection tests)]"
    for test_file in $(find "$CONF_DIR/negative" -maxdepth 1 -name "*.c" | sort); do
        test_name="negative/$(basename "$test_file")"
        obj_file="${test_file%.c}.o"
        if "$COMPILER" -c "$test_file" -o "$obj_file" $INCLUDE_FLAGS -D_WIN32 -D__GNUC__ 2>/dev/null; then
            echo "  [FAIL] $test_name (Incorrectly accepted invalid C11 code)"
            FAILED=$((FAILED + 1))
            FAILED_LIST+=("conformance/$test_name (should fail compile)")
            rm -f "$obj_file"
        else
            echo "  [PASS] $test_name (Correctly rejected)"
            PASSED=$((PASSED + 1))
        fi
    done
fi

# ==============================================================================
# 4. Optimization Passes Test Suite
# ==============================================================================
echo ""
echo "========================================"
echo "Section 4: Optimization Passes Tests"
echo "========================================"

# 4.1 Internal compiler optimization tests (handcrafted IR)
echo ""
echo "[Internal Optimization Tests]"
HANDCRAFTED_EXE="./cmake-build-debug/hlir-opt-test.exe"
if [ ! -f "$HANDCRAFTED_EXE" ]; then
    HANDCRAFTED_EXE="./hlir-opt-test"
fi

if [ -f "$HANDCRAFTED_EXE" ]; then
    if "$HANDCRAFTED_EXE"; then
        echo "  [PASS] hlir-opt-test"
        PASSED=$((PASSED + 1))
    else
        echo "  [FAIL] hlir-opt-test (Execution failed)"
        FAILED=$((FAILED + 1))
        FAILED_LIST+=("optimizations/hlir-opt-test (internal)")
    fi
else
    echo "  [SKIP] hlir-opt-test (Not found)"
fi

# 4.2 Integration tests compiled with chibicc
OPT_DIR="tests/optimizations"
if [ -d "$OPT_DIR" ]; then
    echo ""
    echo "[Integration Optimization Tests]"
    
    # Setup common helper
    gcc -x c -c "tests/common" -o "tests/common.o"
    
    for c_file in $(find "$OPT_DIR" -maxdepth 1 -name "*.c" ! -name "hlir_opt_handcrafted.c" | sort); do
        test_name=$(basename "$c_file" .c)
        obj_file="$OPT_DIR/$test_name.o"
        exe_file="$OPT_DIR/$test_name.exe"
        
        for opt_level in -O1 -O2 -O3 -Os; do
            test_label="optimizations/$test_name.c ($opt_level)"
            if "$COMPILER" -c "$c_file" -o "$obj_file" $INCLUDE_FLAGS $opt_level -D_WIN32 -D__GNUC__ 2>/dev/null; then
                if gcc "$obj_file" "tests/common.o" -o "$exe_file" 2>/dev/null; then
                    if "./$exe_file" >/dev/null 2>&1; then
                        echo "  [PASS] $test_label"
                        PASSED=$((PASSED + 1))
                    else
                        echo "  [FAIL] $test_label (Execution failed)"
                        FAILED=$((FAILED + 1))
                        FAILED_LIST+=("$test_label (exec)")
                    fi
                else
                    echo "  [FAIL] $test_label (Link failed)"
                    FAILED=$((FAILED + 1))
                    FAILED_LIST+=("$test_label (link)")
                fi
            else
                echo "  [FAIL] $test_label (Compilation failed)"
                FAILED=$((FAILED + 1))
                FAILED_LIST+=("$test_label (compile)")
            fi
            rm -f "$obj_file" "$exe_file"
        done
    done
    rm -f "tests/common.o"
fi

# ==============================================================================
# Summary Report
# ==============================================================================
TOTAL=$((PASSED + FAILED))
echo ""
echo "========================================"
echo "            Test Summary                "
echo "========================================"
echo "Total Tests : $TOTAL"
echo "Passed      : $PASSED"
if [ $FAILED -gt 0 ]; then
    echo "Failed      : $FAILED"
    echo "Failed list :"
    for f in "${FAILED_LIST[@]}"; do
        echo "  - $f"
    done
    exit 1
else
    echo "Failed      : 0"
    echo ""
    echo "All tests passed successfully!"
    exit 0
fi
