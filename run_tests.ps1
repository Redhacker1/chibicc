# PowerShell Comprehensive Test Runner for chibicc
param(
    [string]$Compiler = "",
    [string]$OptLevel = "",
    [string]$MinGWInclude = "C:\Users\donov\Documents\MinGW64\x86_64-w64-mingw32\include"
)

$ErrorActionPreference = "Continue"

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
if (-not $Compiler) {
    if (Test-Path "cmake-build-debug\chibicc-cli.exe") {
        $Compiler = "cmake-build-debug\chibicc-cli.exe"
    } elseif (Test-Path "chibicc.exe") {
        $Compiler = "chibicc.exe"
    } else {
        $cmakeConfig = "Debug"
        if ($OptFlag -in @("-O2", "-O3")) {
            $cmakeConfig = "Release"
        } elseif ($OptFlag -eq "-Os") {
            $cmakeConfig = "MinSizeRel"
        }
        Write-Host "Building chibicc ($cmakeConfig)..." -ForegroundColor Cyan
        cmd /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 && cmake --build cmake-build-debug --config $cmakeConfig"
        $Compiler = "cmake-build-debug\chibicc-cli.exe"
    }
} elseif (-not (Test-Path $Compiler)) {
    if (Test-Path "chibicc.exe") {
        $Compiler = "chibicc.exe"
    } elseif (Test-Path "cmake-build-debug\chibicc-cli.exe") {
        $Compiler = "cmake-build-debug\chibicc-cli.exe"
    } else {
        $cmakeConfig = "Debug"
        if ($OptFlag -in @("-O2", "-O3")) {
            $cmakeConfig = "Release"
        } elseif ($OptFlag -eq "-Os") {
            $cmakeConfig = "MinSizeRel"
        }
        Write-Host "Building chibicc ($cmakeConfig)..." -ForegroundColor Cyan
        cmd /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 && cmake --build cmake-build-debug --config $cmakeConfig"
        $Compiler = "cmake-build-debug\chibicc-cli.exe"
    }
}
$Compiler = (Resolve-Path $Compiler).Path

$RootDir = (Resolve-Path ".").Path
$IncludeFlags = @("-I$RootDir", "-I$RootDir\include", "-I$RootDir\compiler_include", "-I$RootDir\tests")
if (Test-Path $MinGWInclude) {
    $IncludeFlags += "-I$MinGWInclude"
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "       chibicc Test Suite Runner        " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Compiler : $Compiler" -ForegroundColor Cyan
if ($OptFlag) {
    Write-Host "Opt Level: $OptFlag" -ForegroundColor Cyan
}

$Passed = 0
$Failed = 0
$FailedTests = @()

# ==============================================================================
# 1. Unit Tests (in tests/*.c)
# ==============================================================================
Write-Host "`n[Setup] Compiling tests/common helper..." -ForegroundColor Yellow
gcc -x c -c "tests/common" -o "tests/common.o"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile tests/common helper object"
    exit 1
}

$unit_tests = Get-ChildItem -Path "tests\*.c" | Where-Object {
    $_.Name -notmatch "^abi_"
} | Sort-Object Name

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Section 1: Unit & Extension Tests ($($unit_tests.Count) tests)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

foreach ($file in $unit_tests) {
    $name = $file.BaseName
    $cFile = $file.FullName
    $oFile = "tests\$name.o"
    $exeFile = "tests\$name.exe"

    # Compile with chibicc
    $optArgs = if ($OptFlag) { @($OptFlag) } else { @() }
    $ccOutput = & $Compiler -c $cFile -o $oFile $IncludeFlags $optArgs -target x86_64-win64 -D_WIN32 -D__GNUC__ 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  [FAIL] $name.c (Compilation failed)" -ForegroundColor Red
        if ($ccOutput) {
            $ccOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
        }
        $Failed++
        $FailedTests += "unit/$name.c (compile)"
        continue
    } elseif ($ccOutput) {
        $ccOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
    }

    # Link with GCC
    $extraLinkFlags = @()
    if ($name -eq "atomic") {
        $extraLinkFlags += "-lpthread"
    }
    if ($name -eq "win_test") {
        $extraLinkFlags += "-lkernel32"
    }

    $linkOutput = gcc $oFile "tests/common.o" -o $exeFile $extraLinkFlags 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  [FAIL] $name.c (Link failed)" -ForegroundColor Red
        if ($linkOutput) {
            $linkOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
        }
        $Failed++
        $FailedTests += "unit/$name.c (link)"
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
        $FailedTests += "unit/$name.c (exec)"
    }

    Remove-Item $oFile, $exeFile -ErrorAction SilentlyContinue
}

