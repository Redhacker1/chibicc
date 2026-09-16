# PowerShell Build Script for chibicc libc & libm Dynamic Library
param(
    [string]$Compiler = "",
    [string]$OutputDir = "sdk",
    [string]$OptLevel = "-O2",
    [string]$Target = "x86_64-win64",
    [switch]$Static,
    [switch]$Clean
)

$ErrorActionPreference = "Continue"

# Normalize optimization level flag
$OptFlag = ""
if ($OptLevel -and $OptLevel.Trim() -ne "") {
    $trimmedOpt = $OptLevel.Trim()
    if ($trimmedOpt -match "^-?O") {
        $OptFlag = if ($trimmedOpt.StartsWith("-")) { $trimmedOpt } else { "-$trimmedOpt" }
    } else {
        $OptFlag = "-O$trimmedOpt"
    }
}

$RootDir = (Resolve-Path ".").Path
$BuildObjDir = "$RootDir\build\libc_obj"
if ([System.IO.Path]::IsPathRooted($OutputDir)) {
    $OutDir = $OutputDir
} else {
    $OutDir = "$RootDir\$OutputDir"
}

if ($Clean) {
    Write-Host "Cleaning libc build artifacts..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildObjDir -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force "$OutDir\lib\libc.*" -ErrorAction SilentlyContinue
}

# 1. Resolve compiler
if (-not $Compiler) {
    if (Test-Path "cmake-build-debug\chibicc-cli.exe") {
        $Compiler = "cmake-build-debug\chibicc-cli.exe"
    } elseif (Test-Path "cmake-build-release\chibicc-cli.exe") {
        $Compiler = "cmake-build-release\chibicc-cli.exe"
    } elseif (Test-Path "build_selfhost\stage2\chibicc-stage2.exe") {
        $Compiler = "build_selfhost\stage2\chibicc-stage2.exe"
    } elseif (Test-Path "chibicc.exe") {
        $Compiler = "chibicc.exe"
    } else {
        Write-Host "Building chibicc-cli first..." -ForegroundColor Cyan
        cmd /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 && cmake --build cmake-build-debug"
        $Compiler = "cmake-build-debug\chibicc-cli.exe"
    }
}

$Compiler = (Resolve-Path $Compiler).Path
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   chibicc C Standard Library Builder    " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "Compiler  : $Compiler" -ForegroundColor Green
Write-Host "Output Dir: $OutDir" -ForegroundColor Green
Write-Host "Opt Level : $(if ($OptFlag) { $OptFlag } else { 'default' })" -ForegroundColor Green
Write-Host "Target    : $Target" -ForegroundColor Green

# 2. Setup Directories
New-Item -ItemType Directory -Force -Path $BuildObjDir | Out-Null
New-Item -ItemType Directory -Force -Path "$OutDir\include" | Out-Null
New-Item -ItemType Directory -Force -Path "$OutDir\lib" | Out-Null
New-Item -ItemType Directory -Force -Path "$OutDir\bin" | Out-Null

# 3. Copy Headers to SDK
Write-Host "`n--- Packaging headers to $OutDir\include ---" -ForegroundColor Cyan
if (Test-Path "libc\libc\include") {
    Copy-Item -Recurse -Force "libc\libc\include\*" "$OutDir\include\"
}
if (Test-Path "include") {
    Copy-Item -Recurse -Force "include\*" "$OutDir\include\"
}

# 4. Collect Source Files from CMakeLists.txt
Write-Host "`n--- Discovering libc sources from CMakeLists.txt ---" -ForegroundColor Cyan

$DisallowedMachines = @("aarch64", "arc", "arc64", "arm", "loongarch", "m68k", "microblaze", "mips", "msp430", "nios2", "or1k", "powerpc", "riscv", "rx", "sh", "sparc", "xtensa", "nds32", "nvptx", "amdgcn", "spu", "hexagon")

