#include "compilation_handler.h"
#include <sstream>
#include <fstream>
#include <filesystem>
#include <random>
#include <chrono>
#include <cstdio>
#include <thread>
#include <future>
#include <stdexcept>
#include <algorithm>

namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace lisa::server;

CompilationHandler::CompilationHandler(const std::string& build_root_path, size_t max_concurrent_jobs)
    : build_root_path_(build_root_path), max_concurrent_jobs_(max_concurrent_jobs), stop_workers_(false) {
    // 创建构建根目录
    fs::create_directories(build_root_path_);

    // 启动工作线程
    for (size_t i = 0; i < max_concurrent_jobs_; ++i) {
        worker_threads_.emplace_back(&CompilationHandler::worker_thread, this);
    }
}

CompilationHandler::~CompilationHandler() {
    stop_workers_ = true;
    job_condition_.notify_all();

    // 等待所有工作线程结束
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // 清理所有构建目录
    for (const auto& entry : jobs_) {
        fs::remove_all(create_build_directory(entry.first));
    }
}

std::string CompilationHandler::generate_job_id() {
    // 生成基于时间戳和随机数的唯一任务ID
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch().count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    std::stringstream ss;
    ss << value << "-" << dis(gen);
    return ss.str();
}

std::string CompilationHandler::create_build_directory(const std::string& job_id) {
    return build_root_path_ + "/" + job_id;
}

std::string CompilationHandler::get_compile_command(const std::string& repo_path, const json& config) {
    std::stringstream cmd;

    // 设置环境变量
    if (config.contains("environment") && config["environment"].contains("variables")) {
        for (const auto& var : config["environment"]["variables"]) {
            cmd << var["name"] << "=\"" << var["value"] << "\" ";
        }
    }

    // 切换到仓库目录
    cmd << "cd \"" << repo_path << "\" && ";

    // 添加编译命令
    if (config.contains("build") && config["build"].contains("command")) {
        cmd << config["build"]["command"];
    } else {
        // 默认编译命令
        cmd << "make -j" << std::thread::hardware_concurrency();
    }

    // 重定向输出到日志文件
    cmd << " > build.log 2>&1";

    return cmd.str();
}

int CompilationHandler::execute_compilation(CompilationJob& job) {
    try {
        // 创建构建目录
        std::string build_dir = create_build_directory(job.id);
        fs::create_directories(build_dir);

        // 获取编译命令
        std::string compile_cmd = get_compile_command(job.repo_path, job.config);
        Logger::info("Executing compilation command for job " + job.id + ": " + compile_cmd);

        // 执行编译命令
        job.started_at = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        job.status = CompilationStatus::RUNNING;
        job.progress = 10;

        // 使用popen执行命令并捕获输出
        FILE* pipe = popen(compile_cmd.c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("Failed to execute compilation command");
        }

        // 定期检查取消标志
        std::thread cancellation_checker([&]() {
            while (job.status == CompilationStatus::RUNNING && !job.cancelled) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (job.cancelled) {
                // 在实际实现中，这里应该获取进程ID并终止它
                Logger::info("Compilation job " + job.id + " cancelled");
            }
        });

        // 等待命令完成
        int exit_code = pclose(pipe);
        cancellation_checker.join();

        job.progress = 100;
        job.completed_at = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        // 读取输出日志
        std::string log_path = job.repo_path + "/build.log";
        std::ifstream log_file(log_path);
        if (log_file.is_open()) {
            job.output = std::string((std::istreambuf_iterator<char>(log_file)),
                                     std::istreambuf_iterator<char>());
            // 限制输出大小
            if (job.output.size() > 1024 * 1024) { // 1MB
                job.output = job.output.substr(0, 1024 * 1024) + "\n[Output truncated]";
            }
        }

        // 设置最终状态
        if (job.cancelled) {
            job.status = CompilationStatus::CANCELLED;
            return -1;
        } else if (WIFEXITED(exit_code) && WEXITSTATUS(exit_code) == 0) {
            job.status = CompilationStatus::COMPLETED;
            return 0;
        } else {
            job.status = CompilationStatus::FAILED;
            return WEXITSTATUS(exit_code);
        }
    } catch (const std::exception& e) {
        job.status = CompilationStatus::FAILED;
        job.output = "Compilation error: " + std::string(e.what());
        job.completed_at = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        return -1;
    }
}

