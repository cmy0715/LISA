#ifndef LOCAL_TO_SERVER_GIT_H
#define LOCAL_TO_SERVER_GIT_H

#include <string>
#include <vector>
#include <git2.h>

namespace lisa {

class GitPusher {
public:
    GitPusher();
    ~GitPusher();

    /**
     * 将本地文件夹克隆到远程Git仓库并创建新分支
     * @param local_folder 本地文件夹路径
     * @param repo_url 远程仓库URL
     * @param branch_name 要创建的分支名称
     * @param exclude_patterns 要排除的文件/目录模式列表
     * @throws std::runtime_error 如果操作失败
     */
    void cloneFolderToRemote(
        const std::string& local_folder,
        const std::string& repo_url,
        const std::string& branch_name,
        const std::vector<std::string>& exclude_patterns = {}
    );

private:
    git_repository* repo_ = nullptr;
    git_remote* remote_ = nullptr;

    // 初始化libgit2库
    void initLibGit2();

    // 检查文件是否匹配排除模式
    bool isExcluded(const std::string& file_path, const std::vector<std::string>& exclude_patterns);

    // 递归添加文件到Git索引
    void addFilesToIndex(const std::string& base_dir, const std::string& current_dir, git_index* index, const std::vector<std::string>& exclude_patterns);
};

} // namespace lisa

#endif // LOCAL_TO_SERVER_GIT_H