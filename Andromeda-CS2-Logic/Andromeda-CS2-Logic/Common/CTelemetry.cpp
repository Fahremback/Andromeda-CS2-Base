#include "CTelemetry.hpp"

TestMetrics CTelemetry::m_metrics{};
std::mutex CTelemetry::m_mutex{};
std::chrono::steady_clock::time_point CTelemetry::m_startTime = std::chrono::steady_clock::now();
std::atomic<bool> CTelemetry::m_enabled{ false };

void CTelemetry::Init() {
    m_startTime = std::chrono::steady_clock::now();
    m_metrics.Cheat_Injected = true;
}

void CTelemetry::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.Cheat_Injected = false;
    Flush();
}

void CTelemetry::SetEnabled(bool enabled) {
    m_enabled.store(enabled, std::memory_order_release);
}

bool CTelemetry::IsEnabled() {
    return m_enabled.load(std::memory_order_acquire);
}

void CTelemetry::LogAimbotMetrics(const Vector3& targetPos, const Vector3& viewAngles, bool isActive) {
    if (!IsEnabled()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.Current_Target_Pos = targetPos;
    m_metrics.Local_View_Angles = viewAngles;
    m_metrics.Aimbot_Active_Status = isActive;
}

void CTelemetry::LogFunctionTime(const std::string& funcName, double timeMs) {
    if (!IsEnabled()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_metrics.functionExecutionTimesMs.find(funcName) != m_metrics.functionExecutionTimesMs.end()) {
        m_metrics.functionExecutionTimesMs[funcName] = m_metrics.functionExecutionTimesMs[funcName] * 0.9 + timeMs * 0.1;
    } else {
        m_metrics.functionExecutionTimesMs[funcName] = timeMs;
    }
}

void CTelemetry::SetFPS(float fps) {
    if (!IsEnabled()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.FPS = fps;
}

void CTelemetry::SetHookTime(const std::string& hookName, double microseconds) {
    if (!IsEnabled()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (hookName == "CreateMove") m_metrics.Hook_CreateMove_Us = static_cast<float>(microseconds);
    else if (hookName == "FrameStageNotify") m_metrics.Hook_FrameStageNotify_Us = static_cast<float>(microseconds);
    else if (hookName == "ScanObjectCluster") m_metrics.CPhysicsAligner_ScanObjectCluster_Us = static_cast<float>(microseconds);
    else if (hookName == "SolveConstraint") m_metrics.CPhysicsAligner_SolveConstraint_Us = static_cast<float>(microseconds);
    else if (hookName == "ProjectCoordinates") m_metrics.CPhysicsAligner_ProjectCoordinates_Us = static_cast<float>(microseconds);
    else if (hookName == "ApplyCollisionSnapshot") m_metrics.CPhysicsSimulationMocks_ApplyCollision_Us = static_cast<float>(microseconds);
    else if (hookName == "RenderStack") m_metrics.RenderStack_Us = static_cast<float>(microseconds);
}

void CTelemetry::SetActiveEntityCount(size_t count) {
    if (!IsEnabled()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.Active_Entities_Last_Frame = count;
}

void CTelemetry::ReportError(const std::string& error) {
    if (!IsEnabled()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.Last_Error = error;
}

void CTelemetry::Flush() {
    if (!IsEnabled()) return;
    static auto lastFlush = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFlush).count() < 100) return;
    lastFlush = now;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.Total_Uptime_Seconds = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime).count());

    std::ofstream file("telemetry.json", std::ios::out | std::ios::trunc);
    if (!file.is_open()) return;

    file << "{\n";
    file << "  \"FPS\": " << m_metrics.FPS << ",\n";
    file << "  \"Cheat_Injected\": " << (m_metrics.Cheat_Injected ? "true" : "false") << ",\n";
    file << "  \"Uptime_Seconds\": " << m_metrics.Total_Uptime_Seconds << ",\n";
    file << "  \"Last_Error\": \"" << m_metrics.Last_Error << "\",\n";
    file << "  \"Hook_Timing_Us\": {\n";
    file << "    \"CreateMove\": " << m_metrics.Hook_CreateMove_Us << ",\n";
    file << "    \"FrameStageNotify\": " << m_metrics.Hook_FrameStageNotify_Us << ",\n";
    file << "    \"ScanObjectCluster\": " << m_metrics.CPhysicsAligner_ScanObjectCluster_Us << ",\n";
    file << "    \"SolveConstraint\": " << m_metrics.CPhysicsAligner_SolveConstraint_Us << ",\n";
    file << "    \"ProjectCoordinates\": " << m_metrics.CPhysicsAligner_ProjectCoordinates_Us << ",\n";
    file << "    \"ApplyCollisionSnapshot\": " << m_metrics.CPhysicsSimulationMocks_ApplyCollision_Us << ",\n";
    file << "    \"RenderStack\": " << m_metrics.RenderStack_Us << "\n";
    file << "  },\n";
    file << "  \"Aimbot\": {\n";
    file << "    \"Active\": " << (m_metrics.Aimbot_Active_Status ? "true" : "false") << ",\n";
    file << "    \"Target_Pos\": {\"x\": " << m_metrics.Current_Target_Pos.m_x << ", \"y\": " << m_metrics.Current_Target_Pos.m_y << ", \"z\": " << m_metrics.Current_Target_Pos.m_z << "},\n";
    file << "    \"View_Angles\": {\"x\": " << m_metrics.Local_View_Angles.m_x << ", \"y\": " << m_metrics.Local_View_Angles.m_y << ", \"z\": " << m_metrics.Local_View_Angles.m_z << "}\n";
    file << "  },\n";
    file << "  \"Active_Entities\": " << m_metrics.Active_Entities_Last_Frame << ",\n";
    file << "  \"Profiling\": {\n";
    bool first = true;
    for (const auto& pair : m_metrics.functionExecutionTimesMs) {
        if (!first) file << ",\n";
        file << "    \"" << pair.first << "\": " << pair.second;
        first = false;
    }
    file << "\n  }\n";
    file << "}\n";
}
