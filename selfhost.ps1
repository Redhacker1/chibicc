param(
    [string]$Compiler = "cmake-build-debug\chibicc-cli.exe",
    [string]$Output = "chibicc-stage2.exe",
    [string]$OptLevel = "",
    [string]$MinGWPath = "C:\Users\donov\Documents\MinGW64\x86_64-w64-mingw32\include",
    [switch]$RunTests,
    [switch]$Stage3
)

$ErrorActionPreference = "Stop"

# Normalize optimization level flag
$OptFlag = ""
if ($OptLevel) {
    if ($OptLevel -match "^-?O") {
        $OptFlag = if ($OptLevel.StartsWith("-")) { $OptLevel } else { "-$OptLevel" }
    } else {
        $OptFlag = "-O$OptLevel"
    }
}

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   chibicc Self-Hosting Bootstrap Tool   " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
if ($OptFlag) {
    Write-Host "Optimization Level: $OptFlag" -ForegroundColor Cyan
}

# 1. Locate stage1 bootstrap compiler
if (-not (Test-Path $Compiler)) {
    Write-Host "Compiler not found at '$Compiler'. Checking cmake-build-debug..." -ForegroundColor Yellow
    if (Test-Path "cmake-build-debug\chibicc-cli.exe") {
        $Compiler = "cmake-build-debug\chibicc-cli.exe"
    } elseif (Test-Path "chibicc.exe") {
        $Compiler = "chibicc.exe"
    } else {
        Write-Host "No compiler found. Building with CMake first..." -ForegroundColor Yellow
        cmd /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 && cmake --build cmake-build-debug"
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path "cmake-build-debug\chibicc-cli.exe")) {
            Write-Error "Failed to build initial bootstrap compiler."
            exit 1
        }
        $Compiler = "cmake-build-debug\chibicc-cli.exe"
    }
}

$Compiler = (Resolve-Path $Compiler).Path
Write-Host "Using bootstrap compiler: $Compiler" -ForegroundColor Green

# 2. Setup output directories and copy built-in headers
$Stage2Dir = "build_selfhost\stage2"
New-Item -ItemType Directory -Force -Path $Stage2Dir | Out-Null

$srcs = @(
    "main.c",
    "hashmap.c",
    "parse.c",
    "preprocess.c",
    "strings.c",
    "tokenize.c",
    "type.c",
    "unicode.c",
    "ir/hlir.c",
    "ir/hlir_opt.c",
    "ir/ir.c",
    "ir/opt.c",
    "ir/regalloc.c",
    "codegen/codegen.c",
    "codegen/common/common.c",
    "codegen/x86_64/x86_64.c",
    "codegen/m68k/m68k.c",
    "codegen/z80/z80.c",
    "abi/abi.c",
    "abi/objfmt.c",
    "abi/sysV64/sysv64.c",
    "abi/win64/win64.c",
    "abi/win32/win32.c",
    "abi/sys6/sys6.c",
    "abi/z80/z80.c",
    "win_src/glob.c",
    "win_src/wait.c",
    "win_src/win_shims.c",
    "win_src/unistd.c"
)

$IncludeFlags = @("-I.", "-Icompiler_include", "-Iwin_includes")
if (Test-Path $MinGWPath) {
    $IncludeFlags += "-I$MinGWPath"
}

function Build-Stage([string]$StageCompiler, [string]$BuildDir, [string]$TargetExeName) {
    Write-Host "`n--- Compiling $($srcs.Count) sources with $StageCompiler ---" -ForegroundColor Cyan
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    if (Test-Path "include") {
        Copy-Item -Recurse -Force "include" "$BuildDir\include"
    }

    $objFiles = @()
    $failed = $false

    foreach ($s in $srcs) {
        $safeName = ($s -replace "[/\\]", "_")
        $safeName = [System.IO.Path]::ChangeExtension($safeName, ".o")
        $objFile = "$BuildDir\$safeName"
        $objFiles += $objFile

        Write-Host "  [CC] $s -> $safeName"
        $optArgs = if ($OptFlag) { @($OptFlag) } else { @() }
        $ccOutput = & $StageCompiler -c $s -o $objFile $IncludeFlags $optArgs -target x86_64-win64 -D_WIN32 -D__GNUC__ 2>&1
        if ($ccOutput) {
            $ccOutput | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
        }
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  [FAIL] Compilation failed for $s" -ForegroundColor Red
            $failed = $true
            break
        }
    }

    if ($failed) {
        Write-Error "Build failed during compilation."
        exit 1
    }

    $outPath = "$BuildDir\$TargetExeName"
    Write-Host "`n--- Linking $outPath ---" -ForegroundColor Cyan
    $linkOutput = gcc "-Wl,--start-group" $objFiles "-Wl,--end-group" -s "-Wl,--stack,16777216" "-Wl,--subsystem,console" "-Wl,--dynamicbase" "-Wl,--nxcompat" "-Wl,--high-entropy-va" "-Wl,--gc-sections" -o $outPath 2>&1
    if ($linkOutput) {
        $linkOutput | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
    }
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Link failed for $outPath."
        exit 1
    }

    Write-Host "Successfully built $outPath" -ForegroundColor Green
    return (Resolve-Path $outPath).Path
}

# 3. Build Stage 2
$Stage2Exe = Build-Stage -StageCompiler $Compiler -BuildDir $Stage2Dir -TargetExeName $Output

# Also copy Stage 2 executable to current directory if user specified a simple output name
if ((Split-Path -Parent $Output) -eq "") {
    Copy-Item -Force $Stage2Exe $Output
    Write-Host "Copied executable to .\$Output" -ForegroundColor Green
}

# 4. Optional Stage 3 build
if ($Stage3) {
    Write-Host "`n=========================================" -ForegroundColor Cyan
    Write-Host "   Building Stage 3 with Stage 2 chibicc   " -ForegroundColor Cyan
    Write-Host "=========================================" -ForegroundColor Cyan
    $Stage3Dir = "build_selfhost\stage3"
    $Stage3Exe = Build-Stage -StageCompiler $Stage2Exe -BuildDir $Stage3Dir -TargetExeName "chibicc-stage3.exe"
}

# 5. Optional Run Tests
if ($RunTests) {
    Write-Host "`n=========================================" -ForegroundColor Cyan
    Write-Host "   Running Tests with Self-Hosted Compiler" -ForegroundColor Cyan
    Write-Host "=========================================" -ForegroundColor Cyan
    
    $TestCompiler = if ($Stage3) { $Stage3Exe } else { $Stage2Exe }
    $testParams = @{ Compiler = $TestCompiler }
    if ($OptFlag) {
        $testParams["OptLevel"] = $OptFlag
    }
    & .\run_tests.ps1 @testParams
}

Write-Host "`nSelf-hosting compilation completed successfully!" -ForegroundColor Green
