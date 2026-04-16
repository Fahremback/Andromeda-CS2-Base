#pragma once
#include <windows.h>
#include <cstdint>

// Forward declarations
class IGameEvent;
class CEntityInstance;
namespace CS2 { class CHandle; }
class CCSGOInput;
class CUserCmd;
struct SharedState;

#define ANDROMEDA_MODULE_INTERFACE_VERSION 0x00000003
#define ANDROMEDA_MODULE_NAME_MAX_LENGTH 64

/**
 * @brief Identificação única do módulo
 */
struct ModuleInfo
{
    char name[ANDROMEDA_MODULE_NAME_MAX_LENGTH] = "Unknown";
    char author[ANDROMEDA_MODULE_NAME_MAX_LENGTH] = "Unknown";
    char version[32] = "1.0.0";
    char description[256] = "";
    uint32_t interfaceVersion = ANDROMEDA_MODULE_INTERFACE_VERSION;
    
    // Flags para dependências e compatibilidade
    bool requiresVisuals = false;
    bool requiresAimbot = false;
    bool requiresResolver = false;
};

/**
 * @brief Estados possíveis do módulo
 */
enum class ModuleState
{
    Unloaded = 0,
    Loading,
    Loaded,
    Error,
    Unloading
};

/**
 * @brief Interface base para todos os módulos Andromeda
 * 
 * Cada módulo (Aimbot, Visual, Autowall, etc.) deve implementar esta interface.
 * O módulo é compilado como DLL separada e carregado sob demanda.
 */
class IAndromedaModule
{
public:
    virtual ~IAndromedaModule() = default;
    
    /**
     * @brief Obtém informações do módulo
     */
    virtual const ModuleInfo& GetModuleInfo() const = 0;
    
    /**
     * @brief Inicializa o módulo
     * @param state Estado compartilhado com SDK, ImGui, Settings
     * @return true se inicializado com sucesso
     */
    virtual bool OnInit(SharedState* state) = 0;
    
    /**
     * @brief FrameStageNotify hook callback
     */
    virtual void OnFrameStageNotify(int stage) = 0;
    
    /**
     * @brief FireEventClientSide hook callback
     */
    virtual void OnFireEventClientSide(IGameEvent* pEvent) = 0;
    
    /**
     * @brief Callback quando entidade é adicionada
     */
    virtual void OnAddEntity(CEntityInstance* pInst, CS2::CHandle handle) = 0;
    
    /**
     * @brief Callback quando entidade é removida
     */
    virtual void OnRemoveEntity(CEntityInstance* pInst, CS2::CHandle handle) = 0;
    
    /**
     * @brief Callback de renderização (chamado a cada frame)
     */
    virtual void OnRender() = 0;
    
    /**
     * @brief ClientOutput hook callback
     */
    virtual void OnClientOutput() = 0;
    
    /**
     * @brief CreateMove hook callback
     * @param pInput Input do jogo
     * @param pCmd Comando do usuário
     */
    virtual void OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd) = 0;
    
    /**
     * @brief Destrói o módulo e libera recursos
     */
    virtual void OnDestroy() = 0;
    
    /**
     * @brief Obtém estado atual do módulo
     */
    virtual ModuleState GetState() const = 0;
    
    /**
     * @brief Define se módulo está ativo
     */
    virtual void SetEnabled(bool enabled) = 0;
    
    /**
     * @brief Verifica se módulo está habilitado
     */
    virtual bool IsEnabled() const = 0;
};

/**
 * @brief Função de exportação padrão para obter instância do módulo
 */
typedef IAndromedaModule* (__cdecl* GetModuleInstanceFn)();

/**
 * @brief Macro para facilitar criação de módulos
 * 
 * Uso no seu módulo:
 *   ANDROMEDA_MODULE_BEGIN(MyAimbotModule)
 *   ... implementação ...
 *   ANDROMEDA_MODULE_END()
 */
#define ANDROMEDA_MODULE_BEGIN(ModuleClass) \
    static ModuleClass g_instance; \
    extern "C" __declspec(dllexport) IAndromedaModule* GetModuleInstance() \
    { \
        return &g_instance; \
    } \
    ModuleClass::ModuleClass()

#define ANDROMEDA_MODULE_END()

/**
 * @brief Macro para definir informações do módulo
 */
#define DECLARE_MODULE_INFO(Name, Author, Version, Desc) \
    const ModuleInfo& GetModuleInfo() const override \
    { \
        static ModuleInfo info = { \
            Name, Author, Version, Desc, \
            ANDROMEDA_MODULE_INTERFACE_VERSION, \
            false, false, false \
        }; \
        return info; \
    }