# Cleanup common helper
Remove-Item "tests/common.o" -ErrorAction SilentlyContinue

# ==============================================================================
# 2. ABI Test Suites
# ==============================================================================
$abi_tests = Get-ChildItem -Path "tests\abi_test_all.c" | Sort-Object Name
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Section 2: ABI Test Suites ($($abi_tests.Count) suite runner)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

foreach ($file in $abi_tests) {
    $name = $file.BaseName
    $cFile = $file.FullName
    $oFile = "tests\$name.o"
    $exeFile = "tests\$name.exe"

    $optArgs = if ($OptFlag) { @($OptFlag) } else { @() }
    $ccOutput = & $Compiler -c $cFile -o $oFile -I"$RootDir\include" $optArgs -target x86_64-win64 -D_WIN32 -D__GNUC__ 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  [FAIL] $name.c (Compilation failed)" -ForegroundColor Red
        if ($ccOutput) {
            $ccOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
        }
        $Failed++
        $FailedTests += "abi/$name.c (compile)"
        continue
    } elseif ($ccOutput) {
        $ccOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
    }

    $linkOutput = gcc -o $exeFile $oFile 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  [FAIL] $name.c (Link failed)" -ForegroundColor Red
        if ($linkOutput) {
            $linkOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
        }
        $Failed++
        $FailedTests += "abi/$name.c (link)"
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
        $FailedTests += "abi/$name.c (exec)"
    }

    Remove-Item $oFile, $exeFile -ErrorAction SilentlyContinue
}

# ==============================================================================
# 3. C11 Conformance Test Suite
# ==============================================================================
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Section 3: C11 Conformance Test Suite   " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$confDir = "$RootDir\tests\conformance-tests"
if (Test-Path $confDir) {
    # 3.1 Positive Conformance Tests
    $categories = Get-ChildItem -Path $confDir -Directory | Where-Object { $_.Name -ne "negative" } | Sort-Object Name

    foreach ($cat in $categories) {
        Write-Host "`n[$($cat.Name)]" -ForegroundColor Magenta
        $tests = Get-ChildItem -Path "$($cat.FullName)\*.c" | Sort-Object Name

        foreach ($test in $tests) {
            $testName = "$($cat.Name)/$($test.Name)"
            $cFile = $test.FullName
            $oFile = [System.IO.Path]::ChangeExtension($cFile, ".o")
            $exeFile = [System.IO.Path]::ChangeExtension($cFile, ".exe")

            $optArgs = if ($OptFlag) { @($OptFlag) } else { @() }
            $ccOutput = & $Compiler -c $cFile -o $oFile $IncludeFlags $optArgs -target x86_64-win64 -D_WIN32 -D__GNUC__ 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-Host "  [FAIL] $testName (Compilation failed)" -ForegroundColor Red
                if ($ccOutput) {
                    $ccOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
                }
                $Failed++
                $FailedTests += "conformance/$testName (compile)"
                continue
            } elseif ($ccOutput) {
                $ccOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
            }

            $extraLinkFlags = @()
            if ($testName -match "thread_local|atomic") {
                $extraLinkFlags += "-lpthread"
            }

            $linkOutput = gcc $oFile -o $exeFile $extraLinkFlags 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-Host "  [FAIL] $testName (Link failed)" -ForegroundColor Red
                if ($linkOutput) {
                    $linkOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
                }
                $Failed++
                $FailedTests += "conformance/$testName (link)"
                Remove-Item $oFile -ErrorAction SilentlyContinue
                continue
            }

            & $exeFile 2>&1 | Out-Null
            if ($LASTEXITCODE -eq 0) {
                Write-Host "  [PASS] $testName" -ForegroundColor Green
                $Passed++
            } else {
                Write-Host "  [FAIL] $testName (Execution returned $LASTEXITCODE)" -ForegroundColor Red
                $Failed++
                $FailedTests += "conformance/$testName (exec $LASTEXITCODE)"
            }

            Remove-Item $oFile, $exeFile -ErrorAction SilentlyContinue
        }
    }

    # 3.2 Negative Conformance Tests (Must FAIL compilation)
    Write-Host "`n[negative (compile rejection tests)]" -ForegroundColor Magenta
    $negTests = Get-ChildItem -Path "$confDir\negative\*.c" | Sort-Object Name

    foreach ($test in $negTests) {
        $testName = "negative/$($test.Name)"
        $cFile = $test.FullName
        $oFile = [System.IO.Path]::ChangeExtension($cFile, ".o")

        $optArgs = if ($OptFlag) { @($OptFlag) } else { @() }
        $ccOutput = & $Compiler -c $cFile -o $oFile $IncludeFlags $optArgs -target x86_64-win64 -D_WIN32 -D__GNUC__ 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  [PASS] $testName (Correctly rejected)" -ForegroundColor Green
            $Passed++
        } else {
            Write-Host "  [FAIL] $testName (Incorrectly accepted invalid C11 code)" -ForegroundColor Red
            if ($ccOutput) {
                $ccOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
            }
            $Failed++
            $FailedTests += "conformance/$testName (should fail compile)"
        }

        Remove-Item $oFile -ErrorAction SilentlyContinue
    }
}

