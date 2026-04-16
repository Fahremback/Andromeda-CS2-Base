#include "CLiveVariables.hpp"
#include <Common/CrashLog.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>

// Forward declare ImGui
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#endif
#include <ImGui/imgui.h>

static CLiveVariablesSystem g_LiveVars;

auto CLiveVariablesSystem::Get() -> CLiveVariablesSystem*
{
    return &g_LiveVars;
}

auto GetLiveVars() -> CLiveVariablesSystem*
{
    return CLiveVariablesSystem::Get();
}

auto CLiveVariablesSystem::Initialize() -> void
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (m_Initialized)
        return;
    
    m_Variables.clear();
    m_Initialized = true;
    
    DEV_LOG("[LiveVars] System initialized\n");
}

auto CLiveVariablesSystem::Shutdown() -> void
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (!m_Initialized)
        return;
    
    m_Variables.clear();
    m_Initialized = false;
    
    DEV_LOG("[LiveVars] System shutdown\n");
}

auto CLiveVariablesSystem::RegisterVar(const std::string& name, const std::string& category, LiveVarType type) -> LiveVariable*
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (m_Variables.find(name) != m_Variables.end())
    {
        DEV_LOG("[LiveVars] Warning: Variable already registered: %s\n", name.c_str());
        return &m_Variables[name];
    }
    
    LiveVariable var;
    var.name = name;
    var.category = category;
    var.type = type;
    var.lastModified = std::chrono::steady_clock::now();
    
    m_Variables[name] = var;
    
    DEV_LOG("[LiveVars] Registered variable: %s (%s)\n", name.c_str(), category.c_str());
    
    return &m_Variables[name];
}

auto CLiveVariablesSystem::SetFloat(const std::string& name, float value) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end() || it->second.type != LiveVarType::Float)
        return false;
    
    it->second.valueFloat = value;
    UpdateLastModified(&it->second);
    TriggerOnChange(&it->second);
    
    return true;
}

auto CLiveVariablesSystem::SetInt(const std::string& name, int value) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end() || it->second.type != LiveVarType::Int)
        return false;
    
    it->second.valueInt = value;
    UpdateLastModified(&it->second);
    TriggerOnChange(&it->second);
    
    return true;
}

auto CLiveVariablesSystem::SetBool(const std::string& name, bool value) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end() || it->second.type != LiveVarType::Bool)
        return false;
    
    it->second.valueBool = value;
    UpdateLastModified(&it->second);
    TriggerOnChange(&it->second);
    
    return true;
}

auto CLiveVariablesSystem::SetString(const std::string& name, const std::string& value) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end() || it->second.type != LiveVarType::String)
        return false;
    
    it->second.valueString = value;
    UpdateLastModified(&it->second);
    TriggerOnChange(&it->second);
    
    return true;
}

auto CLiveVariablesSystem::SetColor3(const std::string& name, float r, float g, float b) -> bool
{
    return SetColor4(name, r, g, b, 1.0f);
}

auto CLiveVariablesSystem::SetColor4(const std::string& name, float r, float g, float b, float a) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end() || it->second.type != LiveVarType::Color4)
        return false;
    
    it->second.valueColor[0] = r;
    it->second.valueColor[1] = g;
    it->second.valueColor[2] = b;
    it->second.valueColor[3] = a;
    UpdateLastModified(&it->second);
    TriggerOnChange(&it->second);
    
    return true;
}

auto CLiveVariablesSystem::GetFloat(const std::string& name, float defaultValue) -> float
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end() || it->second.type != LiveVarType::Float)
        return defaultValue;
    
    return it->second.valueFloat;
}

auto CLiveVariablesSystem::GetInt(const std::string& name, int defaultValue) -> int
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end() || it->second.type != LiveVarType::Int)
        return defaultValue;
    
    return it->second.valueInt;
}

auto CLiveVariablesSystem::GetBool(const std::string& name, bool defaultValue) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end() || it->second.type != LiveVarType::Bool)
        return defaultValue;
    
    return it->second.valueBool;
}

auto CLiveVariablesSystem::GetString(const std::string& name, const std::string& defaultValue) -> std::string
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end() || it->second.type != LiveVarType::String)
        return defaultValue;
    
    return it->second.valueString;
}

auto CLiveVariablesSystem::GetColor4(const std::string& name, float* outColor) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end() || it->second.type != LiveVarType::Color4)
        return false;
    
    if (outColor)
    {
        outColor[0] = it->second.valueColor[0];
        outColor[1] = it->second.valueColor[1];
        outColor[2] = it->second.valueColor[2];
        outColor[3] = it->second.valueColor[3];
    }
    
    return true;
}

