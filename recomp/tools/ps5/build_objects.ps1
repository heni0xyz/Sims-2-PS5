[CmdletBinding()]
param(
    [ValidateSet('Debug','Profile','Release')][string]$Mode='Profile',
    [switch]$GuestProfile,
    [switch]$DeepProfile,
    [switch]$ObjectWaitChannels,
    [switch]$NoHandleCache,
    [string]$Dependencies='',
    [string]$BuildDirectory='',
    [string]$TomlInclude='',
    [string]$FoundationDirectory='',
    [string]$VulkanDirectory='',
    [string]$GlslangSource='',
    [string]$GlslangBuild='',
    [ValidateRange(1,32)][int]$Jobs=2
)
$ErrorActionPreference='Stop'
if(($GuestProfile -or $DeepProfile) -and $Mode -ne 'Profile'){throw '-GuestProfile requires -Mode Profile'}
$repoRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if(-not $Dependencies){$Dependencies=Join-Path (Split-Path -Parent $repoRoot) 'deps'}
if(-not $BuildDirectory){$BuildDirectory=Join-Path $repoRoot ('out/build/ps5-game-'+$Mode.ToLowerInvariant())}
$BuildDirectory=[IO.Path]::GetFullPath($BuildDirectory)
$source=Join-Path $repoRoot 'ps5/game'
$cmake=Join-Path $Dependencies 'python-tools/cmake/data/bin/cmake.exe'
$ninja=Join-Path $Dependencies 'python-tools/bin/ninja.exe'
$llvm=Join-Path $Dependencies 'llvm-mingw-20261020-ucrt-x86_64/bin'
$sdk=Join-Path $Dependencies 'sdk-release/ps5-payload-sdk'
foreach($required in @($cmake,$ninja,(Join-Path $llvm 'clang++.exe'),(Join-Path $sdk 'win/prospero-clang++.cmd'))){
    if(-not(Test-Path -LiteralPath $required)){throw "Missing public build dependency: $required"}
}
if(-not $TomlInclude){
    foreach($relative in @('out/build/host-tools/_deps/tomlplusplus-src/include','out/build/ps5-host-tools/_deps/tomlplusplus-src/include')){
        $candidate=Join-Path $repoRoot $relative
        if(Test-Path -LiteralPath (Join-Path $candidate 'toml++/toml.hpp')){$TomlInclude=$candidate;break}
    }
    if(-not $TomlInclude){$candidate=Join-Path $repoRoot 'out/host/_deps/tomlplusplus-src/include';if(Test-Path -LiteralPath (Join-Path $candidate 'toml++/toml.hpp')){$TomlInclude=$candidate}}
}
if(-not $TomlInclude){throw 'Build the pinned host tools first or provide -TomlInclude'}
if((Get-FileHash -LiteralPath (Join-Path $repoRoot 'assets/default.xbe') -Algorithm SHA1).Hash.ToLowerInvariant() -ne '16f9e41ce26c315a758f25d43ba1fa9d3809d1ad'){
    throw 'The supplied XBE is not the supported USA retail executable'
}
New-Item -ItemType Directory -Path $BuildDirectory -Force | Out-Null
$lock=[IO.File]::Open((Join-Path $BuildDirectory '.objects-build.lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
try {
    $cache=Join-Path $BuildDirectory 'CMakeCache.txt'
    if(Test-Path -LiteralPath $cache){
        $homeLine=Get-Content -LiteralPath $cache | Where-Object {$_ -like 'CMAKE_HOME_DIRECTORY:INTERNAL=*'} | Select-Object -First 1
        if($homeLine){
            $configured=[IO.Path]::GetFullPath($homeLine.Substring($homeLine.IndexOf('=')+1))
            if(-not $configured.Equals([IO.Path]::GetFullPath($source),[StringComparison]::OrdinalIgnoreCase)){throw 'Refusing to configure a different project build tree'}
        }
    }
    $configuration=if($Mode -eq 'Debug'){'Debug'}else{'Release'}
    $optimization=if($Mode -eq 'Debug'){'-O0 -g1'}else{'-O2 -g1 -DNDEBUG'}
    $diagnostic=if($Mode -eq 'Debug'){'ON'}else{'OFF'}
    $performance=if($Mode -eq 'Profile'){'ON'}else{'OFF'}
    $guestProfileSetting=if($GuestProfile){'ON'}else{'OFF'}
    $objectWaitSetting=if($ObjectWaitChannels){'ON'}else{'OFF'}
    $deepProfileSetting=if($DeepProfile){'ON'}else{'OFF'}
    $handleCacheSetting=if($NoHandleCache){'OFF'}else{'ON'}
    $env:PATH="$llvm;$env:PATH"
    $started=[DateTime]::UtcNow
    function Write-BuildState([string]$State,[hashtable]$Details=@{}) {
        $record=@{schema=1;state=$State;mode=$Mode;started_at=$started.ToString('o');observed_at=[DateTime]::UtcNow.ToString('o');elapsed_seconds=([DateTime]::UtcNow-$started).TotalSeconds}
        foreach($key in $Details.Keys){$record[$key]=$Details[$key]}
        $record | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $BuildDirectory 'objects-build-status.json')
    }
    Write-BuildState 'CONFIGURE'
    $configUpper=$configuration.ToUpperInvariant()
    $arguments=@('-S',$source,'-B',$BuildDirectory,'-G','Ninja',"-DCMAKE_MAKE_PROGRAM=$ninja",
        "-DCMAKE_TOOLCHAIN_FILE=$sdk/toolchain/prospero.cmake",
        "-DCMAKE_CXX_COMPILER=$sdk/win/prospero-clang++.cmd","-DCMAKE_C_COMPILER=$sdk/win/prospero-clang.cmd",
        "-DCMAKE_AR=$llvm/llvm-ar.exe","-DCMAKE_RANLIB=$llvm/llvm-ranlib.exe",
        '-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY',"-DCMAKE_BUILD_TYPE=$configuration",
        '-DCMAKE_CXX_FLAGS=--no-default-config','-DCMAKE_C_FLAGS=--no-default-config',
        "-DCMAKE_CXX_FLAGS_${configUpper}=$optimization","-DCMAKE_C_FLAGS_${configUpper}=$optimization",
        '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',"-DPS5_PLATFORM_HEADERS=$Dependencies/sdk-pin-headers/platform/include",
        "-DTOML_INCLUDE=$TomlInclude","-DSIMS2_DIAGNOSTIC_LOG=$diagnostic","-DSIMS2_PERFORMANCE_LOG=$performance",
        "-DSIMS2_GUEST_PROFILE=$guestProfileSetting","-DSIMS2_FAST_HANDLES=$handleCacheSetting","-DSIMS2_DEEP_PROFILE=$deepProfileSetting","-DSIMS2_OBJECT_WAIT_CHANNELS=$objectWaitSetting",
        '-DSIMS2_VULKAN_STARTUP_PROBE=OFF','-DSIMS2_BUILD_ID=')
    foreach($entry in @(@('PS5_FOUNDATION',$FoundationDirectory),@('PS5_VULKAN_DIR',$VulkanDirectory),@('GLSLANG_SOURCE',$GlslangSource),@('GLSLANG_BUILD',$GlslangBuild))){
        if($entry[1]){$arguments+="-D$($entry[0])=$($entry[1])"}
    }
    & $cmake @arguments *> (Join-Path $BuildDirectory 'configure.log')
    if($LASTEXITCODE){Write-BuildState 'CONFIGURE_FAILED';throw 'CMake configuration failed; see configure.log'}
    $commands=Get-Content -LiteralPath (Join-Path $BuildDirectory 'compile_commands.json') -Raw | ConvertFrom-Json
    $selected=@($commands | Where-Object {$_.command -match 'sims2-(runtime|guest|ps5-memory|ps5-services)\.dir'})
    if($selected.Count -lt 65){throw 'Incomplete native/guest compile-command inventory'}
    $expected=if($Mode -eq 'Debug'){'0'}else{'2'}
    foreach($command in $selected){
        $flags=[regex]::Matches($command.command,'(?:^|\s)-O([0-3szg])(?=\s|$)')
        if(-not $flags.Count -or $flags[$flags.Count-1].Groups[1].Value -ne $expected){throw "Unexpected optimization for $($command.file)"}
        if($Mode -ne 'Debug' -and $command.command -notmatch '(?:^|\s)-DNDEBUG(?:\s|$)'){throw 'Optimized build must define NDEBUG'}
    }
    $identity=(Get-Content -LiteralPath (Join-Path $BuildDirectory 'build-id.txt') -Raw).Trim()
    Write-BuildState 'COMPILE' @{build_id=$identity;audited_compile_commands=$selected.Count;optimization="O$expected"}
    & $cmake --build $BuildDirectory --target sims2-runtime sims2-guest sims2-ps5-memory sims2-ps5-services --parallel $Jobs *> (Join-Path $BuildDirectory 'objects-build.log')
    if($LASTEXITCODE){Write-BuildState 'COMPILE_FAILED' @{build_id=$identity};throw 'Native object compilation failed; see objects-build.log'}
    $profile=@{schema=1;build_id=$identity;mode=$Mode;configuration=$configuration;optimization="O$expected";debug_symbols='g1';diagnostic_logging=$diagnostic;performance_logging=$performance;guest_profile=$guestProfileSetting;fast_handles=$handleCacheSetting;deep_profile=$deepProfileSetting;object_wait_channels=$objectWaitSetting;startup_probe='OFF';audited_compile_commands=$selected.Count;native_link='still required through existing title recipe'}
    $profile | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $BuildDirectory 'build-profile.json')
    Write-BuildState 'OBJECTS_READY' $profile
    Write-Output "Native objects ready: mode=$Mode optimization=O$expected build=$identity; final title link remains required."
} finally {$lock.Dispose()}
