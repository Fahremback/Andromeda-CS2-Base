#include "CMenu.hpp"
#include "CAimbot.hpp"
#include "CConfig.hpp"
#include <string>

bool CMenu::bIsOpen = true;
bool CMenu::bStyleInitialized = false;
CMenu::ETab CMenu::CurrentTab = CMenu::ETab::Aimbot;

void CMenu::Initialize() {
    ApplyModernStyle();
}

void CMenu::Toggle() {
    bIsOpen = !bIsOpen;
}

void CMenu::ApplyModernStyle() {
    if (bStyleInitialized) {
        return;
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.ChildRounding = 10.0f;
    style.FrameRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.TabRounding = 8.0f;
    style.WindowPadding = ImVec2(18.0f, 14.0f);
    style.FramePadding = ImVec2(10.0f, 7.0f);
    style.ItemSpacing = ImVec2(10.0f, 9.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.08f, 0.11f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.11f, 0.15f, 0.90f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.16f, 0.22f, 0.95f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.25f, 0.34f, 0.95f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.31f, 0.44f, 0.95f);
    colors[ImGuiCol_Button] = ImVec4(0.17f, 0.20f, 0.30f, 0.95f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.35f, 0.50f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.29f, 0.42f, 0.63f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.27f, 0.39f, 0.95f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.40f, 0.55f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.34f, 0.48f, 0.67f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.44f, 0.83f, 1.0f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.44f, 0.83f, 1.0f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.58f, 0.91f, 1.0f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.24f, 0.28f, 0.39f, 0.60f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.33f, 0.46f, 0.80f);
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.94f, 1.0f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.56f, 0.68f, 1.0f);

    bStyleInitialized = true;
}

bool CMenu::RenderSidebarButton(const char* icon, const char* label, ETab tab) {
    const bool isSelected = CurrentTab == tab;

    if (isSelected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.29f, 0.42f, 0.63f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.32f, 0.47f, 0.69f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.26f, 0.39f, 0.58f, 1.0f));
    }

    const bool clicked = ImGui::Button((std::string(icon) + "  " + label).c_str(), ImVec2(-1.0f, 44.0f));

    if (isSelected) {
        ImGui::PopStyleColor(3);
    }

    if (clicked) {
        CurrentTab = tab;
    }

    return clicked;
}

void CMenu::RenderSectionHeader(const char* title, const char* subtitle) {
    ImGui::TextColored(ImVec4(0.58f, 0.91f, 1.0f, 1.0f), "%s", title);
    if (subtitle) {
        ImGui::TextColored(ImVec4(0.64f, 0.71f, 0.84f, 1.0f), "%s", subtitle);
    }
    ImGui::Separator();
}

void CMenu::RenderTechCard(const char* title, const char* description, const char* impact) {
    ImGui::BeginChild((std::string("##TechCard") + title).c_str(), ImVec2(0.0f, 82.0f), true);
    ImGui::TextColored(ImVec4(0.62f, 0.88f, 1.0f, 1.0f), "%s", title);
    ImGui::TextWrapped("%s", description);
    ImGui::TextColored(ImVec4(0.51f, 0.95f, 0.75f, 1.0f), "Impacto: %s", impact);
    ImGui::EndChild();
}

void CMenu::Render() {
    if (!bIsOpen) return;

    ApplyModernStyle();

    ImGui::SetNextWindowSize(ImVec2(1050.0f, 650.0f), ImGuiCond_Once);
    ImGui::Begin("Andromeda // Extreme Optimization Control Center", &bIsOpen, ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(0.62f, 0.90f, 1.0f, 1.0f), "Andromeda CS2");
    ImGui::SameLine();
    ImGui::TextDisabled("| Interface modular para evoluir com novas features");
    ImGui::Separator();

    ImGui::BeginChild("##Sidebar", ImVec2(250.0f, 0.0f), true);
    ImGui::TextColored(ImVec4(0.56f, 0.93f, 1.0f, 1.0f), "Navegacao");
    ImGui::Spacing();
    RenderSidebarButton("A", "Aimbot", ETab::Aimbot);
    RenderSidebarButton("T", "Trigger", ETab::Trigger);
    RenderSidebarButton("V", "Visuals", ETab::Visuals);
    RenderSidebarButton("C", "Config", ETab::Config);
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##Content", ImVec2(0.0f, 0.0f), true);
    switch (CurrentTab) {
    case ETab::Aimbot:
        RenderAimbotTab();
        break;
    case ETab::Trigger:
        RenderTriggerTab();
        break;
    case ETab::Visuals:
        RenderVisualsTab();
        break;
    case ETab::Config:
        RenderConfigTab();
        break;
    default:
        break;
    }
    ImGui::EndChild();

    ImGui::End();
}