auto CLiveVariablesSystem::UnregisterVar(const std::string& name) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end())
        return false;
    
    m_Variables.erase(it);
    return true;
}

auto CLiveVariablesSystem::GetVarsByCategory(const std::string& category) -> std::vector<LiveVariable*>
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    std::vector<LiveVariable*> result;
    for (auto& pair : m_Variables)
    {
        if (pair.second.category == category && pair.second.visible)
        {
            result.push_back(&pair.second);
        }
    }
    
    return result;
}

auto CLiveVariablesSystem::GetAllVars() -> std::vector<LiveVariable*>
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    std::vector<LiveVariable*> result;
    for (auto& pair : m_Variables)
    {
        if (pair.second.visible)
        {
            result.push_back(&pair.second);
        }
    }
    
    return result;
}

auto CLiveVariablesSystem::RenderUI(bool showWindow) -> void
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (showWindow)
    {
        ImGui::Begin("Live Variables Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    }
    
    // Agrupar por categoria
    std::map<std::string, std::vector<LiveVariable*>> categories;
    for (auto& pair : m_Variables)
    {
        if (!pair.second.visible)
            continue;
        
        categories[pair.second.category].push_back(&pair.second);
    }
    
    for (auto& [category, vars] : categories)
    {
        if (ImGui::CollapsingHeader(category.c_str()))
        {
            for (auto* var : vars)
            {
                ImGui::PushID(var->name.c_str());
                
                switch (var->type)
                {
                case LiveVarType::Float:
                    {
                        float val = var->valueFloat;
                        if (ImGui::SliderFloat(var->name.c_str(), &val, var->minFloat, var->maxFloat, "%.2f"))
                        {
                            var->valueFloat = val;
                            UpdateLastModified(var);
                            TriggerOnChange(var);
                        }
                    }
                    break;
                    
                case LiveVarType::Int:
                    {
                        int val = var->valueInt;
                        if (ImGui::SliderInt(var->name.c_str(), &val, var->minInt, var->maxInt))
                        {
                            var->valueInt = val;
                            UpdateLastModified(var);
                            TriggerOnChange(var);
                        }
                    }
                    break;
                    
                case LiveVarType::Bool:
                    {
                        bool val = var->valueBool;
                        if (ImGui::Checkbox(var->name.c_str(), &val))
                        {
                            var->valueBool = val;
                            UpdateLastModified(var);
                            TriggerOnChange(var);
                        }
                    }
                    break;
                    
                case LiveVarType::String:
                    {
                        char buffer[256];
                        strncpy_s(buffer, sizeof(buffer), var->valueString.c_str(), _TRUNCATE);
                        if (ImGui::InputText(var->name.c_str(), buffer, sizeof(buffer)))
                        {
                            var->valueString = buffer;
                            UpdateLastModified(var);
                            TriggerOnChange(var);
                        }
                    }
                    break;
                    
                case LiveVarType::Color3:
                case LiveVarType::Color4:
                    {
                        float col[4] = {
                            var->valueColor[0],
                            var->valueColor[1],
                            var->valueColor[2],
                            var->valueColor[3]
                        };
                        
                        if (ImGui::ColorEdit4(var->name.c_str(), col))
                        {
                            var->valueColor[0] = col[0];
                            var->valueColor[1] = col[1];
                            var->valueColor[2] = col[2];
                            var->valueColor[3] = col[3];
                            UpdateLastModified(var);
                            TriggerOnChange(var);
                        }
                    }
                    break;
                }
                
                // Mostrar descrição se existir
                if (!var->description.empty())
                {
                    ImGui::SameLine();
                    ImGui::TextDisabled("(?)");
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", var->description.c_str());
                    }
                }
                
                ImGui::PopID();
            }
        }
    }
    
    // Botões de ação
    ImGui::Separator();
    if (ImGui::Button("Export to JSON"))
    {
        ExportToJSON("live_vars_config.json");
    }
    ImGui::SameLine();
    if (ImGui::Button("Import from JSON"))
    {
        ImportFromJSON("live_vars_config.json");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset All"))
    {
        ResetAll();
    }
    
    if (showWindow)
    {
        ImGui::End();
    }
}

auto CLiveVariablesSystem::ExportToJSON(const std::string& filePath) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    try
    {
        std::ofstream file(filePath);
        if (!file.is_open())
        {
            DEV_LOG("[LiveVars] Failed to open file for export: %s\n", filePath.c_str());
            return false;
        }
        
        file << "{\n  \"variables\": {\n";
        
        bool first = true;
        for (const auto& pair : m_Variables)
        {
            if (!first)
                file << ",\n";
            first = false;
            
            const auto& var = pair.second;
            file << "    \"" << var.name << "\": {\n";
            file << "      \"category\": \"" << var.category << "\",\n";
            file << "      \"type\": ";
            
            switch (var.type)
            {
            case LiveVarType::Float:
                file << "\"float\", \"value\": " << var.valueFloat << ",\n";
                file << "      \"min\": " << var.minFloat << ", \"max\": " << var.maxFloat;
                break;
            case LiveVarType::Int:
                file << "\"int\", \"value\": " << var.valueInt << ",\n";
                file << "      \"min\": " << var.minInt << ", \"max\": " << var.maxInt;
                break;
            case LiveVarType::Bool:
                file << "\"bool\", \"value\": " << (var.valueBool ? "true" : "false");
                break;
            case LiveVarType::String:
                file << "\"string\", \"value\": \"" << var.valueString << "\"";
                break;
            case LiveVarType::Color4:
                file << "\"color4\", \"value\": [" 
                     << var.valueColor[0] << ", " 
                     << var.valueColor[1] << ", " 
                     << var.valueColor[2] << ", " 
                     << var.valueColor[3] << "]";
                break;
            default:
                file << "\"unknown\"";
                break;
            }
            
            file << "\n    }";
        }
        
        file << "\n  }\n}\n";
        file.close();
        
        DEV_LOG("[LiveVars] Exported %d variables to %s\n", (int)m_Variables.size(), filePath.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        DEV_LOG("[LiveVars] Export error: %s\n", e.what());
        return false;
    }
}

auto CLiveVariablesSystem::ImportFromJSON(const std::string& filePath) -> bool
{
    // Implementação simplificada - parsing completo exigiria biblioteca JSON
    // Por enquanto, apenas loga a tentativa
    DEV_LOG("[LiveVars] Import from JSON requested: %s\n", filePath.c_str());
    DEV_LOG("[LiveVars] JSON import requires nlohmann/json or similar library\n");
    
    // TODO: Implementar com nlohmann/json quando disponível
    return false;
}

auto CLiveVariablesSystem::SetOnChangeCallback(const std::string& name, std::function<void(const LiveVariable&)> callback) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end())
        return false;
    
    it->second.onChangeCallback = callback;
    return true;
}

