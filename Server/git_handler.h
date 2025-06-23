#ifndef GIT_HANDLER_H
#define GIT_HANDLER_H

#include <string>
#include <git2.h>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "logger.h"

namespace lisa::server {

class GitHandler {
public:
    GitHandler(const std::string& base_repo_path);
    ~GitHandler();

    // 克隆或拉取仓库
    std::string clone_or_pull(const std::string& repo_url, const std::string& branch = "main", const std::string& commit_hash = "");

    // 检查特定提交是否存在并检出
    bool checkout_commit(const std::string& repo_path, const std::string& commit_hash);

    // 获取仓库的最后修改时间
    time_t get_last_modified_time(const std::string& repo_path);

    // 清理过期的仓库缓存
    void clean_expired_repos(time_t max_age_seconds);

private:
    std::string base_repo_path_;
    std::unordered_map<std::string, time_t> repo_last_used_;
    std::mutex repo_mutex_;

    // 生成仓库的本地存储路径
    std::string generate_repo_path(const std::string& repo_url);

    // 初始化libgit2
    void init_libgit2();

    // 克隆仓库
    std::string clone_repo(const std::string& repo_url, const std::string& branch);

    // 拉取仓库更新
    bool pull_repo(const std::string& repo_path, const std::string& branch);

    // 处理libgit2错误
    void handle_error(int error_code, const std::string& operation);
};

} // namespace lisa::server

#endif // GIT_HANDLER_H