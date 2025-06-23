#include <iostream>
#include <string>
#include <vector>
#include "CLI/CLI.hpp"
#include "check_renewing_git.h"
#include "local_to_server_git.h"
#include "compilation_config.h"

int main(int argc, char** argv) {
    CLI::App app("LISA Remote Compilation System");

    // 全局选项
    std::string config_path = ".lisa.yaml";
    app.add_option("-c,--config", config_path, "Path to compilation config file");

    // 子命令: check - 检查本地与仓库差异
    auto* check_cmd = app.add_subcommand("check", "Check differences between local files and remote repository");
    std::string local_dir_check;
    std::string repo_url_check;
    check_cmd->add_option("local_dir", local_dir_check, "Local directory to check")->required();
    check_cmd->add_option("repo_url", repo_url_check, "Remote repository URL")->required();

    // 子命令: push - 推送本地代码到远程
    auto* push_cmd = app.add_subcommand("push", "Push local code to remote repository");
    std::string local_dir_push;
    std::string repo_url_push;
    std::string branch_name;
    std::vector<std::string> exclude_patterns;
    push_cmd->add_option("local_dir", local_dir_push, "Local directory to push")->required();
    push_cmd->add_option("repo_url", repo_url_push, "Remote repository URL")->required();
    push_cmd->add_option("-b,--branch", branch_name, "Branch name to create")->default_val("main");
    push_cmd->add_option("-e,--exclude", exclude_patterns, "Patterns to exclude from push");

    // 子命令: config - 显示配置信息
    auto* config_cmd = app.add_subcommand("config", "Show compilation configuration");

    CLI11_PARSE(app, argc, argv);

    try {
        lisa::CompilationConfigManager config_manager;
        config_manager.loadConfig(config_path);

        if (*check_cmd) {
            std::cout << "Checking differences between " << local_dir_check << " and " << repo_url_check << std::endl;
            lisa::GitChecker checker;
            auto diffs = checker.compareLocalWithRepo(local_dir_check, repo_url_check);
            for (const auto& diff : diffs) {
                std::cout << diff << std::endl;
            }
        } else if (*push_cmd) {
            std::cout << "Pushing " << local_dir_push << " to " << repo_url_push << " (branch: " << branch_name << ")" << std::endl;
            lisa::GitPusher pusher;
            pusher.cloneFolderToRemote(local_dir_push, repo_url_push, branch_name, exclude_patterns);
        } else if (*config_cmd) {
            std::cout << "Compilation configuration from " << config_path << ":" << std::endl;
            auto compiler = config_manager.getCompilerConfig();
            std::cout << "Compiler: " << compiler.type << " (version: " << compiler.version << ")" << std::endl;
            std::cout << "Compiler options: " << compiler.options << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}