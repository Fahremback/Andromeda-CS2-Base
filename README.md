# Featues:
- There's a crashlog in case of an apocalypse (load the dll in x64dbg and hit home, then subtract 0x1000 from the starting address and add the crash offset and look at the location).
- Schema dumper (in Common/Include/Config.hpp) -> DUMP_SCHEMA_ALL_OFFSET to 1
- Blackbone mm support (I'll post the injector with the database when I have time).
- There's a move_crc bypass (all actions are done in AndromedaClient.cpp -> OnCreateMove.GetCL_Bypass()->SetViewAngles).
- There's everything you need for inventory management; you just need to save it from the UC, for example. All patterns, etc., are working (maybe if this becomes popular, I'll post an article on how to do it).
- There are basic visuals with a visual check.
- Easy logs via DEV_LOG("pasters server\n");
- There's a protobuff message parser. An example for sound esp is in Hook_ParseMessage.cpp with sound positions.
- Normal working rendering of all the [removed] and fonts (Example in AndromedaClient.cpp, CVIsual.cpp)
- Possibly something else, but I can't remember. Good luck to everyone.

# Performance & Toolchain Baseline

## MSVC (x64 Release)
- Language: C++20 (`/std:c++20`)
- ISA and math: `/arch:AVX512`, fast FP (`/fp:fast`), intrinsic enable (`/Oi`)
- Optimization profile: `/O2 /Ot /Ob3 /Os /Gw /Gy /GF /MP`
- Runtime cost reductions: exceptions off (`/EHsc-` equivalent project setting), RTTI off (`/GR-`), GS off (`/GS-`)

## GCC/Clang target profile (for non-MSVC ports)
- `-O3 -march=native -mtune=native`
- `-ffast-math -mfma`
- `-fno-exceptions -fno-rtti`

## Runtime memory/layout baseline
- 8MB linear arena allocator aligned to 64 bytes.
- AVX-512 accelerated arena reset path (64-byte stores) with memset fallback.
- Entity cache migrated to fixed-capacity SoA-style aligned buffers (no `std::vector` growth in hot paths).
- Explicit prefetch (`_mm_prefetch`, T0) in visual iteration loops.

## SIMD/math baseline
- Added batched world-to-screen API with AVX path for screen-space conversion.
- FMA path enabled for vector post-transform multiply-add where available.
- Fast vector normalization helper using rsqrt14 path on AVX-512 capable builds.

## Runtime pipeline/concurrency baseline
- Startup stages now measured with `QueryPerformanceCounter` and logged (`[perf] ... ms`).
- `CAndromedaClient` core state uses `std::atomic_bool` with acquire/release semantics for lightweight cross-thread visibility.
- Render path now snapshots entity cache metadata from a short lock window into arena memory, then renders lock-free from the snapshot.

## DX11 render baseline
- Backbuffer RTV creation now uses inferred descriptor (`CreateRenderTargetView(..., nullptr, ...)`) for better compatibility with swapchain formats.
- Present path now backs up and restores OM render targets + viewport state around ImGui rendering to reduce game pipeline interference.

# Links:
[UnknownCheats Thread](https://www.unknowncheats.me/forum/counter-strike-2-a/722929-andromeda-cs2-internal-base.html)<br>
[Powered by Vermillion Hack](https://vermillion-hack.ru/)

# ScreenShots:

<img width="1600" height="900" alt="image" src="https://github.com/user-attachments/assets/106f81d5-e24f-44af-8449-74b1ca1d94ff" />
<img width="1600" height="900" alt="image" src="https://github.com/user-attachments/assets/d72f589d-832c-4af3-9ba2-64ea25c98e5e" />
<img width="1600" height="900" alt="image" src="https://github.com/user-attachments/assets/97d0d9a4-b7b4-4447-81de-5d7ed263907b" />
