#pragma once

#include <Windows.h>

#include "Common/Common.h"

auto __forceinline PrintMessage( const char* fmt , ... ) -> void;

#define DEV_LOG( fmt , ... ) \
{\
PrintMessage( fmt , __VA_ARGS__ );\
}
