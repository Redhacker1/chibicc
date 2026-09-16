#!/usr/bin/env bash
# Shell Build Script for chibicc libc & libm Dynamic Library
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR"
COMPILER=""
OUTPUT_DIR="sdk"
OPT_LEVEL="-O2"
TARGET=""
BUILD_STATIC=0
CLEAN=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --compiler|-c)
            COMPILER="$2"
            shift 2
            ;;
        --output-dir|-o)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --opt-level|-O)
            OPT_LEVEL="$2"
            shift 2
            ;;
        --target|-t)
            TARGET="$2"
            shift 2
            ;;
        --static)
            BUILD_STATIC=1
            shift
            ;;
        --clean)
            CLEAN=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

BUILD_OBJ_DIR="$ROOT_DIR/build/libc_obj"
OUT_DIR="$ROOT_DIR/$OUTPUT_DIR"
if [[ "$OUTPUT_DIR" = /* ]]; then
    OUT_DIR="$OUTPUT_DIR"
fi

if [[ $CLEAN -eq 1 ]]; then
    echo "Cleaning libc build artifacts..."
    rm -rf "$BUILD_OBJ_DIR"
    rm -f "$OUT_DIR/lib/libc"*
fi

# 1. Resolve Compiler
if [[ -z "$COMPILER" ]]; then
    if [[ -x "$ROOT_DIR/cmake-build-debug/chibicc-cli" ]]; then
        COMPILER="$ROOT_DIR/cmake-build-debug/chibicc-cli"
    elif [[ -x "$ROOT_DIR/cmake-build-release/chibicc-cli" ]]; then
        COMPILER="$ROOT_DIR/cmake-build-release/chibicc-cli"
    elif [[ -x "$ROOT_DIR/build_selfhost/stage2/chibicc-stage2" ]]; then
        COMPILER="$ROOT_DIR/build_selfhost/stage2/chibicc-stage2"
    elif [[ -x "$ROOT_DIR/chibicc" ]]; then
        COMPILER="$ROOT_DIR/chibicc"
    elif [[ -x "$ROOT_DIR/cmake-build-debug/chibicc-cli.exe" ]]; then
        COMPILER="$ROOT_DIR/cmake-build-debug/chibicc-cli.exe"
    else
        echo "Error: chibicc compiler not found. Specify with --compiler <path>"
        exit 1
    fi
fi

# Normalize optimization level flag
OPT_FLAG=""
if [[ -n "$OPT_LEVEL" ]]; then
    if [[ "$OPT_LEVEL" =~ ^-?O ]]; then
        if [[ "$OPT_LEVEL" = -* ]]; then
            OPT_FLAG="$OPT_LEVEL"
        else
            OPT_FLAG="-$OPT_LEVEL"
        fi
    else
        OPT_FLAG="-O$OPT_LEVEL"
    fi
fi

echo "========================================="
echo "   chibicc C Standard Library Builder    "
echo "========================================="
echo "Compiler  : $COMPILER"
echo "Output Dir: $OUT_DIR"
echo "Opt Level : ${OPT_FLAG:-default}"
if [[ -n "$TARGET" ]]; then
    echo "Target    : $TARGET"
fi

# 2. Setup Directories
mkdir -p "$BUILD_OBJ_DIR"
mkdir -p "$OUT_DIR/include"
mkdir -p "$OUT_DIR/lib"
mkdir -p "$OUT_DIR/bin"

# 3. Copy Headers
echo -e "\n--- Packaging headers to $OUT_DIR/include ---"
if [[ -d "$ROOT_DIR/libc/libc/include" ]]; then
    cp -rf "$ROOT_DIR/libc/libc/include/"* "$OUT_DIR/include/"
fi
if [[ -d "$ROOT_DIR/include" ]]; then
    cp -rf "$ROOT_DIR/include/"* "$OUT_DIR/include/"
fi

# 4. Discover Sources
echo -e "\n--- Discovering libc sources ---"

DISALLOWED_ARCHS="aarch64|arc|arc64|arm|loongarch|m68k|microblaze|mips|msp430|nios2|or1k|powerpc|riscv|rx|sh|sparc|xtensa|nds32|nvptx|amdgcn|spu|hexagon"

SRC_FILES=()
while IFS= read -r cm; do
    dir=$(dirname "$cm")
    if echo "$dir" | grep -Eq "/machine/($DISALLOWED_ARCHS)(/|$)"; then
        continue
    fi
    in_block=0
    while IFS= read -r line; do
        if echo "$line" | grep -Eq "picolibc_sources(_flags)?\s*\("; then
            in_block=1
            continue
        fi
        if [[ $in_block -eq 1 ]]; then
            if echo "$line" | grep -Eq "\)"; then
                in_block=0
                continue
            fi
            trimmed=$(echo "$line" | sed -e 's/#.*//' -e 's/["'\'']//g' | xargs)
            if [[ "$TARGET" =~ win || "$(uname -s)" =~ MINGW|MSYS|CYGWIN|Windows_NT ]]; then
                if [[ "$trimmed" =~ platform_portable\.c$ ]]; then
                    continue
                fi
            else
                if [[ "$trimmed" =~ platform_win64\.c$ ]]; then
                    continue
                fi
            fi
            if [[ "$trimmed" =~ \.c$ && "$trimmed" != *"interrupt.c"* ]]; then
                cand="$dir/$trimmed"
                if [[ -f "$cand" ]]; then
                    SRC_FILES+=("$cand")
                fi
            fi
        fi
    done < "$cm"
done < <(find "$ROOT_DIR/libc" -name "CMakeLists.txt")

# Sort and remove duplicates
IFS=$'\n' SORTED_SRCS=($(sort -u <<<"${SRC_FILES[*]}"))
unset IFS

echo "Found ${#SORTED_SRCS[@]} active libc/libm C translation units."

# 5. Compile Sources
echo -e "\n--- Compiling ${#SORTED_SRCS[@]} libc sources with chibicc ---"

INCLUDES=(
    "-I$ROOT_DIR/libc/libc/include"
    "-I$ROOT_DIR/libc/libc/include/machine"
    "-I$ROOT_DIR/libc/libc/locale"
    "-I$ROOT_DIR/libc/libc/ctype"
    "-I$ROOT_DIR/libc/libc/stdio"
    "-I$ROOT_DIR/libc/libc/stdlib"
    "-I$ROOT_DIR/libc/libc/string"
    "-I$ROOT_DIR/libc/libc/time"
    "-I$ROOT_DIR/libc/libc/errno"
    "-I$ROOT_DIR/libc/libc/misc"
    "-I$ROOT_DIR/libc/libc/posix"
    "-I$ROOT_DIR/libc/libm/common"
    "-I$ROOT_DIR/libc/libm/math"
    "-I$ROOT_DIR/include"
)

DEFINES=(
    "-D_IEEE_LIBM"
    "-D_PICOLIBC_CTYPE_SMALL=0"
    "-D__GNUC__"
)

if [[ -n "$TARGET" ]]; then
    DEFINES+=("-target" "$TARGET")
fi

OPT_ARGS=()
if [[ -n "$OPT_FLAG" ]]; then
    OPT_ARGS+=("$OPT_FLAG")
fi

OBJ_FILES=()
SUCCESS_COUNT=0
FAIL_COUNT=0

for src in "${SORTED_SRCS[@]}"; do
    rel_path="${src#$ROOT_DIR/}"
    safe_name=$(echo "$rel_path" | sed -e 's|/|_|g' -e 's|\.c$|.o|')
    obj_path="$BUILD_OBJ_DIR/$safe_name"
    s_path="$BUILD_OBJ_DIR/$safe_name.s"

    if [[ -f "$obj_path" && "$src" -ot "$obj_path" ]]; then
        OBJ_FILES+=("$obj_path")
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        continue
    fi

    if "$COMPILER" -S "$src" -o "$s_path" "${INCLUDES[@]}" "${DEFINES[@]}" "${OPT_ARGS[@]}" > /dev/null 2>&1; then
        if as -o "$obj_path" "$s_path" > /dev/null 2>&1; then
            OBJ_FILES+=("$obj_path")
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
            rm -f "$s_path"
            continue
        fi
    fi

    FAIL_COUNT=$((FAIL_COUNT + 1))
    echo "  [SKIP/FAIL] $rel_path"
done

echo "Compiled $SUCCESS_COUNT / ${#SORTED_SRCS[@]} objects successfully ($FAIL_COUNT skipped/failed)."

if [[ ${#OBJ_FILES[@]} -eq 0 ]]; then
    echo "Error: No object files were successfully compiled."
    exit 1
fi

# 6. Link Dynamic Library
DLL_NAME="libc.so"
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*|Windows_NT)
        DLL_NAME="libc.dll"
        ;;
    Darwin*)
        DLL_NAME="libc.dylib"
        ;;
    *)
        DLL_NAME="libc.so"
        ;;
esac

DLL_PATH="$OUT_DIR/lib/$DLL_NAME"
echo -e "\n--- Linking dynamically linked library: $DLL_PATH ---"

RSP_FILE="$BUILD_OBJ_DIR/objects.rsp"
printf "%s\n" "${OBJ_FILES[@]}" > "$RSP_FILE"

if [[ "$DLL_NAME" == "libc.dll" ]]; then
    IMPLIB_PATH="$OUT_DIR/lib/libc.dll.a"
    gcc -shared -nostdlib -o "$DLL_PATH" -Wl,--out-implib,"$IMPLIB_PATH" -Wl,--image-base,0x140400000 -Wl,--disable-dynamicbase -Wl,--disable-high-entropy-va -Wl,--allow-multiple-definition -Wl,-e,DllMainCRTStartup @"$RSP_FILE" -lkernel32
    cp -f "$DLL_PATH" "$OUT_DIR/bin/$DLL_NAME" 2>/dev/null || true
    cp -f "$DLL_PATH" "$ROOT_DIR/$DLL_NAME" 2>/dev/null || true
    cp -f "$IMPLIB_PATH" "$ROOT_DIR/libc.dll.a" 2>/dev/null || true
else
    gcc -shared -o "$DLL_PATH" -Wl,--allow-multiple-definition @"$RSP_FILE" || gcc -shared -o "$DLL_PATH" "${OBJ_FILES[@]}"
    cp -f "$DLL_PATH" "$OUT_DIR/bin/$DLL_NAME" 2>/dev/null || true
    cp -f "$DLL_PATH" "$ROOT_DIR/$DLL_NAME" 2>/dev/null || true
fi

echo "Successfully generated dynamic library: $DLL_PATH"

# 7. Optional Static Archive
if [[ $BUILD_STATIC -eq 1 ]]; then
    LIB_PATH="$OUT_DIR/lib/libc.a"
    echo -e "\n--- Creating static archive: $LIB_PATH ---"
    ar rcs "$LIB_PATH" @"$RSP_FILE"
    echo "Successfully generated static archive: $LIB_PATH"
fi

echo -e "\nlibc build and packaging completed successfully!"
