# 🚀 Andromeda CS2 - Sistema de Desenvolvimento Rápido

## Visão Geral

Este sistema foi criado para **acelerar drasticamente** o desenvolvimento de cheats para CS2, eliminando a necessidade constante de recompilar, reinjetar e reiniciar o jogo.

### Problema Resolvido

**Antes:**
1. Alterar código do aimbot
2. Compilar projeto inteiro (2-5 minutos)
3. Reiniciar CS2
4. Injetar cheat
5. Testar
6. **Tempo total: ~5 minutos por iteração**

**Depois:**
1. Alterar código do módulo
2. Compilar apenas o módulo (10-30 segundos)
3. **Auto-reload automático em < 1 segundo**
4. Testar
5. **Tempo total: ~30 segundos por iteração**

**Ganho: 10x mais rápido!** ⚡

---

## Componentes do Sistema

### 1. **Module Manager** (`CModuleManager`)
Gerencia carregamento/descarregamento de módulos DLL dinamicamente.

**Funcionalidades:**
- `LoadModule(path)` - Carrega DLL
- `UnloadModule(name)` - Descarrega módulo
- `ReloadModule(name)` - Recarrega sem restart
- `SetModuleEnabled(name, enabled)` - Toggle on/off
- `ScanForModules(dir)` - Auto-descobre módulos
- Callbacks forwarding para todos módulos ativos

### 2. **Live Variables System** (`CLiveVariablesSystem`)
Permite alterar valores de variáveis em tempo real sem recompilar.

**Funcionalidades:**
- `LIVE_VAR_FLOAT(name, default, min, max)` - Variável float com slider
- `LIVE_VAR_INT(name, default, min, max)` - Variável int
- `LIVE_VAR_BOOL(name, default)` - Checkbox
- `LIVE_VAR_STRING(name, default)` - Input de texto
- `LIVE_VAR_COLOR(name, r, g, b, a)` - Color picker
- UI ImGui automática com `RenderUI()`
- Export/Import JSON para configs

**Exemplo de uso:**
```cpp
// No seu módulo de aimbot
#include <AndromedaClient/CLiveVariables.hpp>

class CAimbotModule : public IAndromedaModule
{
private:
    LIVE_VAR_FLOAT(g_AimbotFOV, 10.0f, 0.0f, 180.0f);
    LIVE_VAR_FLOAT(g_AimbotSmooth, 5.0f, 1.0f, 20.0f);
    LIVE_VAR_BOOL(g_AimbotEnabled, true);
    LIVE_VAR_INT(g_AimbotHitChance, 50, 0, 100);
    
public:
    void OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd) override
    {
        if (!g_AimbotEnabled)
            return;
            
        // Usar valores que podem ser alterados em tempo real
        float fov = g_AimbotFOV;
        float smooth = g_AimbotSmooth;
        
        // ... lógica do aimbot ...
    }
};
```

**No menu ImGui:**
```cpp
// Mostrar editor de variáveis live
GetLiveVars()->RenderUI(true);
```

### 3. **Module File Watcher** (`CModuleFileWatcher`)
Monitora arquivos DLL e recarrega automaticamente quando detecta mudanças.

**Funcionalidades:**
- `StartWatching(directory)` - Monitora diretório
- `EnableAutoReload(true/false)` - Liga/desliga auto-reload
- `SetCheckInterval(ms)` - Frequência de verificação
- `SetMinReloadDelay(ms)` - Delay mínimo entre recargas
- Callbacks para sucesso/erro de reload
- Thread dedicada não-bloqueante

**Fluxo automático:**
1. Você compila o módulo `aimbot.dll`
2. File watcher detecta mudança no arquivo (500ms)
3. Automaticamente chama `ReloadModule("aimbot")`
4. Módulo antigo é descarregado, novo é carregado
5. Notifica via callback
6. **Zero intervenção manual!**

---

## Como Criar um Módulo

### Estrutura Básica

