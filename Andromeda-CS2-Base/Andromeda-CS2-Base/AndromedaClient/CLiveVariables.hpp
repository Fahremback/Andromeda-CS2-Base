#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>

/**
 * @brief Sistema de Variáveis Live (Tempo Real)
 * 
 * Permite alterar valores de variáveis sem recompilar.
 * Ideal para tuning de aimbot, visuals, etc.
 * 
 * Uso:
 *   LIVE_VAR_FLOAT(g_AimbotFOV, 10.0f, 0.0f, 180.0f);
 *   LIVE_VAR_BOOL(g_AimbotEnabled, true);
 *   LIVE_VAR_INT(g_AimbotSmooth, 5, 1, 20);
 * 
 * No menu ImGui:
 *   GetLiveVars()->RenderUI();
 */

enum class LiveVarType
{
    Float,
    Int,
    Bool,
    String,
    Color3,
    Color4
};

struct LiveVariable
{
    std::string name;
    std::string category;
    LiveVarType type;
    
    // Valores
    float valueFloat = 0.0f;
    int valueInt = 0;
    bool valueBool = false;
    std::string valueString;
    float valueColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    
    // Limites (para sliders)
    float minFloat = 0.0f;
    float maxFloat = 1.0f;
    int minInt = 0;
    int maxInt = 100;
    
    // Callback quando valor muda
    std::function<void(const LiveVariable&)> onChangeCallback;
    
    // Metadados
    std::string description;
    bool visible = true;
    bool readOnly = false;
    
    // Última modificação
    std::chrono::steady_clock::time_point lastModified;
};

class CLiveVariablesSystem final
{
public:
    /**
     * @brief Obtém instância singleton
     */
    static auto Get() -> CLiveVariablesSystem*;
    
    /**
     * @brief Inicializa o sistema
     */
    auto Initialize() -> void;
    
    /**
     * @brief Finaliza o sistema
     */
    auto Shutdown() -> void;
    
    /**
     * @brief Registra uma variável live
     * @param name Nome único da variável
     * @param category Categoria (ex: "Aimbot", "Visual")
     * @param type Tipo da variável
     * @return Referência para a variável registrada
     */
    auto RegisterVar(const std::string& name, const std::string& category, LiveVarType type) -> LiveVariable*;
    
    /**
     * @brief Define valor de variável float
     */
    auto SetFloat(const std::string& name, float value) -> bool;
    
    /**
     * @brief Define valor de variável int
     */
    auto SetInt(const std::string& name, int value) -> bool;
    
    /**
     * @brief Define valor de variável bool
     */
    auto SetBool(const std::string& name, bool value) -> bool;
    
    /**
     * @brief Define valor de variável string
     */
    auto SetString(const std::string& name, const std::string& value) -> bool;
    
    /**
     * @brief Define valor de cor (RGB)
     */
    auto SetColor3(const std::string& name, float r, float g, float b) -> bool;
    
    /**
     * @brief Define valor de cor (RGBA)
     */
    auto SetColor4(const std::string& name, float r, float g, float b, float a) -> bool;
    
    /**
     * @brief Obtém valor float
     */
    auto GetFloat(const std::string& name, float defaultValue = 0.0f) -> float;
    
    /**
     * @brief Obtém valor int
     */
    auto GetInt(const std::string& name, int defaultValue = 0) -> int;
    
    /**
     * @brief Obtém valor bool
     */
    auto GetBool(const std::string& name, bool defaultValue = false) -> bool;
    
    /**
     * @brief Obtém valor string
     */
    auto GetString(const std::string& name, const std::string& defaultValue = "") -> std::string;
    
    /**
     * @brief Obtém cor como array float[4]
     */
    auto GetColor4(const std::string& name, float* outColor) -> bool;
    
    /**
     * @brief Remove uma variável
     */
    auto UnregisterVar(const std::string& name) -> bool;
    
    /**
     * @brief Obtém todas as variáveis de uma categoria
     */
    auto GetVarsByCategory(const std::string& category) -> std::vector<LiveVariable*>;
    
    /**
     * @brief Obtém todas as variáveis
     */
    auto GetAllVars() -> std::vector<LiveVariable*>;
    
