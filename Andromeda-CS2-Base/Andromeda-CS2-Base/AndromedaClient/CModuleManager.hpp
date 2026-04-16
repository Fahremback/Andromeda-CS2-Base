#pragma once

#include <Common/Include/IAndromedaModule.hpp>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <map>

/**
 * @brief Gerenciador de Módulos Dinâmicos Andromeda
 * 
 * Permite carregar/descarregar módulos individualmente sem reiniciar o jogo.
 * Cada módulo é uma DLL separada que implementa IAndromedaModule.
 * 
 * Uso:
 *   - Carregar aimbot: GetModuleManager()->LoadModule("modules/aimbot.dll")
 *   - Descarregar: GetModuleManager()->UnloadModule("aimbot")
 *   - Recarregar: GetModuleManager()->ReloadModule("aimbot")
 */

struct LoadedModule
{
    std::string name;
    std::string path;
    HMODULE handle;
    IAndromedaModule* instance;
    ModuleInfo info;
    ModuleState state;
    bool enabled;
    std::chrono::steady_clock::time_point loadTime;
    std::string lastError;
};

class CModuleManager final
{
public:
    /**
     * @brief Inicializa o gerenciador de módulos
     */
    auto Initialize() -> void;
    
    /**
     * @brief Finaliza e descarrega todos os módulos
     */
    auto Shutdown() -> void;
    
    /**
     * @brief Carrega um módulo de um arquivo DLL
     * @param path Caminho para o arquivo DLL do módulo
     * @return true se carregado com sucesso
     */
    auto LoadModule(const std::string& path) -> bool;
    
    /**
     * @brief Descarrega um módulo pelo nome
     * @param name Nome do módulo (sem extensão)
     * @return true se descarregado com sucesso
     */
    auto UnloadModule(const std::string& name) -> bool;
    
    /**
     * @brief Recarrega um módulo (unload + load)
     * @param name Nome do módulo
     * @return true se recarregado com sucesso
     */
    auto ReloadModule(const std::string& name) -> bool;
    
    /**
     * @brief Habilita ou desabilita um módulo
     * @param name Nome do módulo
     * @param enabled Estado desejado
     * @return true se alterado com sucesso
     */
    auto SetModuleEnabled(const std::string& name, bool enabled) -> bool;
    
    /**
     * @brief Obtém lista de todos os módulos carregados
     * @return Vetor com informações dos módulos
     */
    auto GetLoadedModules() -> std::vector<LoadedModule*>;
    
    /**
     * @brief Verifica se um módulo está carregado
     * @param name Nome do módulo
     * @return true se estiver carregado
     */
    auto IsModuleLoaded(const std::string& name) -> bool;
    
    /**
     * @brief Obtém instância de um módulo carregado
     * @param name Nome do módulo
     * @return Ponteiro para a instância ou nullptr
     */
    auto GetModule(const std::string& name) -> IAndromedaModule*;
    
    /**
     * @brief Escaneia diretório em busca de módulos disponíveis
     * @param directory Diretório para escanear
     * @return Lista de arquivos DLL encontrados
     */
    auto ScanForModules(const std::string& directory) -> std::vector<std::string>;
    
    /**
     * @brief Callbacks que serão encaminhados para todos os módulos ativos
     */
    auto OnFrameStageNotify(int stage) -> void;
    auto OnFireEventClientSide(IGameEvent* pEvent) -> void;
    auto OnAddEntity(CEntityInstance* pInst, CS2::CHandle handle) -> void;
    auto OnRemoveEntity(CEntityInstance* pInst, CS2::CHandle handle) -> void;
    auto OnRender() -> void;
    auto OnClientOutput() -> void;
    auto OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd) -> void;
    
private:
    auto UnloadModuleInternal(LoadedModule& module) -> bool;
    auto ValidateModule(IAndromedaModule* instance) -> bool;
    
private:
    std::map<std::string, LoadedModule> m_Modules;
    std::mutex m_ModuleMutex;
    SharedState* m_SharedState = nullptr;
    bool m_Initialized = false;
};

auto GetModuleManager() -> CModuleManager*;