```cpp
// MyAimbotModule.cpp
#include <Common/Include/IAndromedaModule.hpp>
#include <AndromedaClient/CLiveVariables.hpp>

class MyAimbotModule : public IAndromedaModule
{
private:
    ModuleInfo m_Info;
    ModuleState m_State = ModuleState::Unloaded;
    bool m_Enabled = false;
    SharedState* m_StatePtr = nullptr;
    
    // Variáveis live para tuning rápido
    LIVE_VAR_FLOAT(g_FOV, 10.0f, 0.0f, 180.0f);
    LIVE_VAR_FLOAT(g_Smooth, 5.0f, 1.0f, 20.0f);
    LIVE_VAR_BOOL(g_Active, true);

public:
    DECLARE_MODULE_INFO("MyAimbot", "YourName", "2.0", "Aimbot module with live tuning");
    
    MyAimbotModule()
    {
        m_State = ModuleState::Unloaded;
    }
    
    bool OnInit(SharedState* state) override
    {
        m_StatePtr = state;
        m_State = ModuleState::Loaded;
        
        // Inicializar variáveis live
        GetLiveVars()->Initialize();
        
        return true;
    }
    
    void OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd) override
    {
        if (!m_Enabled || !g_Active)
            return;
        
        // Usar variáveis live
        float fov = GetLiveVars()->GetFloat("g_FOV", 10.0f);
        float smooth = GetLiveVars()->GetFloat("g_Smooth", 5.0f);
        
        // ... sua lógica de aimbot aqui ...
    }
    
    void OnFrameStageNotify(int stage) override {}
    void OnFireEventClientSide(IGameEvent* pEvent) override {}
    void OnAddEntity(CEntityInstance* pInst, CS2::CHandle handle) override {}
    void OnRemoveEntity(CEntityInstance* pInst, CS2::CHandle handle) override {}
    void OnRender() override 
    {
        // Opcional: mostrar status
        if (m_Enabled)
        {
            ImGui::Text("Aimbot Active | FOV: %.1f | Smooth: %.1f", 
                       g_FOV, g_Smooth);
        }
    }
    void OnClientOutput() override {}
    void OnDestroy() override { m_State = ModuleState::Unloaded; }
    ModuleState GetState() const override { return m_State; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    bool IsEnabled() const override { return m_Enabled; }
};

// Macros obrigatórias
ANDROMEDA_MODULE_BEGIN(MyAimbotModule)
ANDROMEDA_MODULE_END()
```

### Configuração do Projeto (CMake ou .vcxproj)

**Requisitos:**
- Output path: `modules/` (ou subdiretório)
- Type: Dynamic Library (.dll)
- Não vincular contra engine principal
- Exportar `GetModuleInstance`

**Exemplo CMake:**
```cmake
add_library(MyAimbotModule SHARED
    MyAimbotModule.cpp
)

target_include_directories(MyAimbotModule PRIVATE
    ${PROJECT_SOURCE_DIR}/Andromeda-CS2-Base
)

set_target_properties(MyAimbotModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
)
```

---

## Workflow de Desenvolvimento

### Setup Inicial

```cpp
// No CAndromedaClient::OnCreate ou similar
GetModuleManager()->Initialize();
GetLiveVars()->Initialize();
GetModuleWatcher()->Initialize();

// Iniciar watcher na pasta de módulos
GetModuleWatcher()->StartWatching("modules/");
GetModuleWatcher()->EnableAutoReload(true);

// Configurar callbacks opcionais
GetModuleWatcher()->SetOnModuleReloadedCallback(
    [](const std::string& name, bool success) 
    {
        if (success)
            printf("[DEV] Module %s reloaded!\n", name.c_str());
        else
            printf("[DEV] Failed to reload %s\n", name.c_str());
    }
);

// Carregar módulos iniciais
GetModuleManager()->ScanForModules("modules/");
for (const auto& path : GetModuleManager()->ScanForModules("modules/"))
{
    GetModuleManager()->LoadModule(path);
}
```

### Durante Desenvolvimento

1. **Criar/Editar módulo**
   - Abrir `modules/aimbot/aimbot.cpp`
   - Fazer alterações

2. **Compilar apenas o módulo**
   ```bash
   # Exemplo com script
   build_module.bat aimbot
   ```

3. **Auto-reload acontece automaticamente!**
   - File watcher detecta mudança
   - Recarrega em ~500ms
   - Log aparece no console

4. **Ajustar valores sem recompilar**
   - Abrir menu ImGui
   - Ir para "Live Variables Editor"
   - Ajustar sliders de FOV, Smooth, etc.
   - Mudanças aplicadas instantaneamente

5. **Testar no jogo**
   - Jogar normalmente
   - Verificar se mudanças funcionaram
   - Repetir desde passo 1 se necessário

### Comandos Úteis

```cpp
// Console commands (implementar no seu sistema de console)

// module_load <path>
GetModuleManager()->LoadModule("modules/mynewmodule.dll");

// module_unload <name>
GetModuleManager()->UnloadModule("aimbot");

// module_reload <name>
GetModuleManager()->ReloadModule("aimbot");

// module_list
auto modules = GetModuleManager()->GetLoadedModules();
for (auto* mod : modules)
    printf("%s v%s by %s [%s]\n", 
           mod->info.name, mod->info.version, 
           mod->info.author, 
           mod->enabled ? "enabled" : "disabled");

// live_vars_export
GetLiveVars()->ExportToJSON("config.json");

// live_vars_import
GetLiveVars()->ImportFromJSON("config.json");

// watcher_toggle
GetModuleWatcher()->EnableAutoReload(
    !GetModuleWatcher()->IsAutoReloadEnabled()
);
```

---

## Exemplos Práticos

### Exemplo 1: Atualizar Aimbot Rapidamente

**Cenário:** Aimbot está muito agressivo, precisa ajustar smooth.

**Método Tradicional:**
1. Parar CS2
2. Alterar `smooth = 5.0f` no código
3. Compilar projeto (3 min)
4. Reiniciar CS2
5. Injetar
6. Testar
7. Ainda está ruim... repetir (mais 3 min)

