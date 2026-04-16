#include "CModuleManager.hpp"
#include <DllLauncher.hpp>
#include <Common/CrashLog.hpp>
#include <filesystem>
#include <algorithm>

static CModuleManager g_ModuleManager;

auto CModuleManager::Initialize() -> void
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    if (m_Initialized)
        return;
    
    m_Modules.clear();
    m_Initialized = true;
    
    DEV_LOG("[ModuleManager] Initialized\n");
}

auto CModuleManager::Shutdown() -> void
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    if (!m_Initialized)
        return;
    
    // Descarregar todos os módulos
    for (auto it = m_Modules.begin(); it != m_Modules.end();)
    {
        UnloadModuleInternal(it->second);
        it = m_Modules.erase(it);
    }
    
    m_Initialized = false;
    DEV_LOG("[ModuleManager] Shutdown complete\n");
}

auto CModuleManager::LoadModule(const std::string& path) -> bool
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    if (!m_Initialized)
    {
        DEV_LOG("[ModuleManager] Error: Not initialized\n");
        return false;
    }
    
    // Verificar se arquivo existe
    if (!std::filesystem::exists(path))
    {
        DEV_LOG("[ModuleManager] Error: Module file not found: %s\n", path.c_str());
        return false;
    }
    
    // Extrair nome do módulo (sem path e extensão)
    std::filesystem::path fsPath(path);
    std::string moduleName = fsPath.stem().string();
    
    // Verificar se já está carregado
    if (m_Modules.find(moduleName) != m_Modules.end())
    {
        DEV_LOG("[ModuleManager] Error: Module already loaded: %s\n", moduleName.c_str());
        return false;
    }
    
    // Carregar DLL
    HMODULE hModule = LoadLibraryA(path.c_str());
    if (!hModule)
    {
        DWORD error = GetLastError();
        DEV_LOG("[ModuleManager] Error: Failed to load module %s (Error: %lu)\n", moduleName.c_str(), error);
        return false;
    }
    
    // Obter função de exportação GetModuleInstance
    GetModuleInstanceFn pGetInstance = (GetModuleInstanceFn)GetProcAddress(hModule, "GetModuleInstance");
    if (!pGetInstance)
    {
        DEV_LOG("[ModuleManager] Error: Module %s missing GetModuleInstance export\n", moduleName.c_str());
        FreeLibrary(hModule);
        return false;
    }
    
    // Obter instância do módulo
    IAndromedaModule* pInstance = pGetInstance();
    if (!pInstance)
    {
        DEV_LOG("[ModuleManager] Error: Module %s returned null instance\n", moduleName.c_str());
        FreeLibrary(hModule);
        return false;
    }
    
    // Validar módulo
    if (!ValidateModule(pInstance))
    {
        DEV_LOG("[ModuleManager] Error: Module %s failed validation\n", moduleName.c_str());
        FreeLibrary(hModule);
        return false;
    }
    
    // Inicializar módulo
    if (!pInstance->OnInit(m_SharedState))
    {
        DEV_LOG("[ModuleManager] Error: Module %s OnInit failed\n", moduleName.c_str());
        FreeLibrary(hModule);
        return false;
    }
    
    // Adicionar à lista de módulos carregados
    LoadedModule loadedModule;
    loadedModule.name = moduleName;
    loadedModule.path = path;
    loadedModule.handle = hModule;
    loadedModule.instance = pInstance;
    loadedModule.info = pInstance->GetModuleInfo();
    loadedModule.state = ModuleState::Loaded;
    loadedModule.enabled = true;
    loadedModule.loadTime = std::chrono::steady_clock::now();
    
    m_Modules[moduleName] = loadedModule;
    
    DEV_LOG("[ModuleManager] Module loaded successfully: %s v%s by %s\n", 
            loadedModule.info.name, 
            loadedModule.info.version, 
            loadedModule.info.author);
    
    return true;
}

auto CModuleManager::UnloadModule(const std::string& name) -> bool
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    auto it = m_Modules.find(name);
    if (it == m_Modules.end())
    {
        DEV_LOG("[ModuleManager] Error: Module not found: %s\n", name.c_str());
        return false;
    }
    
    bool result = UnloadModuleInternal(it->second);
    if (result)
    {
        m_Modules.erase(it);
    }
    
    return result;
}

auto CModuleManager::UnloadModuleInternal(LoadedModule& module) -> bool
{
    try
    {
        module.state = ModuleState::Unloading;
        
        // Chamar OnDestroy
        if (module.instance)
        {
            module.instance->OnDestroy();
        }
        
        // Liberar DLL
        if (module.handle)
        {
            FreeLibrary(module.handle);
        }
        
        module.state = ModuleState::Unloaded;
        DEV_LOG("[ModuleManager] Module unloaded: %s\n", module.name.c_str());
        
        return true;
    }
    catch (...)
    {
        DEV_LOG("[ModuleManager] Error: Exception during unload of %s\n", module.name.c_str());
        return false;
    }
}

auto CModuleManager::ReloadModule(const std::string& name) -> bool
{
    std::string path;
    
    {
        std::lock_guard<std::mutex> lock(m_ModuleMutex);
        
        auto it = m_Modules.find(name);
        if (it == m_Modules.end())
        {
            DEV_LOG("[ModuleManager] Error: Module not found for reload: %s\n", name.c_str());
            return false;
        }
        
        path = it->second.path;
    }
    
    // Descarregar
    if (!UnloadModule(name))
    {
        return false;
    }
    
    // Pequeno delay para garantir que DLL foi liberada
    Sleep(100);
    
    // Carregar novamente
    return LoadModule(path);
}

