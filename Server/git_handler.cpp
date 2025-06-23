#include "git_handler.h"
#include <sstream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <cstdlib>
#include <git2/clone.h>
#include <git2/pull.h>
#include <git2/checkout.h>
#include <git2/branch.h>
#include <git2/errors.h>

namespace fs = std::filesystem;
using namespace lisa::server;

GitHandler::GitHandler(const std::string& base_repo_path) : base_repo_path_(base_repo_path) {
    init_libgit2();
    // 创建基础仓库目录
    fs::create_directories(base_repo_path_);
}

GitHandler::~GitHandler() {
    git_libgit2_shutdown();
}

void GitHandler::init_libgit2() {
    int error = git_libgit2_init();
    if (error < 0) {
        const git_error* e = giterr_last();
        Logger::error("Failed to initialize libgit2: " + std::string(e ? e->message : "No error message"));
        throw std::runtime_error("libgit2 initialization failed");
    }
}

std::string GitHandler::generate_repo_path(const std::string& repo_url) {
    // 从URL生成唯一目录名（简化实现，实际项目中可使用哈希）
    size_t last_slash = repo_url.find_last_of('/');
    size_t dot_git = repo_url.find(".git");
    std::string repo_name = repo_url.substr(last_slash + 1, dot_git - last_slash - 1);
    return base_repo_path_ + "/" + repo_name;
}

void GitHandler::handle_error(int error_code, const std::string& operation) {
    if (error_code >= 0) return;

    const git_error* e = giterr_last();
    std::string error_msg = operation + " failed: " + (e ? e->message : "Unknown error");
    Logger::error(error_msg);
    throw std::runtime_error(error_msg);
}

std::string GitHandler::clone_repo(const std::string& repo_url, const std::string& branch) {
    git_repository* repo = nullptr;
    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

    clone_opts.checkout_opts = checkout_opts;
    clone_opts.checkout_branch = branch.c_str();

    std::string repo_path = generate_repo_path(repo_url);

    // 如果目录已存在，先删除
    if (fs::exists(repo_path)) {
        fs::remove_all(repo_path);
    }

    int error = git_clone(&repo, repo_url.c_str(), repo_path.c_str(), &clone_opts);
    handle_error(error, "Git clone");

    git_repository_free(repo);
    return repo_path;
}

bool GitHandler::pull_repo(const std::string& repo_path, const std::string& branch) {
    git_repository* repo = nullptr;
    int error = git_repository_open(&repo, repo_path.c_str());
    if (error < 0) return false;

    git_remote* origin = nullptr;
    error = git_remote_lookup(&origin, repo, "origin");
    handle_error(error, "Lookup remote origin");

    // 拉取最新代码
    error = git_remote_fetch(origin, nullptr, nullptr, nullptr);
    handle_error(error, "Fetch from remote");

    // 检出指定分支
    git_reference* ref = nullptr;
    error = git_branch_lookup(&ref, repo, branch.c_str(), GIT_BRANCH_LOCAL);
    handle_error(error, "Lookup branch: " + branch);

    git_commit* commit = nullptr;
    error = git_reference_peel(&commit, ref, GIT_OBJECT_COMMIT);
    handle_error(error, "Peel branch reference");

    git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
    checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE;
    error = git_checkout_tree(repo, reinterpret_cast<git_object*>(commit), &checkout_opts);
    handle_error(error, "Checkout tree");

    error = git_repository_set_head(repo, git_reference_name(ref));
    handle_error(error, "Set head to branch: " + branch);

    git_commit_free(commit);
    git_reference_free(ref);
    git_remote_free(origin);
    git_repository_free(repo);
    return true;
}

std::string GitHandler::clone_or_pull(const std::string& repo_url, const std::string& branch, const std::string& commit_hash) {
    std::lock_guard<std::mutex> lock(repo_mutex_);
    std::string repo_path = generate_repo_path(repo_url);

    try {
        if (fs::exists(repo_path)) {
            Logger::info("Pulling updates for repo: " + repo_url);
            pull_repo(repo_path, branch);
        } else {
            Logger::info("Cloning new repo: " + repo_url);
            repo_path = clone_repo(repo_url, branch);
        }

        // 如果指定了commit hash，检出特定提交
        if (!commit_hash.empty()) {
            checkout_commit(repo_path, commit_hash);
        }

        // 更新最后使用时间
        repo_last_used_[repo_path] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        return repo_path;
    } catch (const std::exception& e) {
        Logger::error("Error processing repo: " + std::string(e.what()));
        // 如果失败且目录存在，尝试删除后重新克隆
        if (fs::exists(repo_path)) {
            fs::remove_all(repo_path);
        }
        return clone_repo(repo_url, branch);
    }
}

bool GitHandler::checkout_commit(const std::string& repo_path, const std::string& commit_hash) {
    git_repository* repo = nullptr;
    int error = git_repository_open(&repo, repo_path.c_str());
    handle_error(error, "Open repository");

    git_object* commit = nullptr;
    error = git_object_lookup(&commit, repo, git_oid_fromstr(commit_hash.c_str()), GIT_OBJECT_COMMIT);
    handle_error(error, "Lookup commit: " + commit_hash);

    git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
    checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE;
    error = git_checkout_tree(repo, commit, &checkout_opts);
    handle_error(error, "Checkout commit: " + commit_hash);

    git_object_free(commit);
    git_repository_free(repo);
    return true;
}

time_t GitHandler::get_last_modified_time(const std::string& repo_path) {
    std::lock_guard<std::mutex> lock(repo_mutex_);
    auto it = repo_last_used_.find(repo_path);
    if (it != repo_last_used_.end()) {
        return it->second;
    }
    return 0;
}

void GitHandler::clean_expired_repos(time_t max_age_seconds) {
    std::lock_guard<std::mutex> lock(repo_mutex_);
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    for (auto it = repo_last_used_.begin(); it != repo_last_used_.end();) {
        if (now - it->second > max_age_seconds) {
            Logger::info("Cleaning expired repo: " + it->first);
            fs::remove_all(it->first);
            it = repo_last_used_.erase(it);
        } else {
            ++it;
        }
    }
}