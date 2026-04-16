#pragma once

#include <Common/Common.hpp>

#include <AndromedaClient/Fonts/CFont.hpp>

class CFontManager final
{
public:
	auto FirstInitFonts() -> void;

public:
	CFont m_VerdanaFont;

private:
	bool m_bInit = false;
};

auto GetFontManager() -> CFontManager*;
