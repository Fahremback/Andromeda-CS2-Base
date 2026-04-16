#include "CFontManager.hpp"

#include <AndromedaClient/CAndromedaGUI.hpp>

static CFontManager g_FontManager{};

auto CFontManager::OnInit() -> void
{
}

auto CFontManager::OnDestroy() -> void
{
}

auto GetFontManager() -> CFontManager*
{
	return &g_FontManager;
}