void CMenu::RenderAimbotTab() {
    auto& config = CAimbot::GetConfig();

    RenderSectionHeader("Aimbot Control", "Pipeline SIMD pronto para escalar em features futuras");

    ImGui::BeginChild("##AimbotGeneral", ImVec2(0.0f, 180.0f), true);
    ImGui::Checkbox("Enable Aimbot", &config.bEnabled);
    ImGui::ColorEdit4("Aimbot Color", config.aimbotColor, ImGuiColorEditFlags_NoInputs);
    ImGui::SliderFloat("FOV", &config.flFOV, 0.0f, 180.0f, "%.1f°");
    ImGui::SliderFloat("Smooth", &config.flSmooth, 1.0f, 100.0f, "%.1f%%");
    ImGui::SliderInt("Bone Target", &config.iTargetBone, 0, 19, "%d");
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::BeginChild("##AimbotBallistics", ImVec2(0.0f, 95.0f), true);
    ImGui::TextColored(ImVec4(0.73f, 0.86f, 1.0f, 1.0f), "Ballistics & Penetration");
    ImGui::Checkbox("Auto Wall", &config.bAutoWall);
    ImGui::SliderInt("Min Damage", &config.iMinDamage, 1, 100, "%d HP");
    ImGui::EndChild();

    ImGui::Spacing();
    RenderSectionHeader("Stack de Otimizacao", "Design orientado a throughput e baixa latencia");
    RenderTechCard("SIMD Batch W2S (AVX-512)",
        "Projeta os ossos de multiplos jogadores simultaneamente em um unico fluxo vetorial.",
        "Substitui loop escalar por lote paralelo de instrucoes.");
    RenderTechCard("SoA Entity Cache",
        "Mantem X/Y/Z contiguos para leitura direta do alvo no L1 Cache.",
        "Menos cache miss e lookup quase instantaneo.");
    RenderTechCard("NormalizeVectorFast (rsqrt14)",
        "Normaliza o vetor de mira com raiz inversa aproximada assistida por hardware.",
        "Reducao agressiva de ciclos no smoothing e aim direction.");
    RenderTechCard("Arena Allocator (8MB Aligned)",
        "Buffer temporario alinhado em 64B para trajetoria e visibilidade sem malloc/new.",
        "Elimina overhead de syscall e cache line split.");
}

void CMenu::RenderTriggerTab() {
    auto& config = CAimbot::GetConfig();

    RenderSectionHeader("Trigger Control", "Thread de combate separada da telemetria de debug");

    ImGui::BeginChild("##TriggerGeneral", ImVec2(0.0f, 170.0f), true);
    ImGui::Checkbox("Enable Trigger Bot", &config.bTriggerEnabled);
    ImGui::ColorEdit4("Trigger Color", config.triggerColor, ImGuiColorEditFlags_NoInputs);
    ImGui::SliderInt("Hit Group", &config.iTriggerHitGroup, 0, 6, "%d");
    ImGui::SliderInt("Reaction Delay (ms)", &config.iTriggerDelay, 0, 100, "%d ms");
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::BeginChild("##TriggerPenetration", ImVec2(0.0f, 90.0f), true);
    ImGui::TextColored(ImVec4(0.73f, 0.86f, 1.0f, 1.0f), "Penetration Filter");
    ImGui::Checkbox("Auto Wall Filter", &config.bTriggerAutoWall);
    ImGui::SliderInt("Min Damage", &config.iTriggerMinDamage, 1, 100, "%d HP");
    ImGui::EndChild();

    ImGui::Spacing();
    RenderSectionHeader("Stack de Otimizacao", "Assincronia agressiva sem lock na thread critica");
    RenderTechCard("MPSC Ring-Buffer",
        "Publica eventos de debug (FOV, linhas e estado interno) para consumo assíncrono.",
        "UI nunca bloqueia a logica principal do trigger.");
    RenderTechCard("Thread-Local Staging",
        "Cada core prepara comandos de disparo isoladamente antes da fase de commit.",
        "Escala em multi-thread sem race condition.");
}

void CMenu::RenderVisualsTab() {
    auto& config = CAimbot::GetConfig();

    RenderSectionHeader("Visuals", "ESP modular para expandir overlays no futuro");
    ImGui::BeginChild("##VisualsGeneral", ImVec2(0.0f, 145.0f), true);
    ImGui::Checkbox("Enable ESP", &config.bESP);
    ImGui::Checkbox("Box ESP", &config.bBoxESP);
    ImGui::Checkbox("Skeleton", &config.bSkeleton);
    ImGui::SliderFloat("ESP Distance", &config.flESPDistance, 0.0f, 1000.0f, "%.0f units");
    ImGui::EndChild();

    ImGui::Spacing();
    RenderSectionHeader("Stack de Otimizacao", "Projecao precisa para muitos alvos simultaneos");
    RenderTechCard("FMA (_mm512_fmadd_ps)",
        "A multiplicacao da matriz de projecao usa fused-multiply-add de hardware.",
        "Mais precisao numerica com menos custo de instrucao.");
    RenderTechCard("SIMD Batch W2S",
        "Converte ossos de multiplos players em screen-space no mesmo batch vetorial.",
        "Melhor taxa de atualizacao do ESP sem gargalo CPU.");
}

void CMenu::RenderConfigTab() {
    static char configName[64] = "";

    RenderSectionHeader("Config Manager", "Camada pronta para plug-in de storage e presets");
    ImGui::BeginChild("##ConfigEditor", ImVec2(0.0f, 120.0f), true);
    ImGui::InputText("Config Name", configName, IM_ARRAYSIZE(configName));
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::BeginChild("##ConfigActions", ImVec2(0.0f, 75.0f), true);
    if (ImGui::Button("Save Config", ImVec2(220, 36))) {
        // Implementar save de config
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Config", ImVec2(220, 36))) {
        // Implementar load de config
    }
    ImGui::EndChild();

    ImGui::Spacing();
    RenderSectionHeader("System Telemetry");
    ImGui::BeginChild("##SystemInfo", ImVec2(0.0f, 120.0f), true);
    ImGui::BulletText("AVX-512 Support: %s", CAimbot::HasAVX512() ? "YES" : "NO");
    ImGui::BulletText("Thread Count: %d", CAimbot::GetOptimalThreadCount());
    ImGui::BulletText("Arena Memory: 8MB aligned (64B)");
    ImGui::BulletText("Renderer pipeline: Modular sections + reusable cards");
    ImGui::EndChild();
}
