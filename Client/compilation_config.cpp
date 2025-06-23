#include "compilation_config.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace lisa {

CompilationConfigManager::CompilationConfigManager(const std::string& config_path) : config_path_(config_path) {}

bool CompilationConfigManager::loadConfig() {
    if (!fs::exists(config_path_)) {
        std::cerr << "配置文件不存在: " << config_path_ << std::endl;
        return false;
    }
    return parseYaml(config_path_);
}

bool CompilationConfigManager::parseYaml(const std::string& file_path) {
    try {
        YAML::Node root = YAML::LoadFile(file_path);

        // 解析编译器配置
        if (root["compiler"]) {
            config_.compiler.type = root["compiler"]["type"].as<std::string>();
            config_.compiler.version = root["compiler"]["version"].as<std::string>();
            config_.compiler.options = root["compiler"]["options"].as<std::vector<std::string>>();
        } else {
            throw std::runtime_error("缺少编译器配置");
        }

        // 解析构建配置
        if (root["build"]) {
            config_.build.command = root["build"]["command"].as<std::string>();
            config_.build.working_dir = root["build"]["working_dir"].as<std::string>();
        } else {
            throw std::runtime_error("缺少构建配置");
        }

        // 解析环境变量
        if (root["env"]) {
            for (const auto& node : root["env"]) {
                std::string key = node.first.as<std::string>();
                std::string value = node.second.as<std::string>();
                config_.env.variables[key] = value;
            }
        }

        return isValid();

    } catch (const YAML::Exception& e) {
        std::cerr << "YAML解析错误: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "配置解析错误: " << e.what() << std::endl;
        return false;
    }
}

bool CompilationConfigManager::isValid() const {
    if (config_.compiler.type.empty() || config_.compiler.version.empty()) {
        std::cerr << "无效的编译器配置"
        return false;
    }
    if (config_.build.command.empty()) {
        std::cerr << "无效的构建命令"
        return false;
    }
    return true;
}

} // namespace lisa