auto CLiveVariablesSystem::ResetAll() -> void
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    for (auto& pair : m_Variables)
    {
        auto& var = pair.second;
        
        switch (var.type)
        {
        case LiveVarType::Float:
            var.valueFloat = var.minFloat;
            break;
        case LiveVarType::Int:
            var.valueInt = var.minInt;
            break;
        case LiveVarType::Bool:
            var.valueBool = false;
            break;
        case LiveVarType::String:
            var.valueString.clear();
            break;
        case LiveVarType::Color4:
            var.valueColor[0] = 1.0f;
            var.valueColor[1] = 1.0f;
            var.valueColor[2] = 1.0f;
            var.valueColor[3] = 1.0f;
            break;
        }
        
        UpdateLastModified(&var);
        TriggerOnChange(&var);
    }
    
    DEV_LOG("[LiveVars] All variables reset\n");
}

auto CLiveVariablesSystem::ResetVar(const std::string& name) -> bool
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Variables.find(name);
    if (it == m_Variables.end())
        return false;
    
    auto& var = it->second;
    
    switch (var.type)
    {
    case LiveVarType::Float:
        var.valueFloat = var.minFloat;
        break;
    case LiveVarType::Int:
        var.valueInt = var.minInt;
        break;
    case LiveVarType::Bool:
        var.valueBool = false;
        break;
    case LiveVarType::String:
        var.valueString.clear();
        break;
    case LiveVarType::Color4:
        var.valueColor[0] = 1.0f;
        var.valueColor[1] = 1.0f;
        var.valueColor[2] = 1.0f;
        var.valueColor[3] = 1.0f;
        break;
    }
    
    UpdateLastModified(&var);
    TriggerOnChange(&var);
    
    return true;
}

auto CLiveVariablesSystem::UpdateLastModified(LiveVariable* var) -> void
{
    if (var)
    {
        var->lastModified = std::chrono::steady_clock::now();
    }
}

auto CLiveVariablesSystem::TriggerOnChange(LiveVariable* var) -> void
{
    if (var && var->onChangeCallback)
    {
        try
        {
            var->onChangeCallback(*var);
        }
        catch (...)
        {
            DEV_LOG("[LiveVars] Exception in onChange callback for %s\n", var->name.c_str());
        }
    }
}