# ==============================================================================
# 4. Optimization Passes Test Suite
# ==============================================================================
$optDir = "$RootDir\tests\optimizations"
if (Test-Path $optDir) {
    # 4.1 Integration tests compiled with chibicc
    $integrationTests = Get-ChildItem -Path "$optDir\*.c" | Where-Object { $_.Name -ne "hlir_opt_handcrafted.c" } | Sort-Object Name
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "Section 4: Optimization Passes Tests ($($integrationTests.Count) suites)" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan

    Write-Host "`n[Setup] Compiling tests/common helper for optimizations..." -ForegroundColor Yellow
    gcc -x c -c "tests/common" -o "tests/common.o"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to compile tests/common helper object"
        exit 1
    }

    $optLevels = if ($OptFlag) { @($OptFlag) } else { @("-O1", "-O2", "-O3", "-Os") }

    foreach ($file in $integrationTests) {
        $name = $file.BaseName
        $cFile = $file.FullName
        $oFile = "tests\optimizations\$name.o"
        $exeFile = "tests\optimizations\$name.exe"

        foreach ($optFlag in $optLevels) {
            $testLabel = "optimizations/$name.c ($optFlag)"

            # Compile with chibicc at the specified optimization level
            $ccOutput = & $Compiler -c $cFile -o $oFile $IncludeFlags -target x86_64-win64 -D_WIN32 -D__GNUC__ $optFlag 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-Host "  [FAIL] $testLabel (Compilation failed)" -ForegroundColor Red
                if ($ccOutput) {
                    $ccOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
                }
                $Failed++
                $FailedTests += "$testLabel (compile)"
                continue
            } elseif ($ccOutput) {
                $ccOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
            }

            # Link with GCC
            $linkOutput = gcc $oFile "tests/common.o" -o $exeFile 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-Host "  [FAIL] $testLabel (Link failed)" -ForegroundColor Red
                if ($linkOutput) {
                    $linkOutput | ForEach-Object { Write-Host "         $_" -ForegroundColor DarkGray }
                }
                $Failed++
                $FailedTests += "$testLabel (link)"
                Remove-Item $oFile -ErrorAction SilentlyContinue
                continue
            }

            # Execute test binary
            $output = & ".\$exeFile" 2>&1
            if ($LASTEXITCODE -eq 0) {
                Write-Host "  [PASS] $testLabel" -ForegroundColor Green
                $Passed++
            } else {
                Write-Host "  [FAIL] $testLabel (Execution failed, exit code $LASTEXITCODE)" -ForegroundColor Red
                if ($output) {
                    Write-Host "         $output" -ForegroundColor DarkGray
                }
                $Failed++
                $FailedTests += "$testLabel (exec)"
            }

            Remove-Item $oFile, $exeFile -ErrorAction SilentlyContinue
        }
    }

    # 4.2 Internal compiler optimization tests (handcrafted IR)
    $handcraftedExe = ".\cmake-build-debug\hlir-opt-test.exe"
    if (-not (Test-Path $handcraftedExe)) {
        $handcraftedExe = ".\hlir-opt-test.exe"
    }
    
    Write-Host "`n[Internal Optimization Tests]" -ForegroundColor Magenta
    if (Test-Path $handcraftedExe) {
        $output = & $handcraftedExe 2>&1
        $output | ForEach-Object {
            Write-Host "  $_" -ForegroundColor DarkGray
        }
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  [PASS] hlir-opt-test" -ForegroundColor Green
            $Passed++
        } else {
            Write-Host "  [FAIL] hlir-opt-test (Failed with exit code $LASTEXITCODE)" -ForegroundColor Red
            $Failed++
            $FailedTests += "optimizations/hlir-opt-test (internal)"
        }
    } else {
        Write-Host "  [SKIP] hlir-opt-test (Not found at $handcraftedExe)" -ForegroundColor Yellow
    }

    # Cleanup common helper
    Remove-Item "tests/common.o" -ErrorAction SilentlyContinue
}

# ==============================================================================
# Summary Report
# ==============================================================================
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
