# Documentação do Sistema de Módulos Andromeda

## Visão Geral

O sistema de módulos Andromeda permite carregar e descarregar funcionalidades individualmente sem reiniciar o jogo. Cada módulo é uma DLL independente que implementa a interface `IAndromedaModule`.

## Arquitetura

```
Andromeda-Bridge.dll (Motor Fixo)
    │
    ├── CModuleManager (Gerenciador)
    │       │
    │       ├── modules/aimbot.dll (Carregado sob demanda)
    │       ├── modules/autowall.dll (Carregado sob demanda)
    │       └── modules/resolver.dll (Carregado sob demanda)
    │
    └── Andromeda-Logic.dll (Legado - ainda suportado)
```

## Como Criar um Módulo

### 1. Estrutura Básica

```cpp
// MyModule.hpp
#pragma once
#include <Common/Include/IAndromedaModule.hpp>

class CMyModule final : public IAndromedaModule
{
public:
    const ModuleInfo& GetModuleInfo() const override;
    bool OnInit(SharedState* state) override;
    void OnFrameStageNotify(int stage) override;
    void OnFireEventClientSide(IGameEvent* pEvent) override;
    void OnAddEntity(CEntityInstance* pInst, CHandle handle) override;
    void OnRemoveEntity(CEntityInstance* pInst, CHandle handle) override;
    void OnRender() override;
    void OnClientOutput() override;
    void OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd) override;
    void OnDestroy() override;
    
    ModuleState GetState() const override { return m_State; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    bool IsEnabled() const override { return m_Enabled; }

private:
    SharedState* m_SharedState = nullptr;
    ModuleState m_State = ModuleState::Unloaded;
    std::atomic<bool> m_Enabled = true;
};

ANDROMEDA_MODULE_BEGIN(CMyModule) {}
ANDROMEDA_MODULE_END()

DECLARE_MODULE_INFO(
    "Nome do Módulo",
    "Autor",
    "1.0.0",
    "Descrição"
)
```

### 2. Implementação

```cpp
// MyModule.cpp
#include "MyModule.hpp"

bool CMyModule::OnInit(SharedState* state)
{
    if (!state || state->interfaceVersion != ANDROMEDA_MODULE_INTERFACE_VERSION)
        return false;
    
    m_SharedState = state;
    m_State = ModuleState::Loaded;
    return true;
}

void CMyModule::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
    if (!m_Enabled || !pInput || !pCmd)
        return;
    
    // Sua lógica aqui
}

void CMyModule::OnDestroy()
{
    m_State = ModuleState::Unloaded;
}
```

## API do Module Manager

### Carregar Módulo
```cpp
// Carrega um módulo específico
GetModuleManager()->LoadModule("modules/aimbot.dll");

// Retorna true se sucesso
```

### Descarregar Módulo
```cpp
// Descarrega pelo nome (sem extensão)
GetModuleManager()->UnloadModule("aimbot");
```

### Recarregar Módulo
```cpp
// Descarrega e carrega novamente
GetModuleManager()->ReloadModule("aimbot");
```

### Habilitar/Desabilitar
```cpp
GetModuleManager()->SetModuleEnabled("aimbot", false);
GetModuleManager()->SetModuleEnabled("aimbot", true);
```

### Verificar Status
```cpp
bool loaded = GetModuleManager()->IsModuleLoaded("aimbot");
auto modules = GetModuleManager()->GetLoadedModules();
auto module = GetModuleManager()->GetModule("aimbot");
```

### Escanear Diretório
```cpp
// Auto-carrega todos os módulos da pasta
auto found = GetModuleManager()->ScanForModules("modules");
for (const auto& path : found)
    GetModuleManager()->LoadModule(path);
```

## Fluxo de Callbacks

Todos os callbacks são encaminhados para os módulos ativos:

1. `OnFrameStageNotify` → Todos os módulos
2. `OnFireEventClientSide` → Todos os módulos
3. `OnAddEntity` → Todos os módulos
4. `OnRemoveEntity` → Todos os módulos
5. `OnRender` → Todos os módulos
6. `OnClientOutput` → Todos os módulos
7. `OnCreateMove` → Todos os módulos

Cada módulo pode estar habilitado ou desabilitado independentemente.

## Exemplos de Uso

### Aimbot Module
Já incluso: `Andromeda-Module-Aimbot`
- Silent aim
- Legit bot com smooth
- FOV configurável

### Autowall Module
Já incluso: `Andromeda-Module-Autowall`
- Cálculo de dano através de paredes
- API pública para outros módulos usarem

### Criando Visual Module
```cpp
class CVisualModule : public IAndromedaModule
{
    void OnRender() override
    {
        if (!m_Enabled) return;
        
        // ESP Box
        // Snaplines
        // Health bars
    }
};
```

## Compilação

Cada módulo deve ser compilado como DLL separada:

1. Crie projeto .vcxproj separado
2. Link contra as libs do Base
3. Exporte `GetModuleInstance`
4. Compile em Release/x64
5. Coloque DLL na pasta `modules`

## Vantagens

✅ **Atualização Rápida**: Atualize apenas o módulo changed
✅ **Estabilidade**: Bug em um módulo não crasha tudo
✅ **Customização**: Cada usuário carrega só o que precisa
✅ **Desenvolvimento**: Teste módulos isoladamente
✅ **Memória**: Só carrega o necessário

## Boas Práticas

1. **Sempre verifique `m_Enabled`** antes de executar lógica
2. **Use try-catch** em operações críticas
3. **Libere recursos** em `OnDestroy`
4. **Não guarde estados globais** estáticos
5. **Valide pointers** do SDK antes de usar

## Comandos no Console (Futuro)

```
module_load aimbot
module_unload aimbot
module_reload aimbot
module_list
module_enable aimbot
module_disable aimbot
```

## Integração com Menu

No futuro, o menu permitirá:
- Lista de módulos carregados
- Toggle enable/disable
- Configurações por módulo
- Hotkey para reload

## Suporte ao Legacy Logic

O sistema antigo `Andromeda-Logic.dll` continua funcionando. Ambos coexistem:
- Module Manager para módulos individuais
- Logic DLL para funcionalidade monolítica

## Versionamento

Interface atual: `ANDROMEDA_MODULE_INTERFACE_VERSION = 0x00000003`

Módulos devem verificar compatibilidade no `OnInit`.