$cmakeFiles = Get-ChildItem -Path "libc" -Filter "CMakeLists.txt" -Recurse | Where-Object {
    $dirName = $_.DirectoryName
    $skip = $false
    foreach ($arch in $DisallowedMachines) {
        if ($dirName -match "[\\/]machine[\\/]$arch([\\/]|$)") {
            $skip = $true
            break
        }
    }
    -not $skip
}

$isWin = ($Target -match "win") -or ($env:OS -match "Windows")

$srcFiles = @()
foreach ($cm in $cmakeFiles) {
    $content = Get-Content $cm.FullName
    $inBlock = $false
    foreach ($line in $content) {
        if ($line -match "picolibc_sources(_flags)?\s*\(") {
            $inBlock = $true
            continue
        }
        if ($inBlock) {
            if ($line -match "\)") {
                $inBlock = $false
                continue
            }
            $trimmed = $line.Trim()
            # remove inline comments or quotes
            $trimmed = ($trimmed -replace "#.*$", "") -replace "['""]", ""
            $trimmed = $trimmed.Trim()
            if ($isWin -and $trimmed -match "platform_portable\.c$") {
                continue
            }
            if (-not $isWin -and $trimmed -match "platform_win64\.c$") {
                continue
            }
            if ($trimmed -match "\.c$" -and $trimmed -notmatch "interrupt\.c$") {
                $candidatePath = Join-Path $cm.DirectoryName $trimmed
                if (Test-Path $candidatePath) {
                    $srcFiles += (Resolve-Path $candidatePath).Path
                }
            }
        }
    }
}

# Remove duplicates if any
$srcFiles = $srcFiles | Sort-Object -Unique

Write-Host "Found $($srcFiles.Count) active libc/libm C translation units." -ForegroundColor Cyan

Write-Host "`n--- Compiling $($srcFiles.Count) libc sources with chibicc ---" -ForegroundColor Cyan

$IncludeFlags = @(
    "-I$RootDir\libc\libc\machine\x86",
    "-I$RootDir\libc\libc\include",
    "-I$RootDir\libc\libc\include\machine",
    "-I$RootDir\libc\libc\locale",
    "-I$RootDir\libc\libc\ctype",
    "-I$RootDir\libc\libc\stdio",
    "-I$RootDir\libc\libc\stdlib",
    "-I$RootDir\libc\libc\string",
    "-I$RootDir\libc\libc\time",
    "-I$RootDir\libc\libc\errno",
    "-I$RootDir\libc\libc\misc",
    "-I$RootDir\libc\libc\posix",
    "-I$RootDir\libc\libm\common",
    "-I$RootDir\libc\libm\math",
    "-I$RootDir\include"
)

$Defines = @(
    "-target", $Target,
    "-D_WIN32",
    "-D__GNUC__",
    "-D_IEEE_LIBM",
    "-D_PICOLIBC_CTYPE_SMALL=0"
)

$optArgs = if ($OptFlag) { @($OptFlag) } else { @() }

$objFiles = @()
$successCount = 0
$failCount = 0

foreach ($file in $srcFiles) {
    $fileStr = [string]$file
    $relPath = if ($fileStr.StartsWith($RootDir)) { $fileStr.Substring($RootDir.Length + 1) } else { $fileStr }
    $safeObjName = ($relPath -replace "[\\/]", "_") -replace "\.c$", ".o"
    $objPath = "$BuildObjDir\$safeObjName"
    $sFile = "$BuildObjDir\$safeObjName.s"

    if ((Test-Path $objPath) -and ((Get-Item $fileStr).LastWriteTime -le (Get-Item $objPath).LastWriteTime)) {
        $objFiles += $objPath
        $successCount++
        continue
    }

    $cmdArgs = @("-S", $fileStr, "-o", $sFile) + $IncludeFlags + $Defines + $optArgs
    $ccOutput = & $Compiler @cmdArgs 2>&1
    if ($LASTEXITCODE -eq 0 -and (Test-Path $sFile)) {
        $sFileNorm = $sFile -replace "\\", "/"
        $objPathNorm = $objPath -replace "\\", "/"
        & as -o $objPathNorm $sFileNorm 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0 -and (Test-Path $objPath)) {
            $objFiles += $objPath
            $successCount++
            Remove-Item -Force $sFile -ErrorAction SilentlyContinue
            continue
        }
    }

    $failCount++
    Write-Host "  [SKIP/FAIL] $relPath" -ForegroundColor Yellow
    if ($ccOutput) {
        $ccOutput | ForEach-Object { Write-Host "              $_" -ForegroundColor DarkGray }
    }
}

