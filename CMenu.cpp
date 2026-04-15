#include "CMenu.hpp"
#include "CAimbot.hpp"
#include "CConfig.hpp"

bool CMenu::bIsOpen = true;

void CMenu::Initialize() {
    // Configuração inicial do menu
}

void CMenu::Toggle() {
    bIsOpen = !bIsOpen;
}

void CMenu::Render() {
    if (!bIsOpen) return;

    ImGui::Begin("Andromeda CS2", &bIsOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    
    if (ImGui::BeginTabBar("##MainTabs")) {
        if (ImGui::BeginTabItem("Aimbot")) {
            RenderAimbotTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Trigger")) {
            RenderTriggerTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Visuals")) {
            RenderVisualsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Config")) {
            RenderConfigTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

void CMenu::RenderAimbotTab() {
    auto& config = CAimbot::GetConfig();
    
    ImGui::Checkbox("Enable Aimbot", &config.bEnabled);
    ImGui::SameLine();
    ImGui::ColorEdit4("Aimbot Color", config.aimbotColor);
    
    ImGui::SliderFloat("FOV", &config.flFOV, 0.0f, 180.0f, "%.1f°");
    ImGui::SliderFloat("Smooth", &config.flSmooth, 1.0f, 100.0f, "%.1f%%");
    ImGui::SliderInt("Bone Target", &config.iTargetBone, 0, 19, "%d");
    
    ImGui::Separator();
    
    ImGui::Checkbox("Auto Wall", &config.bAutoWall);
    ImGui::SameLine();
    ImGui::SliderInt("Min Damage", &config.iMinDamage, 1, 100, "%d HP");
    
    ImGui::Separator();
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Performance Tech:");
    ImGui::BulletText("SIMD AVX-512 Batch Processing (8 alvos/ciclo)");
    ImGui::BulletText("SoA Entity Cache (L1 Cache otimizado)");
    ImGui::BulletText("rsqrt14 Fast Normalize (70%% menos ciclos)");
    ImGui::BulletText("Arena Allocator (Zero syscall overhead)");
}

void CMenu::RenderTriggerTab() {
    auto& config = CAimbot::GetConfig();
    
    ImGui::Checkbox("Enable Trigger Bot", &config.bTriggerEnabled);
    ImGui::SameLine();
    ImGui::ColorEdit4("Trigger Color", config.triggerColor);
    
    ImGui::SliderInt("Hit Group", &config.iTriggerHitGroup, 0, 6, "%d");
    ImGui::SliderInt("Reaction Delay (ms)", &config.iTriggerDelay, 0, 100, "%d ms");
    
    ImGui::Separator();
    
    ImGui::Checkbox("Auto Wall Filter", &config.bTriggerAutoWall);
    ImGui::SameLine();
    ImGui::SliderInt("Min Damage", &config.iTriggerMinDamage, 1, 100, "%d HP");
    
    ImGui::Separator();
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Performance Tech:");
    ImGui::BulletText("Instant Crosshair Detection");
    ImGui::BulletText("MPSC Ring-Buffer (Zero contention)");
    ImGui::BulletText("Thread-Local Staging (No race conditions)");
}

void CMenu::RenderVisualsTab() {
    ImGui::Checkbox("Enable ESP", &CAimbot::GetConfig().bESP);
    ImGui::SameLine();
    ImGui::Checkbox("Box ESP", &CAimbot::GetConfig().bBoxESP);
    ImGui::SameLine();
    ImGui::Checkbox("Skeleton", &CAimbot::GetConfig().bSkeleton);
    
    ImGui::SliderFloat("ESP Distance", &CAimbot::GetConfig().flESPDistance, 0.0f, 1000.0f, "%.0f units");
    
    ImGui::Separator();
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Performance Tech:");
    ImGui::BulletText("FMA Matrix Projection (Hardware precision)");
    ImGui::BulletText("SIMD Batch W2S (All bones simultaneously)");
}

void CMenu::RenderConfigTab() {
    static char configName[64] = "";
    
    ImGui::InputText("Config Name", configName, IM_ARRAYSIZE(configName));
    
    ImGui::Separator();
    
    if (ImGui::Button("Save Config", ImVec2(200, 30))) {
        // Implementar save de config
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Config", ImVec2(200, 30))) {
        // Implementar load de config
    }
    
    ImGui::Separator();
    
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "System Info:");
    ImGui::BulletText("AVX-512 Support: %s", CAimbot::HasAVX512() ? "YES" : "NO");
    ImGui::BulletText("Thread Count: %d", CAimbot::GetOptimalThreadCount());
    ImGui::BulletText("Arena Memory: 8MB Aligned (64B)");
}
