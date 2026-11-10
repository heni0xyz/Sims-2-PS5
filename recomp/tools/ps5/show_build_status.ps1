# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
# Read-only dashboard. Closing it does not stop any build or watcher.
param(
    [string]$ProjectRoot = (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent),
    [switch]$Snapshot
)
$ErrorActionPreference = 'Stop'
$compilerDir = Join-Path $ProjectRoot 'out/console-compiler'
$probeDir = Join-Path $ProjectRoot 'out/console-linker-probe'

function Read-Receipt([string]$Path) {
    try { return (Get-Content -LiteralPath $Path -Raw -ErrorAction Stop | ConvertFrom-Json) }
    catch { return $null } # A concurrent writer may be replacing the JSON.
}
function Read-LogTail([string]$Path) {
    $stream = $null
    try {
        $stream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read,
            ([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
        $length = [int][Math]::Min(131072, $stream.Length)
        [void]$stream.Seek(-$length, [IO.SeekOrigin]::End)
        $buffer = New-Object byte[] $length
        $count = $stream.Read($buffer, 0, $length)
        return [Text.Encoding]::UTF8.GetString($buffer, 0, $count)
    } catch { return '' }
    finally { if ($stream) { $stream.Dispose() } }
}
function Get-WorkerState($Receipt) {
    if (!$Receipt -or !$Receipt.pid -or !$Receipt.started) { return 'UNKNOWN' }
    try {
        $worker = [Diagnostics.Process]::GetProcessById([int]$Receipt.pid)
        try {
            if ($worker.HasExited) { return 'STOPPED' }
            $epoch = [DateTime]::SpecifyKind([DateTime]'1970-01-01', [DateTimeKind]::Utc)
            $expected = $epoch.AddSeconds([double]$Receipt.started)
            if ([Math]::Abs(($worker.StartTime.ToUniversalTime() - $expected).TotalSeconds) -gt 20) {
                return 'STOPPED' # A reused PID after a reboot is not this job.
            }
            return 'RUNNING'
        } finally { $worker.Dispose() }
    } catch [ArgumentException] { return 'STOPPED' }
    catch { return 'UNKNOWN' }
}
function Get-BuildSnapshot {
    $linkerBuild = Read-Receipt (Join-Path $compilerDir 'build-linker.json')
    $compilerBuild = Read-Receipt (Join-Path $compilerDir 'build-ps5.json')
    $compilerStage = $compilerBuild -and (!$linkerBuild -or $compilerBuild.started -gt $linkerBuild.started)
    $native = if ($compilerStage) { $compilerBuild } else { $linkerBuild }
    $activeProbeDir = if ($compilerStage) { Join-Path $ProjectRoot 'out/console-compiler-probe' } else { $probeDir }
    $objects = Read-Receipt (Join-Path $activeProbeDir 'build-probe-objects.json')
    $watch = Read-Receipt (Join-Path $activeProbeDir 'state-finish-probe.json')
    $result = Read-Receipt (Join-Path $activeProbeDir 'package-result.json')
    $nativeLog = Join-Path $compilerDir $(if ($compilerStage) { 'build-ps5.log' } else { 'build-linker.log' })
    $logPath = $nativeLog
    if ($watch -and $watch.status -in @('PACKAGING', 'FAIL', 'BUILD_PASS_CONSOLE_NOT_TESTED')) {
        $attempt = Get-ChildItem -LiteralPath $activeProbeDir -Directory -Filter 'package*' -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending | Select-Object -First 1
        if ($attempt) {
            $lastLog = Get-ChildItem -LiteralPath $attempt.FullName -File -Filter '*.log' |
                Sort-Object LastWriteTime -Descending | Select-Object -First 1
            if ($lastLog) { $logPath = $lastLog.FullName }
        }
    }
    $nativeTail = Read-LogTail $nativeLog
    $matches = [regex]::Matches($nativeTail, '(?m)^\[(\d+)/(\d+)\]\s*(.*)')
    $done = 0; $total = 0; $step = 'Oczekiwanie na wpis w logu'
    if ($matches.Count) {
        $latest = $matches[$matches.Count - 1]
        $done = [int]$latest.Groups[1].Value
        $total = [int]$latest.Groups[2].Value
        $step = $latest.Groups[3].Value.Trim()
    }
    $nativeStatus = 'UNKNOWN'
    if ($native) { $nativeStatus = [string]$native.status }
    $workerState = Get-WorkerState $native
    $watchState = Get-WorkerState $watch
    $status = 'RUNNING'; $headline = 'Trwa kompilacja'; $detail = $step
    if ($compilerStage) { $headline = 'Kompilator PS5: trwa budowanie' }
    if (!$native) { $status = 'UNKNOWN'; $headline = 'Odczytywanie statusu...' }
    elseif ($nativeStatus -eq 'FAIL' -or ($watch -and $watch.status -eq 'FAIL')) {
        $status = 'FAIL'; $headline = 'BLAD - wyslij mi log'
        if ($watch.error) { $detail = [string]$watch.error }
        else { $detail = 'Kompilacja zostala zakonczona bledem. Log jest zachowany.' }
    }
    elseif ($nativeStatus -eq 'RUNNING' -and $workerState -eq 'STOPPED') {
        $status = 'STOPPED'; $headline = 'ZATRZYMANY - proces nie dziala'
        $detail = 'Pliki sa zachowane. Napisz do mnie, aby wznowic.'
    }
    elseif ($nativeStatus -eq 'PASS') {
        if ($watch -and $watch.status -eq 'BUILD_PASS_CONSOLE_NOT_TESTED' -and $result -and
            $result.status -eq 'BUILD_PASS_CONSOLE_NOT_TESTED') {
            $status = 'READY'; $headline = 'GOTOWE DO TESTU NA PS5'
            $detail = [string]$result.title
        }
        elseif ($watch -and $watch.status -in @('WAITING', 'PACKAGING') -and $watchState -eq 'STOPPED') {
            $status = 'STOPPED'; $headline = 'PAKOWANIE ZATRZYMANE'
            $detail = 'Biblioteki sa gotowe, ale proces pakowania nie dziala.'
        }
        else {
            if ($compilerStage -and !$watch) {
                $status = 'LIBRARIES_READY'; $headline = 'Biblioteki kompilatora gotowe'
                $detail = 'Nastepny etap: przygotowanie testu kompilacji C++ na PS5.'
            } elseif ($compilerStage -and $watch.status -eq 'WAITING') {
                $headline = 'Kompilator: kontrola koncowa'
                $detail = 'Biblioteki zbudowane. Trwa koncowe sprawdzanie przed pakowaniem.'
            } else {
                $headline = 'Linkowanie i pakowanie testu PS5'
                $detail = 'Biblioteki gotowe. Trwa tworzenie i sprawdzanie paczki.'
            }
        }
    }
    $tail = Read-LogTail $logPath
    $lines = @($tail -split '\r?\n' | Where-Object {
        $_ -match '^\[\d+/\d+\]|FAILED:|\berror:|^ERROR:|^ld.lld:|^Traceback|BuildError|^RuntimeError'
    } | Select-Object -Last 10)
    if (!$lines.Count) { $lines = @($tail -split '\r?\n' | Where-Object { $_.Trim() } | Select-Object -Last 8) }
    $epoch = [DateTime]::SpecifyKind([DateTime]'1970-01-01', [DateTimeKind]::Utc)
    $elapsed = [TimeSpan]::Zero
    if ($native.started) {
        if ($native.seconds -and $nativeStatus -in @('PASS', 'FAIL')) { $elapsed = [TimeSpan]::FromSeconds($native.seconds) }
        else { $elapsed = [DateTime]::UtcNow - $epoch.AddSeconds([double]$native.started) }
    }
    $age = 0
    try { $age = [int]([DateTime]::UtcNow - (Get-Item -LiteralPath $logPath).LastWriteTimeUtc).TotalSeconds } catch {}
    $interfaces = @('embedded', 'linker', 'worker', 'fatal')
    $interfacePass = @($interfaces | Where-Object {
        (Read-Receipt (Join-Path $compilerDir ('check-interface-' + $_ + '.json'))).status -eq 'PASS'
    }).Count -eq 4
    $objectStatus = 'UNKNOWN'; if ($objects) { $objectStatus = [string]$objects.status }
    $packageStatus = 'WAITING'; if ($watch) { $packageStatus = [string]$watch.status }
    if ($status -eq 'READY') { $packageStatus = 'PASS' }
    return [pscustomobject]@{
        status=$status; headline=$headline; detail=$detail; done=$done; total=$total;
        stage=$(if ($compilerStage) { 'compiler' } else { 'linker' });
        percent=$(if ($nativeStatus -eq 'PASS') { 100 } elseif ($total) { [int][Math]::Min(100, 100 * $done / $total) } else { 0 });
        elapsed=('{0:00}:{1:00}:{2:00}' -f [int][Math]::Floor($elapsed.TotalHours), $elapsed.Minutes, $elapsed.Seconds);
        log_age_seconds=$age; log=$logPath; lines=($lines -join [Environment]::NewLine);
        worker=$workerState; watcher=$watchState; native_pid=$native.pid; watcher_pid=$watch.pid;
        interface_status=$(if ($interfacePass) { 'PASS' } else { 'UNKNOWN' });
        object_status=$objectStatus; native_status=$nativeStatus; package_status=$packageStatus;
        title=$result.title; probe_directory=$activeProbeDir
    }
}
if ($Snapshot) { Get-BuildSnapshot | ConvertTo-Json -Depth 4; exit }

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()
$form = New-Object Windows.Forms.Form
$form.Text = 'Sims 2 - Status kompilacji'
$form.ClientSize = New-Object Drawing.Size(880, 650)
$form.MinimumSize = New-Object Drawing.Size(760, 650)
$form.StartPosition = 'CenterScreen'
$form.FormBorderStyle = 'FixedSingle'
$form.MaximizeBox = $false
$form.Font = New-Object Drawing.Font('Segoe UI', 11)
$form.BackColor = [Drawing.Color]::FromArgb(246, 248, 251)
$headlineLabel = New-Object Windows.Forms.Label
$headlineLabel.Location = New-Object Drawing.Point(24, 20)
$headlineLabel.Size = New-Object Drawing.Size(832, 42)
$headlineLabel.Font = New-Object Drawing.Font('Segoe UI', 21, [Drawing.FontStyle]::Bold)
$form.Controls.Add($headlineLabel)
$progress = New-Object Windows.Forms.ProgressBar
$progress.Location = New-Object Drawing.Point(24, 76)
$progress.Size = New-Object Drawing.Size(832, 26)
$form.Controls.Add($progress)
$countLabel = New-Object Windows.Forms.Label
$countLabel.Location = New-Object Drawing.Point(24, 112)
$countLabel.Size = New-Object Drawing.Size(832, 26)
$form.Controls.Add($countLabel)
$detailLabel = New-Object Windows.Forms.Label
$detailLabel.Location = New-Object Drawing.Point(24, 147)
$detailLabel.Size = New-Object Drawing.Size(832, 48)
$detailLabel.AutoEllipsis = $true
$form.Controls.Add($detailLabel)
$list = New-Object Windows.Forms.ListView
$list.Location = New-Object Drawing.Point(24, 206)
$list.Size = New-Object Drawing.Size(832, 150)
$list.View = 'Details'; $list.FullRowSelect = $true; $list.HeaderStyle = 'Nonclickable'
[void]$list.Columns.Add('Etap', 525)
[void]$list.Columns.Add('Wynik', 275)
foreach ($name in @('1. Interfejs kompilatora i linkera PS5', '2. Aplikacja testowa z ekranem i logowaniem',
    '3. Biblioteki linkera (LLD / LLVM)', '4. Linkowanie, sprawdzenie i paczka PPSA99146')) {
    $item = New-Object Windows.Forms.ListViewItem($name)
    [void]$item.SubItems.Add('...'); [void]$list.Items.Add($item)
}
$form.Controls.Add($list)
$logLabel = New-Object Windows.Forms.Label
$logLabel.Location = New-Object Drawing.Point(24, 369)
$logLabel.Size = New-Object Drawing.Size(832, 24)
$form.Controls.Add($logLabel)
$logBox = New-Object Windows.Forms.TextBox
$logBox.Location = New-Object Drawing.Point(24, 399)
$logBox.Size = New-Object Drawing.Size(832, 145)
$logBox.Multiline = $true; $logBox.ReadOnly = $true; $logBox.ScrollBars = 'Both'; $logBox.WordWrap = $false
$logBox.Font = New-Object Drawing.Font('Consolas', 9)
$form.Controls.Add($logBox)
$openLog = New-Object Windows.Forms.Button
$openLog.Text = 'Otworz log'; $openLog.Location = New-Object Drawing.Point(24, 560)
$openLog.Size = New-Object Drawing.Size(150, 34)
$openLog.Add_Click({ if ($script:lastSnapshot.log) { Start-Process notepad.exe -ArgumentList ('"' + $script:lastSnapshot.log + '"') } })
$form.Controls.Add($openLog)
$openFolder = New-Object Windows.Forms.Button
$openFolder.Text = 'Otworz folder'; $openFolder.Location = New-Object Drawing.Point(188, 560)
$openFolder.Size = New-Object Drawing.Size(150, 34)
$openFolder.Add_Click({
    $target = if ($script:lastSnapshot.probe_directory) { $script:lastSnapshot.probe_directory } else { $probeDir }
    Start-Process explorer.exe -ArgumentList ('"' + $target + '"')
})
$form.Controls.Add($openFolder)
$footer = New-Object Windows.Forms.Label
$footer.Text = 'Odswiezanie co 2 s. Zamkniecie okna nie zatrzymuje buildu. Wynik dotyczy etapu pokazanego w oknie.'
$footer.Location = New-Object Drawing.Point(24, 607)
$footer.Size = New-Object Drawing.Size(832, 38)
$footer.Font = New-Object Drawing.Font('Segoe UI', 9)
$form.Controls.Add($footer)

function Update-Dashboard {
    try {
        $s = Get-BuildSnapshot
        $script:lastSnapshot = $s
        $headlineLabel.Text = $s.headline
        $headlineLabel.ForeColor = switch ($s.status) {
            'READY' { [Drawing.Color]::FromArgb(20, 132, 70) }
            'LIBRARIES_READY' { [Drawing.Color]::FromArgb(20, 132, 70) }
            'FAIL' { [Drawing.Color]::FromArgb(190, 35, 45) }
            'STOPPED' { [Drawing.Color]::FromArgb(190, 35, 45) }
            default { [Drawing.Color]::FromArgb(35, 80, 175) }
        }
        $progress.Value = $s.percent
        $subject = if ($s.stage -eq 'compiler') { 'Kompilator' } else { 'Biblioteki linkera' }
        $countLabel.Text = if ($s.native_status -eq 'PASS') {
            $subject + ': PASS (gotowe)    |    Czas ostatniej kompilacji: ' + $s.elapsed
        } else {
            '{0}: {1} / {2} krokow ({3}%)    |    Czas buildu: {4}' -f $subject, $s.done, $s.total, $s.percent, $s.elapsed
        }
        $list.Items[2].Text = if ($s.stage -eq 'compiler') { '3. Kompilator PS5 (Clang / LLVM)' } else { '3. Biblioteki linkera (LLD / LLVM)' }
        $list.Items[3].Text = if ($s.stage -eq 'compiler') { '4. Linkowanie, sprawdzenie i paczka PPSA99147' } else { '4. Linkowanie, sprawdzenie i paczka PPSA99146' }
        $detailLabel.Text = $s.detail
        $states = @($s.interface_status, $s.object_status, $s.native_status, $s.package_status)
        for ($i = 0; $i -lt 4; $i++) {
            $state = $states[$i]
            $label = switch ($state) {
                'PASS' { 'PASS' }
                'BUILD_PASS_CONSOLE_NOT_TESTED' { 'PASS - do testu' }
                'FAIL' { 'FAIL' }
                'RUNNING' { 'W TRAKCIE' }
                'PACKAGING' { 'W TRAKCIE' }
                'WAITING' { 'OCZEKUJE' }
                default { 'Brak potwierdzenia' }
            }
            if ($i -eq 2 -and $s.status -eq 'STOPPED' -and $s.native_status -eq 'RUNNING') { $label = 'ZATRZYMANY' }
            $list.Items[$i].SubItems[1].Text = $label
            $list.Items[$i].ForeColor = if ($state -eq 'PASS' -or $state -eq 'BUILD_PASS_CONSOLE_NOT_TESTED') {
                [Drawing.Color]::FromArgb(20, 132, 70)
            } elseif ($state -eq 'FAIL' -or $label -eq 'ZATRZYMANY') {
                [Drawing.Color]::FromArgb(190, 35, 45)
            } else { [Drawing.Color]::FromArgb(70, 80, 95) }
        }
        $activity = if ($s.native_status -eq 'RUNNING') { 'Proces: ' + $s.worker } else { 'Pakowanie: ' + $s.watcher }
        $logLabel.Text = 'Ostatnie kroki  |  ' + $activity + '  |  Ostatni wpis: ' + $s.log_age_seconds + ' s temu'
        if ($logBox.Text -ne $s.lines) { $logBox.Text = $s.lines }
        $s | Add-Member -NotePropertyName monitor_pid -NotePropertyValue $PID
        $s | Add-Member -NotePropertyName refreshed_utc -NotePropertyValue ([DateTime]::UtcNow.ToString('o'))
        $s | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $probeDir 'build-monitor-state.json') -Encoding UTF8
    } catch {
        $headlineLabel.Text = 'Nie udalo sie odczytac statusu'
        $detailLabel.Text = $_.Exception.Message
    }
}
$timer = New-Object Windows.Forms.Timer
$timer.Interval = 2000
$timer.Add_Tick({ Update-Dashboard })
$form.Add_Shown({ Update-Dashboard; $timer.Start(); $form.Activate() })
$form.Add_FormClosed({ $timer.Stop(); $timer.Dispose() })
[void]$form.ShowDialog()
$form.Dispose()