void CompilationHandler::worker_thread() {
    while (!stop_workers_) {
        std::unique_lock<std::mutex> lock(jobs_mutex_);
        job_condition_.wait(lock, [this] { return !job_queue_.empty() || stop_workers_; });

        if (stop_workers_) break;

        std::string job_id = job_queue_.front();
        job_queue_.pop();

        lock.unlock();

        auto it = jobs_.find(job_id);
        if (it == jobs_.end()) {
            Logger::error("Job not found: " + job_id);
            continue;
        }

        CompilationJob& job = *it->second;
        job.exit_code = execute_compilation(job);
    }
}

std::string CompilationHandler::create_job(const std::string& repo_path, const json& config) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    std::string job_id = generate_job_id();
    auto job = std::make_unique<CompilationJob>();

    job->id = job_id;
    job->repo_path = repo_path;
    job->config = config;
    job->status = CompilationStatus::PENDING;
    job->progress = 0;
    job->exit_code = -1;
    job->cancelled = false;
    job->started_at = 0;
    job->completed_at = 0;

    jobs_[job_id] = std::move(job);
    job_queue_.push(job_id);

    job_condition_.notify_one();
    Logger::info("Created new compilation job: " + job_id);

    return job_id;
}

std::optional<JobStatusInfo> CompilationHandler::get_job_status(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return std::nullopt;
    }

    const auto& job = it->second;
    JobStatusInfo status_info;

    status_info.job_id = job->id;
    status_info.progress = job->progress;
    status_info.started_at = job->started_at;
    status_info.completed_at = job->completed_at;
    status_info.completed = job->status == CompilationStatus::COMPLETED || 
                           job->status == CompilationStatus::FAILED || 
                           job->status == CompilationStatus::CANCELLED;

    switch (job->status) {
        case CompilationStatus::PENDING: status_info.status = "pending"; break;
        case CompilationStatus::RUNNING: status_info.status = "running"; break;
        case CompilationStatus::COMPLETED: status_info.status = "completed"; break;
        case CompilationStatus::FAILED: status_info.status = "failed"; break;
        case CompilationStatus::CANCELLED: status_info.status = "cancelled"; break;
        default: status_info.status = "unknown"; break;
    }

    return status_info;
}

std::optional<JobResultInfo> CompilationHandler::get_job_result(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return std::nullopt;
    }

    const auto& job = it->second;
    JobResultInfo result_info;

    result_info.job_id = job->id;
    result_info.exit_code = job->exit_code;
    result_info.output = job->output;
    result_info.completed_at = job->completed_at;
    result_info.completed = job->status == CompilationStatus::COMPLETED || 
                           job->status == CompilationStatus::FAILED || 
                           job->status == CompilationStatus::CANCELLED;

    switch (job->status) {
        case CompilationStatus::COMPLETED: result_info.status = "completed"; break;
        case CompilationStatus::FAILED: result_info.status = "failed"; break;
        case CompilationStatus::CANCELLED: result_info.status = "cancelled"; break;
        default: result_info.status = "in_progress"; break;
    }

    return result_info;
}

bool CompilationHandler::cancel_job(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return false;
    }

    auto& job = it->second;
    if (job->status == CompilationStatus::COMPLETED || 
        job->status == CompilationStatus::FAILED || 
        job->status == CompilationStatus::CANCELLED) {
        return false;
    }

    job->cancelled = true;
    return true;
}

void CompilationHandler::clean_expired_jobs(time_t max_age_seconds) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    for (auto it = jobs_.begin(); it != jobs_.end();) {
        const auto& job = it->second;
        if (job->completed_at > 0 && now - job->completed_at > max_age_seconds) {
            Logger::info("Cleaning expired job: " + job->id);
            fs::remove_all(create_build_directory(job->id));
            it = jobs_.erase(it);
        } else {
            ++it;
        }
    }
}