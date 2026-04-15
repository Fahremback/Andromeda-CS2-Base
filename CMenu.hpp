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
    static bool bIsOpen;
    static void RenderAimbotTab();
    static void RenderTriggerTab();
    static void RenderVisualsTab();
    static void RenderConfigTab();
};
