# PowerShell Test Runner for chibicc ABI Test Suites
$ErrorActionPreference = "Stop"

$Compiler = ".\cmake-build-debug\chibicc-cli.exe"
if (!(Test-Path $Compiler)) {
    Write-Host "Building chibicc..." -ForegroundColor Cyan
    cmd /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 && cmake --build cmake-build-debug"
}

Write-Host "Compiling and Linking ABI Test Suites..." -ForegroundColor Cyan

# 1. Compile Comprehensive ABI Test Suite
& $Compiler -c "test\abi_test_all.c" -o "test\abi_test_all.o" -I".\include" -target x86_64-win64 -D_WIN32 -D__GNUC__
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile test\abi_test_all.c"
    exit 1
}

# 2. Link with MinGW GCC / LD
gcc.exe -o "test\abi_test_all.exe" "test\abi_test_all.o"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to link test\abi_test_all.exe"
    exit 1
}

# 3. Run Test Binary
Write-Host "Executing Test Binary..." -ForegroundColor Green
& ".\test\abi_test_all.exe"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Test suite execution failed with exit code $LASTEXITCODE"
    exit 1
}

Write-Host "`nAll ABI tests compiled, linked, and executed successfully!" -ForegroundColor Green
