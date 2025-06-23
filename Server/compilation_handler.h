#ifndef COMPILATION_HANDLER_H
#define COMPILATION_HANDLER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <future>
#include <chrono>
#include <condition_variable>
#include <queue>
#include <nlohmann/json.hpp>
#include "logger.h"

namespace lisa::server {

// 编译任务状态
enum class CompilationStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

// 编译任务信息
struct CompilationJob {
    std::string id;
    std::string repo_path;
    nlohmann::json config;
    CompilationStatus status;
    int progress;
    int exit_code;
    std::string output;
    time_t started_at;
    time_t completed_at;
    std::future<int> future;
    std::atomic<bool> cancelled;
};

// 编译任务状态信息（用于API返回）
struct JobStatusInfo {
    std::string job_id;
    std::string status;
    int progress;
    time_t started_at;
    time_t completed_at;
    bool completed;
};

// 编译任务结果信息（用于API返回）
struct JobResultInfo {
    std::string job_id;
    std::string status;
    int exit_code;
    std::string output;
    time_t completed_at;
    bool completed;
};

class CompilationHandler {
public:
    CompilationHandler(const std::string& build_root_path, size_t max_concurrent_jobs = 4);
    ~CompilationHandler();

    // 创建新的编译任务
    std::string create_job(const std::string& repo_path, const nlohmann::json& config);

    // 获取任务状态
    std::optional<JobStatusInfo> get_job_status(const std::string& job_id);

    // 获取任务结果
    std::optional<JobResultInfo> get_job_result(const std::string& job_id);

    // 取消任务
    bool cancel_job(const std::string& job_id);

    // 清理过期任务
    void clean_expired_jobs(time_t max_age_seconds);

private:
    std::string build_root_path_;
    size_t max_concurrent_jobs_;
    std::unordered_map<std::string, std::unique_ptr<CompilationJob>> jobs_;
    std::queue<std::string> job_queue_;
    std::vector<std::thread> worker_threads_;
    std::mutex jobs_mutex_;
    std::condition_variable job_condition_;
    std::atomic<bool> stop_workers_;

    // 生成唯一任务ID
    std::string generate_job_id();

    // 工作线程函数
    void worker_thread();

    // 执行编译任务
    int execute_compilation(CompilationJob& job);

    // 解析编译配置
    std::string get_compile_command(const std::string& repo_path, const nlohmann::json& config);

    // 创建构建目录
    std::string create_build_directory(const std::string& job_id);
};

} // namespace lisa::server

#endif // COMPILATION_HANDLER_H