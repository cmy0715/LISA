#include <iostream>
#include <string>
#include <thread>
#include <future>
#include <httplib.h>
#include <git2.h>
#include <yaml-cpp/yaml.h>
#include "server.h"
#include "git_handler.h"
#include "compilation_handler.h"
#include "config.h"
#include "logger.h"

using namespace httplib;
using namespace lisa::server;

int main() {
    // 加载服务器配置
    Config config;
    if (!config.load("server_config.yaml")) {
        Logger::error("Failed to load server configuration");
        return 1;
    }

    // 初始化组件
    GitHandler git_handler(config.git_repo_path());
    CompilationHandler compilation_handler(config.build_root_path());
    Server server(config, git_handler, compilation_handler);

    // 设置路由
    Server::set_routes(server);

    // 启动服务器
    Logger::info("Starting LISA remote compilation server on port " + std::to_string(config.port()));
    if (!server.start()) {
        Logger::error("Failed to start server");
        return 1;
    }

    Logger::info("Server stopped");
    return 0;
}