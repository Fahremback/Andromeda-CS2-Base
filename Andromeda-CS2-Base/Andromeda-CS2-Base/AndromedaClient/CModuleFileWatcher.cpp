#include "CModuleFileWatcher.hpp"
#include "CModuleManager.hpp"
#include <Common/CrashLog.hpp>
#include <algorithm>

static CModuleFileWatcher g_ModuleWatcher;

auto CModuleFileWatcher::Get() -> CModuleFileWatcher*
{
    return &g_ModuleWatcher;
}

auto GetModuleWatcher() -> CModuleFileWatcher*
{
    return CModuleFileWatcher::Get();
}

auto CModuleFileWatcher::Initialize() -> void
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (m_Initialized)
        return;
    
    m_WatchedFiles.clear();
    m_Running = false;
    m_AutoReloadEnabled = true;
    m_Initialized = true;
    
    DEV_LOG("[ModuleWatcher] System initialized\n");
}

auto CModuleFileWatcher::Shutdown() -> void
{
    StopWatching();
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (!m_Initialized)
        return;
    
    m_WatchedFiles.clear();
    m_Initialized = false;
    
    DEV_LOG("[ModuleWatcher] System shutdown\n");
}

auto CModuleFileWatcher::StartWatching(const std::string& directory, bool recursive) -> void
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (m_Running)
    {
        DEV_LOG("[ModuleWatcher] Already running\n");
        return;
    }
    
    m_WatchDirectory = directory;
    
    // Escanear diretório inicial
    try
    {
        if (!std::filesystem::exists(directory))
        {
            std::filesystem::create_directories(directory);
            DEV_LOG("[ModuleWatcher] Created directory: %s\n", directory.c_str());
        }
        
        // Adicionar todos os DLLs do diretório
        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".dll")
            {
                std::string path = entry.path().string();
                
                // Ignorar bridge e logic
                if (path.find("Andromeda-Bridge") != std::string::npos ||
                    path.find("Andromeda-Logic") != std::string::npos)
                {
                    continue;
                }
                
                AddFileToWatch(path);
            }
        }
    }
    catch (const std::exception& e)
    {
        DEV_LOG("[ModuleWatcher] Error scanning directory: %s\n", e.what());
    }
    
    // Iniciar thread de monitoramento
    m_Running = true;
    m_WatcherThread = std::make_unique<std::thread>(&CModuleFileWatcher::WatcherThreadFunc, this);
    
    DEV_LOG("[ModuleWatcher] Started watching directory: %s (%d files)\n", 
            directory.c_str(), (int)m_WatchedFiles.size());
}

auto CModuleFileWatcher::StopWatching() -> void
{
    m_Running = false;
    
    if (m_WatcherThread && m_WatcherThread->joinable())
    {
        m_WatcherThread->join();
        m_WatcherThread.reset();
    }
    
    DEV_LOG("[ModuleWatcher] Stopped watching\n");
}

auto CModuleFileWatcher::AddFileToWatch(const std::string& filePath) -> void
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (m_WatchedFiles.find(filePath) != m_WatchedFiles.end())
    {
        return; // Já está sendo monitorado
    }
    
    ModuleFileWatch watch;
    watch.filePath = filePath;
    watch.moduleName = GetModuleFromFilePath(filePath);
    watch.autoReloadEnabled = true;
    watch.isBeingReloaded = false;
    watch.reloadCount = 0;
    
    try
    {
        watch.lastWriteTime = std::filesystem::last_write_time(filePath);
    }
    catch (...)
    {
        watch.lastWriteTime = std::filesystem::file_time_type::min();
    }
    
    m_WatchedFiles[filePath] = watch;
    
    DEV_LOG("[ModuleWatcher] Added file to watch: %s (module: %s)\n", 
            filePath.c_str(), watch.moduleName.c_str());
}

auto CModuleFileWatcher::RemoveFileFromWatch(const std::string& filePath) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_WatchedFiles.find(filePath);
    if (it == m_WatchedFiles.end())
        return false;
    
    m_WatchedFiles.erase(it);
    return true;
}

auto CModuleFileWatcher::EnableAutoReload(bool enabled) -> void
{
    m_AutoReloadEnabled = enabled;
    DEV_LOG("[ModuleWatcher] Auto-reload %s\n", enabled ? "enabled" : "disabled");
}

auto CModuleFileWatcher::IsAutoReloadEnabled() const -> bool
{
    return m_AutoReloadEnabled;
}

auto CModuleFileWatcher::SetCheckInterval(int intervalMs) -> void
{
    m_CheckIntervalMs = std::max(100, intervalMs); // Mínimo 100ms
    DEV_LOG("[ModuleWatcher] Check interval set to %dms\n", m_CheckIntervalMs);
}

auto CModuleFileWatcher::SetMinReloadDelay(int delayMs) -> void
{
    m_MinReloadDelayMs = std::max(500, delayMs); // Mínimo 500ms
    DEV_LOG("[ModuleWatcher] Min reload delay set to %dms\n", m_MinReloadDelayMs);
}

auto CModuleFileWatcher::ForceReloadModule(const std::string& moduleName) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    // Encontrar arquivo do módulo
    std::string filePath;
    for (const auto& [path, watch] : m_WatchedFiles)
    {
        if (watch.moduleName == moduleName)
        {
            filePath = path;
            break;
        }
    }
    
    if (filePath.empty())
    {
        DEV_LOG("[ModuleWatcher] Module not found for force reload: %s\n", moduleName.c_str());
        return false;
    }
    
    // Liberar lock antes de recarregar
    m_Mutex.unlock();
    bool result = ReloadModule(moduleName, filePath);
    m_Mutex.lock();
    
    return result;
}

