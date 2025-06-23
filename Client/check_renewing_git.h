#ifndef CHECK_RENEWING_GIT_H
#define CHECK_RENEWING_GIT_H

#include <string>
#include <vector>
#include <git2.h>

namespace lisa {

class GitChecker {
public:
    GitChecker();
    ~GitChecker();

    /**
     * 比较本地文件夹与远程Git仓库的差异
     * @param local_dir 本地文件夹路径
     * @param repo_url 远程仓库URL
     * @return 差异文件列表，格式为"状态 文件路径"（A:添加, M:修改, D:删除）
     */
    std::vector<std::string> compareLocalWithRepo(const std::string& local_dir, const std::string& repo_url);

private:
    git_repository* repo_ = nullptr;
    git_commit* head_commit_ = nullptr;
    git_tree* commit_tree_ = nullptr;

    // 初始化libgit2库
    void initLibGit2();

    // 递归获取Git仓库中的所有文件路径
    void getRepoFilePaths(git_tree* tree, const std::string& parent_path, std::vector<std::string>& file_paths);

    // 检查文件是否存在于Git仓库
    bool fileExistsInRepo(const std::string& file_path);

    // 比较本地文件与仓库中文件的内容差异
    bool hasFileDifference(const std::string& local_file_path, const std::string& repo_file_path);
};

} // namespace lisa

#endif // CHECK_RENEWING_GIT_H