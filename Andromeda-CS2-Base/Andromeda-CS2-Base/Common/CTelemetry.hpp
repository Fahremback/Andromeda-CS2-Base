#pragma once
#include <string>
#include <chrono>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>
#include "../../CS2/SDK/Math/Vector3.hpp"

struct TestMetrics {
    Vector3 Current_Target_Pos;
    Vector3 Local_View_Angles;
    bool Aimbot_Active_Status = false;
    float FPS = 0.0f;
    float Hook_CreateMove_Us = 0.0f;
    float Hook_FrameStageNotify_Us = 0.0f;
    float CPhysicsAligner_ScanObjectCluster_Us = 0.0f;
    float CPhysicsAligner_SolveConstraint_Us = 0.0f;
    float CPhysicsAligner_ProjectCoordinates_Us = 0.0f;
    float CPhysicsSimulationMocks_ApplyCollision_Us = 0.0f;
    float RenderStack_Us = 0.0f;
    uint64_t Total_Uptime_Seconds = 0;
    size_t Active_Entities_Last_Frame = 0;
    size_t Entities_Hitbox_Tracked = 0;
    bool Cheat_Injected = true;
    std::string Last_Error;
    std::unordered_map<std::string, double> functionExecutionTimesMs;
};

class CTelemetry {
public:
    static void Init();
    static void Shutdown();
    static void SetEnabled(bool enabled);
    static bool IsEnabled();
    static void LogAimbotMetrics(const Vector3& targetPos, const Vector3& viewAngles, bool isActive);
    static void LogFunctionTime(const std::string& funcName, double timeMs);
    static void SetFPS(float fps);
    static void SetHookTime(const std::string& hookName, double microseconds);
    static void SetActiveEntityCount(size_t count);
    static void ReportError(const std::string& error);
    static void Flush();

private:
    static TestMetrics m_metrics;
    static std::mutex m_mutex;
    static std::chrono::steady_clock::time_point m_startTime;
    static std::atomic<bool> m_enabled;
};

class ScopedProfiler {
public:
    ScopedProfiler(const std::string& name) : m_name(name) {
        m_start = std::chrono::high_resolution_clock::now();
    }
    ~ScopedProfiler() {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - m_start;
        CTelemetry::LogFunctionTime(m_name, duration.count());
    }
private:
    std::string m_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

class ScopedHookTimer {
public:
    ScopedHookTimer(const std::string& hookName)
        : m_hookName(hookName), m_start(std::chrono::high_resolution_clock::now()) {}
    ~ScopedHookTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> duration = end - m_start;
        CTelemetry::SetHookTime(m_hookName, duration.count());
    }
private:
    std::string m_hookName;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

#define PROFILE_FUNCTION() ScopedProfiler _profiler(__FUNCTION__)
#define PROFILE_HOOK(name) ScopedHookTimer _hook_timer(name)
