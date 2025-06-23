#include "config.h"
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace lisa::server;

bool Config::load(const std::string& file_path) {
    try {
        if (!fs::exists(file_path)) {
            Logger::warn("Config file not found: " + file_path + ", using default configuration");
            return validate(); // 使用默认配置
        }

        // 加载并解析YAML配置文件
        YAML::Node config = YAML::LoadFile(file_path);
        config_json_ = nlohmann::json::parse(config.as<std::string>());

        // 服务器配置
        if (config["server"]) {
            if (config["server"]["host"]) {
                host_ = config["server"]["host"].as<std::string>();
            }
            if (config["server"]["port"]) {
                port_ = config["server"]["port"].as<int>();
            }
        }

        // Git配置
        if (config["git"]) {
            if (config["git"]["repo_path"]) {
                git_repo_path_ = config["git"]["repo_path"].as<std::string>();
            }
            if (config["git"]["cache_expiration_seconds"]) {
                repo_cache_expiration_seconds_ = config["git"]["cache_expiration_seconds"].as<time_t>();
            }
        }

        // 编译配置
        if (config["compilation"]) {
            if (config["compilation"]["build_root_path"]) {
                build_root_path_ = config["compilation"]["build_root_path"].as<std::string>();
            }
            if (config["compilation"]["max_concurrent_jobs"]) {
                max_concurrent_jobs_ = config["compilation"]["max_concurrent_jobs"].as<size_t>();
            }
            if (config["compilation"]["job_expiration_seconds"]) {
                job_expiration_seconds_ = config["compilation"]["job_expiration_seconds"].as<time_t>();
            }
        }

        // 验证配置有效性
        return validate();
    } catch (const YAML::Exception& e) {
        Logger::error("Failed to parse config file: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        Logger::error("Error loading config: " + std::string(e.what()));
        return false;
    }
}

bool Config::validate() {
    try {
        // 验证服务器端口
        if (port_ <= 0 || port_ > 65535) {
            throw std::invalid_argument("Invalid port number: " + std::to_string(port_));
        }

        // 验证最大并发任务数
        if (max_concurrent_jobs_ == 0) {
            throw std::invalid_argument("Max concurrent jobs must be greater than 0");
        }

        // 验证路径 - 如果不存在则创建
        if (!fs::exists(git_repo_path_)) {
            Logger::info("Creating git repo directory: " + git_repo_path_);
            fs::create_directories(git_repo_path_);
        }

        if (!fs::exists(build_root_path_)) {
            Logger::info("Creating build root directory: " + build_root_path_);
            fs::create_directories(build_root_path_);
        }

        // 验证过期时间
        if (job_expiration_seconds_ < 60) {
            Logger::warn("Job expiration time is too short (less than 60 seconds)");
        }

        if (repo_cache_expiration_seconds_ < 300) {
            Logger::warn("Repo cache expiration time is too short (less than 300 seconds)");
        }

        Logger::info("Configuration validated successfully");
        return true;
    } catch (const std::exception& e) {
        Logger::error("Configuration validation failed: " + std::string(e.what()));
        return false;
    }
}