Write-Host "Compiled $successCount / $($srcFiles.Count) objects successfully ($failCount skipped/failed)." -ForegroundColor Green

if ($objFiles.Count -eq 0) {
    Write-Error "No object files were successfully compiled."
    exit 1
}

# 5. Link Dynamic Library
$DllPath = "$OutDir\lib\libc.dll"
$ImplibPath = "$OutDir\lib\libc.dll.a"
$BinDllPath = "$OutDir\bin\libc.dll"

Write-Host "`n--- Linking dynamically linked library: $DllPath ---" -ForegroundColor Cyan

# Batch arguments if command line is long
$objFileListFile = "$BuildObjDir\objects.rsp"
$objFilesEscaped = $objFiles | ForEach-Object { "$_" -replace "\\", "/" }
$objFilesEscaped -join "`n" | Set-Content -Path $objFileListFile

$linkOutput = gcc -shared -nostdlib -o $DllPath "-Wl,--out-implib,$ImplibPath" "-Wl,--image-base,0x140400000" "-Wl,--disable-dynamicbase" "-Wl,--disable-high-entropy-va" "-Wl,--allow-multiple-definition" "-Wl,-e,DllMainCRTStartup" "@$objFileListFile" -lkernel32 2>&1

if ($LASTEXITCODE -eq 0 -and (Test-Path $DllPath)) {
    Copy-Item -Force $DllPath "$RootDir\libc.dll" -ErrorAction SilentlyContinue
    & gendef "libc.dll"
    if (Test-Path "libc.def") {
        & dlltool -d "libc.def" -l $ImplibPath -D libc.dll -m i386:x86-64
        $LibExe = Get-Command "lib.exe" -ErrorAction SilentlyContinue
        if ($LibExe) {
            & lib.exe /def:libc.def /out:"$RootDir\sdk\lib\libc.lib" /machine:x64 /nologo | Out-Null
            Copy-Item -Force "$RootDir\sdk\lib\libc.lib" "$RootDir\libc.lib" -ErrorAction SilentlyContinue
        }
        Remove-Item "libc.def" -Force -ErrorAction SilentlyContinue
    }
    Write-Host "Successfully generated dynamic library: $DllPath" -ForegroundColor Green
    Copy-Item -Force $DllPath $BinDllPath
    Copy-Item -Force $ImplibPath "$RootDir\libc.dll.a" -ErrorAction SilentlyContinue
    if (Test-Path "cmake-build-debug") {
        Copy-Item -Force $DllPath "cmake-build-debug\libc.dll" -ErrorAction SilentlyContinue
    }
} else {
    Write-Host "Dynamic link output: $linkOutput" -ForegroundColor Red
    Write-Error "Failed to link dynamic library $DllPath."
    exit 1
}

# 6. Optional Static Library
if ($Static) {
    $LibPath = "$OutDir\lib\libc.a"
    Write-Host "`n--- Creating static archive: $LibPath ---" -ForegroundColor Cyan
    $arOutput = & ar rcs $LibPath "@$objFileListFile" 2>&1
    if ($LASTEXITCODE -eq 0 -and (Test-Path $LibPath)) {
        Write-Host "Successfully generated static archive: $LibPath" -ForegroundColor Green
    } else {
        Write-Host "Static archive output: $arOutput" -ForegroundColor Red
        Write-Error "Failed to create static archive $LibPath."
        exit 1
    }
}

Write-Host "`nlibc build and packaging completed successfully!" -ForegroundColor Green
