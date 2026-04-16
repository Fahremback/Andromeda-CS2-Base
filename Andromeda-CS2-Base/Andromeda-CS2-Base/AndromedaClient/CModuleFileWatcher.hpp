#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <chrono>

/**
 * @brief Sistema de Auto-Reload de Módulos via File Watcher
 * 
 * Monitora arquivos DLL em tempo real e recarrega módulos automaticamente
 * quando detecta mudanças. Elimina a necessidade de reinjetar manualmente.
 * 
 * Uso:
 *   GetModuleWatcher()->StartWatching("modules/");
 *   GetModuleWatcher()->EnableAutoReload(true);
 */

struct ModuleFileWatch
{
    std::string filePath;
    std::string moduleName;
    std::filesystem::file_time_type lastWriteTime;
    bool autoReloadEnabled = true;
    bool isBeingReloaded = false;
    std::chrono::steady_clock::time_point lastReloadTime;
    int reloadCount = 0;
    std::string lastError;
};

class CModuleFileWatcher final
{
public:
    /**
     * @brief Obtém instância singleton
     */
    static auto Get() -> CModuleFileWatcher*;
    
    /**
     * @brief Inicializa o watcher
     */
    auto Initialize() -> void;
    
    /**
     * @brief Finaliza o watcher e para threads
     */
    auto Shutdown() -> void;
    
    /**
     * @brief Inicia monitoramento de um diretório
     * @param directory Diretório para monitorar
     * @param recursive Se true, monitora subdiretórios também
     */
    auto StartWatching(const std::string& directory, bool recursive = false) -> void;
    
    /**
     * @brief Para monitoramento
     */
    auto StopWatching() -> void;
    
    /**
     * @brief Adiciona arquivo específico para monitorar
     * @param filePath Caminho completo do arquivo
     */
    auto AddFileToWatch(const std::string& filePath) -> void;
    
    /**
     * @brief Remove arquivo do monitoramento
     * @param filePath Caminho do arquivo
     */
    auto RemoveFileFromWatch(const std::string& filePath) -> bool;
    
    /**
     * @brief Habilita/desabilita auto-reload
     * @param enabled Se true, recarrega automaticamente
     */
    auto EnableAutoReload(bool enabled) -> void;
    
    /**
     * @brief Verifica se auto-reload está habilitado
     */
    auto IsAutoReloadEnabled() const -> bool;
    
    /**
     * @brief Define intervalo de verificação (em ms)
     * @param intervalMs Intervalo em milissegundos
     */
    auto SetCheckInterval(int intervalMs) -> void;
    
    /**
     * @brief Define delay mínimo entre recargas (evita loops)
     * @param delayMs Delay em milissegundos
     */
    auto SetMinReloadDelay(int delayMs) -> void;
    
    /**
     * @brief Força recarga de um módulo específico
     * @param moduleName Nome do módulo
     * @return true se recarregado
     */
    auto ForceReloadModule(const std::string& moduleName) -> bool;
    
    /**
     * @brief Obtém status de todos os arquivos monitorados
     */
    auto GetWatchStatus() -> std::vector<ModuleFileWatch*>;
    
    /**
     * @brief Define callback para quando um módulo for recarregado
     */
    auto SetOnModuleReloadedCallback(std::function<void(const std::string& moduleName, bool success)> callback) -> void;
    
    /**
     * @brief Define callback para erro no reload
     */
    auto SetOnReloadErrorCallback(std::function<void(const std::string& moduleName, const std::string& error)> callback) -> void;
    
    /**
     * @brief Verifica se um módulo está sendo recarregado
     */
    auto IsModuleBeingReloaded(const std::string& moduleName) const -> bool;
    
    /**
     * @brief Obtém contador de recargas de um módulo
     */
    auto GetReloadCount(const std::string& moduleName) -> int;
    
private:
    auto WatcherThreadFunc() -> void;
    auto CheckForChanges() -> void;
    auto ReloadModule(const std::string& moduleName, const std::string& filePath) -> bool;
    auto GetModuleFromFilePath(const std::string& filePath) -> std::string;
    
private:
    std::map<std::string, ModuleFileWatch> m_WatchedFiles;
    std::string m_WatchDirectory;
    std::atomic<bool> m_Running{false};
    std::atomic<bool> m_AutoReloadEnabled{true};
    std::unique_ptr<std::thread> m_WatcherThread;
    std::mutex m_Mutex;
    
    int m_CheckIntervalMs = 500;          // Verifica a cada 500ms
    int m_MinReloadDelayMs = 2000;        // Mínimo 2s entre recargas
    int m_MaxReloadAttempts = 3;          // Máximo de tentativas
    
    std::function<void(const std::string&, bool)> m_OnModuleReloaded;
    std::function<void(const std::string&, const std::string&)> m_OnReloadError;
    
    bool m_Initialized = false;
};

// Helper function
auto GetModuleWatcher() -> CModuleFileWatcher*;
