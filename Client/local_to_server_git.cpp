#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <regex>
#include <git2.h>
#include <fstream>

namespace fs = std::filesystem;

// 初始化libgit2
void initLibGit2() {
    static bool initialized = false;
    if (!initialized) {
        git_libgit2_init();
        initialized = true;
    }
}

// 检查文件是否匹配排除模式
bool isExcluded(const std::string& file_path, const std::vector<std::regex>& exclude_patterns) {
    for (const auto& pattern : exclude_patterns) {
        if (std::regex_search(file_path, pattern)) {
            return true;
        }
    }
    return false;
}

// 添加文件到Git索引
int addFilesToIndex(git_index* index, const std::string& source_path, 
                   const std::vector<std::regex>& exclude_patterns) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(source_path)) {
            if (fs::is_regular_file(entry.path())) {
                std::string rel_path = fs::relative(entry.path(), source_path).string();
                
                // 检查是否需要排除
                if (isExcluded(rel_path, exclude_patterns)) {
                    continue;
                }
                
                // 添加文件到索引
                if (git_index_add_bypath(index, rel_path.c_str()) != 0) {
                    std::cerr << "错误: 无法添加文件到索引 - " << rel_path << std::endl;
                    return -1;
                }
            }
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "添加文件到索引时出错: " << e.what() << std::endl;
        return -1;
    }
}

/**
 * 将本地文件夹推送到远程Git仓库并创建新分支
 * @param source_path 本地文件夹地址
 * @param hub_address 远程仓库地址
 * @param branch_name 新分支名称，默认为当前时间戳
 * @param exclude_patterns 忽略的文件/目录模式列表（正则表达式）
 * @return 操作是否成功
 */
bool cloneFolderToRemote(const std::string& source_path, const std::string& hub_address, 
                        const std::string& branch_name = "",
                        const std::vector<std::string>& exclude_patterns = {}) {
    initLibGit2();
    git_repository* repo = nullptr;
    git_index* index = nullptr;
    git_oid tree_oid, commit_oid;
    git_tree* tree = nullptr;
    git_reference* head = nullptr;
    git_signature* sig = nullptr;
    std::string temp_dir;
    bool success = false;

    // 编译排除模式正则表达式
    std::vector<std::regex> exclude_regexes;
    for (const auto& pattern : exclude_patterns) {
        exclude_regexes.emplace_back(pattern, std::regex::ECMAScript | std::regex::icase);
    }

    try {
        // 检查源文件夹是否存在
        if (!fs::exists(source_path) || !fs::is_directory(source_path)) {
            throw std::runtime_error("源文件夹不存在或不是有效目录 - " + source_path);
        }

        // 生成默认分支名
        std::string new_branch = branch_name.empty() 
            ? "branch_" + std::to_string(time(nullptr)) 
            : branch_name;

        // 创建临时目录
        temp_dir = "/tmp/lisa_git_repo_" + std::to_string(time(nullptr));
        fs::create_directories(temp_dir);

        // 初始化Git仓库
        if (git_repository_init(&repo, temp_dir.c_str(), false) != 0) {
            throw std::runtime_error("无法初始化Git仓库");
        }

        // 获取索引
        if (git_repository_index(&index, repo) != 0) {
            throw std::runtime_error("无法获取Git索引");
        }

        // 添加文件到索引
        if (addFilesToIndex(index, source_path, exclude_regexes) != 0) {
            throw std::runtime_error("添加文件到索引失败");
        }

        // 写入索引到磁盘
        if (git_index_write(index) != 0) {
            throw std::runtime_error("无法写入索引");
        }

        // 创建树对象
        if (git_index_write_tree(&tree_oid, index) != 0) {
            throw std::runtime_error("无法创建树对象");
        }

        // 查找树对象
        if (git_tree_lookup(&tree, repo, &tree_oid) != 0) {
            throw std::runtime_error("无法查找树对象");
        }

        // 创建提交签名
        if (git_signature_default(&sig, repo) != 0) {
            throw std::runtime_error("无法创建提交签名");
        }

        // 创建初始提交
        std::string commit_msg = "Add folder: " + fs::path(source_path).filename().string();
        if (git_commit_create_v(&commit_oid, repo, "HEAD", sig, sig, 
                                nullptr, commit_msg.c_str(), tree, 0) != 0) {
            throw std::runtime_error("无法创建提交");
        }

        // 创建新分支
        if (git_branch_create(&head, repo, new_branch.c_str(), 
                             (git_object*)git_commit_lookup(repo, &commit_oid), false) != 0) {
            throw std::runtime_error("无法创建新分支");
        }

        // 设置远程仓库
        git_remote* remote = nullptr;
        if (git_remote_create(&remote, repo, "origin", hub_address.c_str()) != 0) {
            throw std::runtime_error("无法设置远程仓库");
        }

        // 推送分支
        git_push_options push_opts = GIT_PUSH_OPTIONS_INIT;
        git_strarray refspecs = {const_cast<char**>(&new_branch), 1};
        if (git_push(remote, &refspecs, &push_opts) != 0) {
            throw std::runtime_error("推送分支失败");
        }

        std::cout << "成功将文件夹 " << source_path << " 推送到远程仓库 " << hub_address 
                  << " 并创建新分支: " << new_branch << std::endl;
        success = true;

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
    }

    // 清理资源
    git_signature_free(sig);
    git_tree_free(tree);
    git_index_free(index);
    git_repository_free(repo);
    if (!temp_dir.empty()) {
        fs::remove_all(temp_dir);
    }

    return success;
}

/*int main() {
    // 示例用法
    std::string local_folder = "/path/to/your/local/folder";
    std::string remote_repo = "https://github.com/yourusername/your-repo.git";
    std::string custom_branch = "feature/new-folder";
    
    // 使用自定义分支名
    if (cloneFolderToRemote(local_folder, remote_repo, custom_branch)) {
        std::cout << "操作成功完成，分支: " << custom_branch << std::endl;
    } else {
        std::cout << "操作失败" << std::endl;
    }
    
    // 使用默认分支名（时间戳）
    if (cloneFolderToRemote(local_folder, remote_repo)) {
        std::cout << "操作成功完成，使用默认分支名" << std::endl;
    } else {
        std::cout << "操作失败" << std::endl;
    }
    
    // 排除特定文件/目录
    std::vector<std::string> exclude = {"*.tmp", "node_modules", ".DS_Store"};
    if (cloneFolderToRemote(local_folder, remote_repo, "clean-upload", exclude)) {
        std::cout << "操作成功完成，已排除临时文件" << std::endl;
    }
    
    return 0;
}*/