    /**
     * @brief Renderiza UI ImGui para edição das variáveis
     * @param showWindow Se true, mostra janela completa
     */
    auto RenderUI(bool showWindow = false) -> void;
    
    /**
     * @brief Exporta todas as variáveis para JSON
     * @param filePath Caminho do arquivo
     * @return true se exportado com sucesso
     */
    auto ExportToJSON(const std::string& filePath) -> bool;
    
    /**
     * @brief Importa variáveis de JSON
     * @param filePath Caminho do arquivo
     * @return true se importado com sucesso
     */
    auto ImportFromJSON(const std::string& filePath) -> bool;
    
    /**
     * @brief Define callback para quando uma variável mudar
     */
    auto SetOnChangeCallback(const std::string& name, std::function<void(const LiveVariable&)> callback) -> bool;
    
    /**
     * @brief Reseta todas as variáveis para valores padrão
     */
    auto ResetAll() -> void;
    
    /**
     * @brief Reseta variável específica
     */
    auto ResetVar(const std::string& name) -> bool;
    
private:
    auto UpdateLastModified(LiveVariable* var) -> void;
    auto TriggerOnChange(LiveVariable* var) -> void;
    
private:
    std::map<std::string, LiveVariable> m_Variables;
    std::mutex m_Mutex;
    bool m_Initialized = false;
};

// Macros facilitadoras
#define LIVE_VAR_FLOAT(name, defaultValue, minVal, maxVal) \
    static float name = defaultValue; \
    static bool name##_registered = []() { \
        GetLiveVars()->RegisterVar(#name, "Default", LiveVarType::Float); \
        GetLiveVars()->SetFloat(#name, defaultValue); \
        GetLiveVars()->m_Variables[#name].minFloat = minVal; \
        GetLiveVars()->m_Variables[#name].maxFloat = maxVal; \
        GetLiveVars()->m_Variables[#name].onChangeCallback = [](const LiveVariable& v) { \
            name = v.valueFloat; \
        }; \
        return true; \
    }()

#define LIVE_VAR_INT(name, defaultValue, minVal, maxVal) \
    static int name = defaultValue; \
    static bool name##_registered = []() { \
        GetLiveVars()->RegisterVar(#name, "Default", LiveVarType::Int); \
        GetLiveVars()->SetInt(#name, defaultValue); \
        GetLiveVars()->m_Variables[#name].minInt = minVal; \
        GetLiveVars()->m_Variables[#name].maxInt = maxVal; \
        GetLiveVars()->m_Variables[#name].onChangeCallback = [](const LiveVariable& v) { \
            name = v.valueInt; \
        }; \
        return true; \
    }()

#define LIVE_VAR_BOOL(name, defaultValue) \
    static bool name = defaultValue; \
    static bool name##_registered = []() { \
        GetLiveVars()->RegisterVar(#name, "Default", LiveVarType::Bool); \
        GetLiveVars()->SetBool(#name, defaultValue); \
        GetLiveVars()->m_Variables[#name].onChangeCallback = [](const LiveVariable& v) { \
            name = v.valueBool; \
        }; \
        return true; \
    }()

#define LIVE_VAR_STRING(name, defaultValue) \
    static std::string name = defaultValue; \
    static bool name##_registered = []() { \
        GetLiveVars()->RegisterVar(#name, "Default", LiveVarType::String); \
        GetLiveVars()->SetString(#name, defaultValue); \
        GetLiveVars()->m_Variables[#name].onChangeCallback = [](const LiveVariable& v) { \
            name = v.valueString; \
        }; \
        return true; \
    }()

#define LIVE_VAR_COLOR(name, r, g, b, a) \
    static float name[4] = {r, g, b, a}; \
    static bool name##_registered = []() { \
        GetLiveVars()->RegisterVar(#name, "Default", LiveVarType::Color4); \
        GetLiveVars()->SetColor4(#name, r, g, b, a); \
        GetLiveVars()->m_Variables[#name].onChangeCallback = [](const LiveVariable& v) { \
            name[0] = v.valueColor[0]; \
            name[1] = v.valueColor[1]; \
            name[2] = v.valueColor[2]; \
            name[3] = v.valueColor[3]; \
        }; \
        return true; \
    }()

// Helper function
auto GetLiveVars() -> CLiveVariablesSystem*;
