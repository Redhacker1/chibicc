# PowerShell Comprehensive Test Runner for chibicc
$ErrorActionPreference = "Continue"

$Compiler = ".\cmake-build-debug\chibicc-cli.exe"
if (!(Test-Path $Compiler)) {
    Write-Host "Building chibicc..." -ForegroundColor Cyan
    cmd /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 && cmake --build cmake-build-debug"
}

$MinGWInclude = "C:\Users\donov\Documents\MinGW64\x86_64-w64-mingw32\include"
$IncludeFlags = @("-I.", "-Iinclude", "-Itest")
if (Test-Path $MinGWInclude) {
    $IncludeFlags += "-I$MinGWInclude"
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "       chibicc Test Suite Runner        " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 1. Compile common test driver helper
Write-Host "`n[Setup] Compiling test/common helper..." -ForegroundColor Yellow
gcc -x c -c "test/common" -o "test/common.o"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile test/common helper object"
    exit 1
}

$Passed = 0
$Failed = 0
$FailedTests = @()

# 2. Discover and run all unit tests in test/*.c
$unit_tests = Get-ChildItem -Path "test\*.c" | Where-Object {
    $_.Name -notmatch "^abi_" -and $_.Name -ne "tls.c"
} | Sort-Object Name

Write-Host "`n[Stage 1] Running Unit Tests ($($unit_tests.Count) tests)..." -ForegroundColor Yellow

foreach ($file in $unit_tests) {
    $name = $file.BaseName
    $cFile = $file.FullName
    $oFile = "test\$name.o"
    $exeFile = "test\$name.exe"

    # Compile with chibicc
    & $Compiler -c $cFile -o $oFile $IncludeFlags -target x86_64-win64 -D_WIN32 -D__GNUC__ 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  [FAIL] $name.c (Compilation failed)" -ForegroundColor Red
        $Failed++
        $FailedTests += "$name.c (compile)"
        continue
    }

    # Link with GCC
    $extraLinkFlags = @()
    if ($name -eq "atomic") {
        $extraLinkFlags += "-lpthread"
    }
    if ($name -eq "win_test") {
        $extraLinkFlags += "-lkernel32"
    }

    gcc $oFile "test/common.o" -o $exeFile $extraLinkFlags 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  [FAIL] $name.c (Link failed)" -ForegroundColor Red
        $Failed++
        $FailedTests += "$name.c (link)"
        Remove-Item $oFile -ErrorAction SilentlyContinue
        continue
    }

    # Execute test binary
    $output = & ".\$exeFile" 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  [PASS] $name.c" -ForegroundColor Green
        $Passed++
    } else {
        Write-Host "  [FAIL] $name.c (Execution failed, exit code $LASTEXITCODE)" -ForegroundColor Red
        if ($output) {
            Write-Host "         $output" -ForegroundColor DarkGray
        }
        $Failed++
        $FailedTests += "$name.c (exec)"
    }

    Remove-Item $oFile, $exeFile -ErrorAction SilentlyContinue
}

# 3. Run ABI Test Suites
$abi_tests = Get-ChildItem -Path "test\abi_test_all.c" | Sort-Object Name
Write-Host "`n[Stage 2] Running ABI Test Suites ($($abi_tests.Count) suite runner)..." -ForegroundColor Yellow

foreach ($file in $abi_tests) {
    $name = $file.BaseName
    $cFile = $file.FullName
    $oFile = "test\$name.o"
    $exeFile = "test\$name.exe"

    & $Compiler -c $cFile -o $oFile -I"include" -target x86_64-win64 -D_WIN32 -D__GNUC__ 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  [FAIL] $name.c (Compilation failed)" -ForegroundColor Red
        $Failed++
        $FailedTests += "$name.c (compile)"
        continue
    }

    gcc -o $exeFile $oFile 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  [FAIL] $name.c (Link failed)" -ForegroundColor Red
        $Failed++
        $FailedTests += "$name.c (link)"
        Remove-Item $oFile -ErrorAction SilentlyContinue
        continue
    }

    $output = & ".\$exeFile" 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  [PASS] $name.c" -ForegroundColor Green
        $Passed++
    } else {
        Write-Host "  [FAIL] $name.c (Execution failed, exit code $LASTEXITCODE)" -ForegroundColor Red
        $Failed++
        $FailedTests += "$name.c (exec)"
    }

    Remove-Item $oFile, $exeFile -ErrorAction SilentlyContinue
}

# Cleanup common helper
Remove-Item "test/common.o" -ErrorAction SilentlyContinue

# 4. Summary Report
$Total = $Passed + $Failed
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "            Test Summary                " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Total Tests : $Total"
Write-Host "Passed      : $Passed" -ForegroundColor Green
if ($Failed -gt 0) {
    Write-Host "Failed      : $Failed" -ForegroundColor Red
    Write-Host "Failed list :" -ForegroundColor Red
    foreach ($ft in $FailedTests) {
        Write-Host "  - $ft" -ForegroundColor Red
    }
    exit 1
} else {
    Write-Host "Failed      : 0" -ForegroundColor Green
    Write-Host "`nAll tests passed successfully!" -ForegroundColor Green
    exit 0
}