auto CModuleFileWatcher::GetWatchStatus() -> std::vector<ModuleFileWatch*>
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    std::vector<ModuleFileWatch*> status;
    for (auto& pair : m_WatchedFiles)
    {
        status.push_back(&pair.second);
    }
    
    return status;
}

auto CModuleFileWatcher::SetOnModuleReloadedCallback(std::function<void(const std::string&, bool)> callback) -> void
{
    m_OnModuleReloaded = callback;
}

auto CModuleFileWatcher::SetOnReloadErrorCallback(std::function<void(const std::string&, const std::string&)> callback) -> void
{
    m_OnReloadError = callback;
}

auto CModuleFileWatcher::IsModuleBeingReloaded(const std::string& moduleName) const -> bool
{
    for (const auto& pair : m_WatchedFiles)
    {
        if (pair.second.moduleName == moduleName && pair.second.isBeingReloaded)
        {
            return true;
        }
    }
    return false;
}

auto CModuleFileWatcher::GetReloadCount(const std::string& moduleName) -> int
{
    for (const auto& pair : m_WatchedFiles)
    {
        if (pair.second.moduleName == moduleName)
        {
            return pair.second.reloadCount;
        }
    }
    return 0;
}

auto CModuleFileWatcher::WatcherThreadFunc() -> void
{
    DEV_LOG("[ModuleWatcher] Watcher thread started\n");
    
    while (m_Running)
    {
        CheckForChanges();
        
        // Sleep pelo intervalo configurado
        std::this_thread::sleep_for(std::chrono::milliseconds(m_CheckIntervalMs));
    }
    
    DEV_LOG("[ModuleWatcher] Watcher thread stopped\n");
}

auto CModuleFileWatcher::CheckForChanges() -> void
{
    std::vector<std::pair<std::string, std::string>> changesDetected;
    
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        for (auto& [filePath, watch] : m_WatchedFiles)
        {
            if (!watch.autoReloadEnabled || !m_AutoReloadEnabled)
                continue;
            
            if (watch.isBeingReloaded)
            {
                // Verificar se já passou o tempo mínimo
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - watch.lastReloadTime).count();
                
                if (elapsed < m_MinReloadDelayMs)
                    continue;
                
                // Parar flag de reload
                watch.isBeingReloaded = false;
            }
            
            try
            {
                auto currentTime = std::filesystem::last_write_time(filePath);
                
                if (currentTime != watch.lastWriteTime)
                {
                    // Arquivo mudou!
                    watch.lastWriteTime = currentTime;
                    
                    // Verificar delay mínimo desde última recarga
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - watch.lastReloadTime).count();
                    
                    if (elapsed >= m_MinReloadDelayMs)
                    {
                        DEV_LOG("[ModuleWatcher] Change detected: %s\n", filePath.c_str());
                        changesDetected.push_back({watch.moduleName, filePath});
                    }
                }
            }
            catch (const std::exception& e)
            {
                // Arquivo pode ter sido deletado ou estar em uso
                DEV_LOG("[ModuleWatcher] Error checking file %s: %s\n", filePath.c_str(), e.what());
            }
        }
    }
    
    // Processar mudanças (fora do lock)
    for (const auto& [moduleName, filePath] : changesDetected)
    {
        ReloadModule(moduleName, filePath);
    }
}

auto CModuleFileWatcher::ReloadModule(const std::string& moduleName, const std::string& filePath) -> bool
{
    // Marcar como sendo recarregado
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_WatchedFiles.find(filePath);
        if (it != m_WatchedFiles.end())
        {
            it->second.isBeingReloaded = true;
            it->second.lastReloadTime = std::chrono::steady_clock::now();
            it->second.reloadCount++;
        }
    }
    
    DEV_LOG("[ModuleWatcher] Reloading module: %s (attempt %d)\n", 
            moduleName.c_str(), 
            m_WatchedFiles[filePath].reloadCount);
    
    // Usar ModuleManager para recarregar
    auto* moduleManager = GetModuleManager();
    if (!moduleManager)
    {
        DEV_LOG("[ModuleWatcher] ModuleManager not available\n");
        
        if (m_OnReloadError)
            m_OnReloadError(moduleName, "ModuleManager not available");
        
        return false;
    }
    
    bool success = false;
    
    try
    {
        // Tentar recarregar
        success = moduleManager->ReloadModule(moduleName);
        
        if (success)
        {
            DEV_LOG("[ModuleWatcher] Module reloaded successfully: %s\n", moduleName.c_str());
            
            if (m_OnModuleReloaded)
                m_OnModuleReloaded(moduleName, true);
        }
        else
        {
            DEV_LOG("[ModuleWatcher] Failed to reload module: %s\n", moduleName.c_str());
            
            std::string error = "ReloadModule returned false";
            if (m_OnReloadError)
                m_OnReloadError(moduleName, error);
        }
    }
    catch (const std::exception& e)
    {
        DEV_LOG("[ModuleWatcher] Exception during reload of %s: %s\n", moduleName.c_str(), e.what());
        
        if (m_OnReloadError)
            m_OnReloadError(moduleName, e.what());
        
        success = false;
    }
    
    // Atualizar status
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_WatchedFiles.find(filePath);
        if (it != m_WatchedFiles.end())
        {
            it->second.isBeingReloaded = false;
            it->second.lastError = success ? "" : "Reload failed";
        }
    }
    
    return success;
}

auto CModuleFileWatcher::GetModuleFromFilePath(const std::string& filePath) -> std::string
{
    try
    {
        std::filesystem::path path(filePath);
        return path.stem().string();
    }
    catch (...)
    {
        return "unknown";
    }
}
