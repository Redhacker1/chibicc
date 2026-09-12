param(
    [string]$Compiler = "..\cmake-build-debug\chibicc-cli.exe",
    [string]$OptLevel = "",
    [string]$MinGWInclude = "C:\Users\donov\Documents\MinGW64\x86_64-w64-mingw32\include"
)

# Normalize optimization level flag
$OptFlag = ""
if ($OptLevel) {
    if ($OptLevel -match "^-?O") {
        $OptFlag = if ($OptLevel.StartsWith("-")) { $OptLevel } else { "-$OptLevel" }
    } else {
        $OptFlag = "-O$OptLevel"
    }
}

# Resolve compiler path
if (-not (Test-Path $Compiler)) {
    if (Test-Path "..\chibicc.exe") {
        $Compiler = "..\chibicc.exe"
    } elseif (Test-Path "cmake-build-debug\chibicc-cli.exe") {
        $Compiler = "cmake-build-debug\chibicc-cli.exe"
    } elseif (Test-Path "chibicc.exe") {
        $Compiler = "chibicc.exe"
    }
}
$Compiler = (Resolve-Path $Compiler).Path

$OrigDir = Get-Location
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

$RootDir = (Resolve-Path "..").Path
$IncludeFlags = @("-I$RootDir", "-I$RootDir\include", "-I$RootDir\compiler_include")
if (Test-Path $MinGWInclude) {
    $IncludeFlags += "-I$MinGWInclude"
}

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "       C11 Conformance Test Suite         " -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Compiler : $Compiler" -ForegroundColor Cyan
if ($OptFlag) {
    Write-Host "Opt Level: $OptFlag" -ForegroundColor Cyan
}

$Passed = 0
$Failed = 0
$FailedList = @()

# 1. Positive tests
$categories = Get-ChildItem -Directory | Where-Object { $_.Name -ne "negative" } | Sort-Object Name

foreach ($cat in $categories) {
    Write-Host "`n[$($cat.Name)]" -ForegroundColor Magenta
    $tests = Get-ChildItem -Path "$($cat.FullName)\*.c" | Sort-Object Name

    foreach ($test in $tests) {
        $name = "$($cat.Name)/$($test.Name)"
        $cFile = $test.FullName
        $oFile = [System.IO.Path]::ChangeExtension($cFile, ".o")
        $exeFile = [System.IO.Path]::ChangeExtension($cFile, ".exe")

        $optArgs = if ($OptFlag) { @($OptFlag) } else { @() }
        & $Compiler -c $cFile -o $oFile $IncludeFlags $optArgs -target x86_64-win64 -D_WIN32 -D__GNUC__ 2>$null
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  [FAIL] $name (Compilation failed)" -ForegroundColor Red
            $Failed++
            $FailedList += "$name (compile)"
            continue
        }

        $extraLinkFlags = @()
        if ($name -match "thread_local|atomic") {
            $extraLinkFlags += "-lpthread"
        }

        gcc $oFile -o $exeFile $extraLinkFlags 2>$null
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  [FAIL] $name (Link failed)" -ForegroundColor Red
            $Failed++
            $FailedList += "$name (link)"
            Remove-Item $oFile -ErrorAction SilentlyContinue
            continue
        }

        & $exeFile 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  [PASS] $name" -ForegroundColor Green
            $Passed++
        } else {
            Write-Host "  [FAIL] $name (Execution returned $LASTEXITCODE)" -ForegroundColor Red
            $Failed++
            $FailedList += "$name (exec $LASTEXITCODE)"
        }

        Remove-Item $oFile, $exeFile -ErrorAction SilentlyContinue
    }
}

# 2. Negative tests (Must FAIL compilation)
Write-Host "`n[negative (compile rejection tests)]" -ForegroundColor Magenta
$negTests = Get-ChildItem -Path "negative\*.c" | Sort-Object Name

foreach ($test in $negTests) {
    $name = "negative/$($test.Name)"
    $cFile = $test.FullName
    $oFile = [System.IO.Path]::ChangeExtension($cFile, ".o")

    $optArgs = if ($OptFlag) { @($OptFlag) } else { @() }
    & $Compiler -c $cFile -o $oFile $IncludeFlags $optArgs -target x86_64-win64 -D_WIN32 -D__GNUC__ 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  [PASS] $name (Correctly rejected)" -ForegroundColor Green
        $Passed++
    } else {
        Write-Host "  [FAIL] $name (Incorrectly accepted invalid C11 code)" -ForegroundColor Red
        $Failed++
        $FailedList += "$name (should fail compile)"
    }

    Remove-Item $oFile -ErrorAction SilentlyContinue
}

Set-Location $OrigDir

Write-Host "`n==========================================" -ForegroundColor Cyan
Write-Host "C11 Conformance Suite Results" -ForegroundColor Cyan
Write-Host "Passed: $Passed" -ForegroundColor Green
if ($Failed -gt 0) {
    Write-Host "Failed: $Failed" -ForegroundColor Red
    foreach ($f in $FailedList) {
        Write-Host "  - $f" -ForegroundColor Red
    }
    exit 1
} else {
    Write-Host "All tests passed successfully!" -ForegroundColor Green
    exit 0
}