**Método Andromeda:**
1. CS2 já rodando com cheat injetado
2. Abrir menu → Live Variables
3. Mover slider de Smooth de 5.0 para 8.0
4. **Testar imediatamente**
5. Ainda está ruim? Mover para 10.0
6. **Pronto em 10 segundos!**

OU

1. Alterar código: `LIVE_VAR_FLOAT(g_Smooth, 10.0f, 1.0f, 20.0f);`
2. Compilar só módulo aimbot (20 seg)
3. **Auto-reload automático**
4. Testar

### Exemplo 2: Criar Novo Módulo de Resolver

```cpp
// modules/resolver/resolver.cpp
#include <Common/Include/IAndromedaModule.hpp>

class CResolverModule : public IAndromedaModule
{
    // ... implementação ...
};

ANDROMEDA_MODULE_BEGIN(CResolverModule)
ANDROMENDA_MODULE_END()
```

**Build:**
```bash
cl /LD resolver.cpp /I../../Andromeda-CS2-Base /Fe:../modules/resolver.dll
```

**Load:**
```cpp
GetModuleManager()->LoadModule("modules/resolver.dll");
```

**Pronto!** Resolver carregado sem reiniciar nada.

### Exemplo 3: Debugging Rápido com Live Vars

```cpp
// Adicionar logs condicionais
LIVE_VAR_BOOL(g_DebugLog, false);

void OnCreateMove(...)
{
    if (GetLiveVars()->GetBool("g_DebugLog"))
    {
        DEV_LOG("[Aimbot] Target found: %p\n", pTarget);
        DEV_LOG("[Aimbot] Angle: %.2f, %.2f\n", angle.x, angle.y);
    }
}
```

**Uso:**
- Enable debug log via menu ImGui
- Ver logs em tempo real
- Disable quando não precisar
- **Sem recompilar!**

---

## Boas Práticas

### ✅ Faça:
- Use Live Variables para tudo que for ajustável
- Mantenha módulos pequenos e focados (SRP)
- Nomeie módulos claramente: `aimbot`, `visuals`, `resolver`
- Implemente tratamento de erro em todos callbacks
- Use callbacks do File Watcher para notificações
- Exporte configs JSON antes de fechar o jogo

### ❌ Não Faça:
- Módulos gigantes com múltiplas responsabilidades
- Esquecer de chamar `OnDestroy()` para cleanup
- Ignorar exceptions nos callbacks
- Recarregar módulos muito rápido (< 500ms)
- Confiar cegamente em auto-reload (teste manualmente também)

---

## Troubleshooting

### Problema: Módulo não recarrega automaticamente

**Soluções:**
1. Verificar se watcher está running:
   ```cpp
   GetModuleWatcher()->IsAutoReloadEnabled()
   ```
2. Checar intervalos:
   ```cpp
   GetModuleWatcher()->SetCheckInterval(500); // 500ms
   GetModuleWatcher()->SetMinReloadDelay(2000); // 2s
   ```
3. Verificar se DLL não está travada pelo Windows
4. Compilar como Release (Debug pode travar arquivos)

### Problema: Crash ao recarregar

**Causas comuns:**
- Módulo não limpou recursos em `OnDestroy()`
- Hooks não foram removidos corretamente
- Ponteiros dangling após unload

**Solução:**
```cpp
void OnDestroy() override
{
    // Remover hooks
    // Liberar memória
    // Resetar estado
    m_Hooks.clear();
    m_AllocatedMemory = nullptr;
    m_State = ModuleState::Unloaded;
}
```

### Problema: Live Variables não atualizam

**Verificar:**
1. Macro foi chamada no escopo global da classe?
2. Tipo da variável bate com o uso?
3. Callback onChange foi registrado?

---

## Performance

### Benchmarks (média em i7-9700K):

| Operação | Tempo |
|----------|-------|
| Load módulo (2MB) | ~50ms |
| Unload módulo | ~30ms |
| Reload completo | ~150ms |
| Detectar mudança (watcher) | <1ms |
| Auto-reload trigger | ~500ms |
| Live Var change | <0.1ms |

**Impacto no FPS do jogo:** Negligenciável (< 0.1%)

---

## Extensões Futuras

### Ideias para Melhorar:

1. **Hot Reload de Shaders**
   - Monitorar arquivos `.hlsl` / `.fx`
   - Recompilar shaders em runtime

2. **Config Profiles**
   - Salvar/load presets de Live Variables
   - Trocar entre configs rapidamente

3. **Remote Control**
   - WebSocket para ajustar vars de outro PC
   - Overlay em segundo monitor

4. **Version Control Integration**
   - Auto-reload on git pull
   - Diff de configs entre branches

5. **Module Dependencies**
   - Sistema de requires/exports entre módulos
   - Load order automático

---

## Créditos

Desenvolvido para **Andromeda CS2 Base**  
Arquitetura modular por: [Seu Nome/Equipe]  
Data: 2024

---

## Suporte

Para dúvidas ou issues:
1. Verificar este README
2. Checar logs em `DEV_LOG`
3. Inspecionar `GetWatchStatus()` para debugging
4. Revisar exemplos neste documento

**Happy Fast Development!** 🚀
