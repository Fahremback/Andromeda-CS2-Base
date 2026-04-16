#pragma once

#include <Common/Common.hpp>
#include <map>
#include <string>

class CFont;
class CFontManager
{
public:
	auto OnInit() -> void;
	auto OnDestroy() -> void;

public:
	auto GetFont( const char* szName ) -> CFont* { return m_Fonts[szName]; }

private:
	std::map<std::string , CFont*> m_Fonts;
};

auto GetFontManager() -> CFontManager*;
