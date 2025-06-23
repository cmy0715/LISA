#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <nlohmann/json.hpp>
#include "logger.h"

namespace lisa::server {

class Config {
public:
    Config() = default;
    ~Config() = default;

    // 加载配置文件
    bool load(const std::string& file_path);

    // 获取服务器主机地址
    const std::string& host() const { return host_; }

    // 获取服务器端口
    int port() const { return port_; }

    // 获取Git仓库存储路径
    const std::string& git_repo_path() const { return git_repo_path_; }

    // 获取构建根目录
    const std::string& build_root_path() const { return build_root_path_; }

    // 获取最大并发编译任务数
    size_t max_concurrent_jobs() const { return max_concurrent_jobs_; }

    // 获取任务过期时间(秒)
    time_t job_expiration_seconds() const { return job_expiration_seconds_; }

    // 获取仓库缓存过期时间(秒)
    time_t repo_cache_expiration_seconds() const { return repo_cache_expiration_seconds_; }

    // 获取配置的JSON对象
    const nlohmann::json& get_json() const { return config_json_; }

private:
    std::string host_ = "0.0.0.0";          // 服务器主机地址
    int port_ = 8080;                        // 服务器端口
    std::string git_repo_path_ = "./repos"; // Git仓库存储路径
    std::string build_root_path_ = "./builds"; // 构建根目录
    size_t max_concurrent_jobs_ = 4;         // 最大并发编译任务数
    time_t job_expiration_seconds_ = 3600;   // 任务过期时间(秒)
    time_t repo_cache_expiration_seconds_ = 86400; // 仓库缓存过期时间(秒)
    nlohmann::json config_json_;             // 完整配置JSON对象

    // 验证配置有效性
    bool validate();
};

} // namespace lisa::server

#endif // CONFIG_H