auto CModuleManager::SetModuleEnabled(const std::string& name, bool enabled) -> bool
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    auto it = m_Modules.find(name);
    if (it == m_Modules.end())
    {
        return false;
    }
    
    it->second.enabled = enabled;
    it->second.instance->SetEnabled(enabled);
    
    DEV_LOG("[ModuleManager] Module %s %s\n", name.c_str(), enabled ? "enabled" : "disabled");
    
    return true;
}

auto CModuleManager::GetLoadedModules() -> std::vector<LoadedModule*>
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    std::vector<LoadedModule*> modules;
    for (auto& pair : m_Modules)
    {
        modules.push_back(&pair.second);
    }
    
    return modules;
}

auto CModuleManager::IsModuleLoaded(const std::string& name) -> bool
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    return m_Modules.find(name) != m_Modules.end();
}

auto CModuleManager::GetModule(const std::string& name) -> IAndromedaModule*
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    auto it = m_Modules.find(name);
    if (it == m_Modules.end())
    {
        return nullptr;
    }
    
    return it->second.instance;
}

auto CModuleManager::ScanForModules(const std::string& directory) -> std::vector<std::string>
{
    std::vector<std::string> modules;
    
    try
    {
        if (!std::filesystem::exists(directory))
        {
            DEV_LOG("[ModuleManager] Directory does not exist: %s\n", directory.c_str());
            return modules;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".dll")
            {
                std::string filename = entry.path().filename().string();
                
                // Ignorar DLLs do sistema e da bridge
                if (filename.find("Andromeda-Logic") != std::string::npos ||
                    filename.find("Andromeda-Bridge") != std::string::npos)
                {
                    continue;
                }
                
                modules.push_back(entry.path().string());
            }
        }
    }
    catch (const std::exception& e)
    {
        DEV_LOG("[ModuleManager] Error scanning directory %s: %s\n", directory.c_str(), e.what());
    }
    
    DEV_LOG("[ModuleManager] Found %d potential modules in %s\n", (int)modules.size(), directory.c_str());
    
    return modules;
}

auto CModuleManager::ValidateModule(IAndromedaModule* instance) -> bool
{
    if (!instance)
        return false;
    
    const ModuleInfo& info = instance->GetModuleInfo();
    
    // Verificar versão da interface
    if (info.interfaceVersion != ANDROMEDA_MODULE_INTERFACE_VERSION)
    {
        DEV_LOG("[ModuleManager] Version mismatch: Module=%u, Expected=%u\n", 
                info.interfaceVersion, ANDROMEDA_MODULE_INTERFACE_VERSION);
        return false;
    }
    
    return true;
}

// Callbacks forwarding
auto CModuleManager::OnFrameStageNotify(int stage) -> void
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    for (auto& pair : m_Modules)
    {
        if (pair.second.enabled && pair.second.instance)
        {
            try
            {
                pair.second.instance->OnFrameStageNotify(stage);
            }
            catch (...)
            {
                DEV_LOG("[ModuleManager] Exception in OnFrameStageNotify: %s\n", pair.first.c_str());
            }
        }
    }
}

auto CModuleManager::OnFireEventClientSide(IGameEvent* pEvent) -> void
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    for (auto& pair : m_Modules)
    {
        if (pair.second.enabled && pair.second.instance)
        {
            try
            {
                pair.second.instance->OnFireEventClientSide(pEvent);
            }
            catch (...)
            {
                DEV_LOG("[ModuleManager] Exception in OnFireEventClientSide: %s\n", pair.first.c_str());
            }
        }
    }
}

auto CModuleManager::OnAddEntity(CEntityInstance* pInst, CS2::CHandle handle) -> void
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    for (auto& pair : m_Modules)
    {
        if (pair.second.enabled && pair.second.instance)
        {
            try
            {
                pair.second.instance->OnAddEntity(pInst, handle);
            }
            catch (...)
            {
                DEV_LOG("[ModuleManager] Exception in OnAddEntity: %s\n", pair.first.c_str());
            }
        }
    }
}

auto CModuleManager::OnRemoveEntity(CEntityInstance* pInst, CS2::CHandle handle) -> void
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    for (auto& pair : m_Modules)
    {
        if (pair.second.enabled && pair.second.instance)
        {
            try
            {
                pair.second.instance->OnRemoveEntity(pInst, handle);
            }
            catch (...)
            {
                DEV_LOG("[ModuleManager] Exception in OnRemoveEntity: %s\n", pair.first.c_str());
            }
        }
    }
}

auto CModuleManager::OnRender() -> void
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    for (auto& pair : m_Modules)
    {
        if (pair.second.enabled && pair.second.instance)
        {
            try
            {
                pair.second.instance->OnRender();
            }
            catch (...)
            {
                DEV_LOG("[ModuleManager] Exception in OnRender: %s\n", pair.first.c_str());
            }
        }
    }
}

auto CModuleManager::OnClientOutput() -> void
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    for (auto& pair : m_Modules)
    {
        if (pair.second.enabled && pair.second.instance)
        {
            try
            {
                pair.second.instance->OnClientOutput();
            }
            catch (...)
            {
                DEV_LOG("[ModuleManager] Exception in OnClientOutput: %s\n", pair.first.c_str());
            }
        }
    }
}

auto CModuleManager::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd) -> void
{
    std::lock_guard<std::mutex> lock(m_ModuleMutex);
    
    for (auto& pair : m_Modules)
    {
        if (pair.second.enabled && pair.second.instance)
        {
            try
            {
                pair.second.instance->OnCreateMove(pInput, pCmd);
            }
            catch (...)
            {
                DEV_LOG("[ModuleManager] Exception in OnCreateMove: %s\n", pair.first.c_str());
            }
        }
    }
}

auto GetModuleManager() -> CModuleManager*
{
    return &g_ModuleManager;
}
