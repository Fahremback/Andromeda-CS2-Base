#pragma once
#include <imgui.h>
#include "CAimbot.hpp"
#include "CConfig.hpp"

class CMenu {
public:
    static void Render();
    static void Initialize();
    static void Toggle();

private:
    enum class ETab {
        Aimbot = 0,
        Trigger,
        Visuals,
        Config
    };

    static bool bIsOpen;
    static bool bStyleInitialized;
    static ETab CurrentTab;

    static void ApplyModernStyle();
    static bool RenderSidebarButton(const char* icon, const char* label, ETab tab);
    static void RenderSectionHeader(const char* title, const char* subtitle = nullptr);
    static void RenderTechCard(const char* title, const char* description, const char* impact);

    static void RenderAimbotTab();
    static void RenderTriggerTab();
    static void RenderVisualsTab();
    static void RenderConfigTab();
};
