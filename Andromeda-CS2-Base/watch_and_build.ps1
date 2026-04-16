# ============================================
# PowerShell Watcher para Auto-Build
# ============================================
# Uso: .\watch_and_build.ps1 -ModulePath "modules/aimbot" -BuildScript ".\build_fast.bat"
# ============================================

param(
    [string]$ModulePath = "modules",
    [string]$BuildScript = ".\build_fast.bat",
    [int]$DebounceMs = 1000,
    [string]$Filter = "*.cpp,*.hpp,*.h"
)

$ErrorActionPreference = "Stop"

Write-Host "============================================" -ForegroundColor Green
Write-Host "  Andromeda CS2 - Watch & Build System" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host ""
Write-Host "Monitoring: $ModulePath" -ForegroundColor Cyan
Write-Host "Filter: $Filter" -ForegroundColor Cyan
Write-Host "Debounce: ${DebounceMs}ms" -ForegroundColor Cyan
Write-Host "Build Script: $BuildScript" -ForegroundColor Cyan
Write-Host ""
Write-Host "Press Ctrl+C to stop" -ForegroundColor Yellow
Write-Host ""

# Criar FileSystemWatcher
$watcher = New-Object System.IO.FileSystemWatcher
$watcher.Path = (Resolve-Path $ModulePath).Path
$watcher.Filter = "*.*"
$watcher.IncludeSubdirectories = $true
$watcher.EnableRaisingEvents = $true

# Timer para debounce
$timer = New-Object System.Timers.Timer
$timer.Interval = $DebounceMs
$timer.AutoReset = $false

$changedFiles = @{}
$isBuilding = $false

# Ação quando arquivo muda
$action = {
    param($sender, $event)
    
    $file = $event.SourceEventArgs.FullPath
    $changeType = $event.SourceEventArgs.ChangeType
    
    # Ignorar arquivos temporários e de build
    if ($file -match "\.(tmp|obj|pch|ipdb|pdb)$") { return }
    if ($file -match "build_" ) { return }
    
    # Adicionar à lista de mudanças
    $script:changedFiles[$file] = $changeType
    
    # Reset timer
    $script:timer.Stop()
    $script:timer.Start()
    
    Write-Host "[$([DateTime]::Now.ToString('HH:mm:ss'))] Changed: $file ($changeType)" -ForegroundColor Gray
}

# Ação quando timer dispara (build)
$timerAction = {
    if ($script:isBuilding) {
        Write-Host "Build already in progress, skipping..." -ForegroundColor Yellow
        return
    }
    
    if ($script:changedFiles.Count -eq 0) { return }
    
    $script:isBuilding = $true
    
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "  Building..." -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    
    $filesChanged = ($script:changedFiles.Keys | ForEach-Object { Split-Path $_ -Leaf }) -join ", "
    Write-Host "Files: $filesChanged" -ForegroundColor Gray
    
    # Extrair nome do módulo do path
    $firstFile = $script:changedFiles.Keys[0]
    $moduleName = Split-Path (Split-Path $firstFile -Parent) -Leaf
    
    try {
        # Executar build script
        & $BuildScript $moduleName
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Build succeeded!" -ForegroundColor Green
            
            # Notificar via log file (opcional)
            $logFile = "$env:TEMP\andromeda_build.log"
            "$(Get-Date): Module $moduleName rebuilt successfully" | Out-File -Append $logFile
        } else {
            Write-Host "Build failed with exit code $LASTEXITCODE" -ForegroundColor Red
        }
    } catch {
        Write-Host "Build error: $_" -ForegroundColor Red
    }
    
    $script:changedFiles.Clear()
    $script:isBuilding = $false
    Write-Host ""
}

# Registrar eventos
Register-ObjectEvent -InputObject $watcher -EventName Changed -Action $action -SourceIdentifier "FileChanged"
Register-ObjectEvent -InputObject $watcher -EventName Created -Action $action -SourceIdentifier "FileCreated"
Register-ObjectEvent -InputObject $watcher -EventName Renamed -Action $action -SourceIdentifier "FileRenamed"

# Configurar timer
$timer.Add_Elapsed($timerAction)

Write-Host "Watcher started. Waiting for changes..." -ForegroundColor Green
Write-Host ""

# Manter script rodando
try {
    while ($true) {
        Start-Sleep -Milliseconds 500
    }
} finally {
    # Cleanup
    $watcher.Dispose()
    $timer.Dispose()
    Unregister-Event -SourceIdentifier "FileChanged" -ErrorAction SilentlyContinue
    Unregister-Event -SourceIdentifier "FileCreated" -ErrorAction SilentlyContinue
    Unregister-Event -SourceIdentifier "FileRenamed" -ErrorAction SilentlyContinue
    
    Write-Host ""
    Write-Host "Watcher stopped." -ForegroundColor Yellow
}
