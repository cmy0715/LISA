#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <memory>
#include <stdexcept>
#include <git2.h>

namespace fs = std::filesystem;

// 初始化libgit2
void initLibGit2() {
    static bool initialized = false;
    if (!initialized) {
        git_libgit2_init();
        initialized = true;
    }
}

// 检查文件是否存在于Git仓库中
bool fileExistsInRepo(const std::string& file_path, git_repository* repo) {
    git_oid oid;
    git_tree* tree = nullptr;
    git_commit* commit = nullptr;
    bool exists = false;

    initLibGit2();

    // 获取HEAD提交
    if (git_revparse_single((git_object**)&commit, repo, "HEAD") != 0) {
        return false;
    }

    // 获取提交对应的树
    if (git_commit_tree(&tree, commit) != 0) {
        git_commit_free(commit);
        return false;
    }

    // 检查文件是否存在于树中
    git_tree_entry* entry = git_tree_entry_bypath(tree, file_path.c_str());
    if (entry != nullptr) {
        exists = true;
        git_tree_entry_free(entry);
    }

    git_tree_free(tree);
    git_commit_free(commit);
    return exists;
}

// 比较本地文件与Git仓库中的文件差异
bool hasFileDifference(const std::string& file_path, git_repository* repo, const std::string& local_file_path) {
    initLibGit2();

    git_commit* commit = nullptr;
    git_tree* tree = nullptr;
    git_tree_entry* entry = nullptr;
    git_blob* blob = nullptr;
    bool different = true;

    try {
        // 获取HEAD提交
        if (git_revparse_single((git_object**)&commit, repo, "HEAD") != 0) {
            throw std::runtime_error("无法获取HEAD提交");
        }

        // 获取提交树
        if (git_commit_tree(&tree, commit) != 0) {
            throw std::runtime_error("无法获取提交树");
        }

        // 查找文件条目
        entry = git_tree_entry_bypath(tree, file_path.c_str());
        if (!entry) {
            throw std::runtime_error("文件不存在于仓库中");
        }

        // 获取文件blob
        if (git_blob_lookup(&blob, repo, git_tree_entry_id(entry)) != 0) {
            throw std::runtime_error("无法获取文件blob");
        }

        // 读取本地文件
        std::ifstream local_file(local_file_path, std::ios::binary | std::ios::ate);
        if (!local_file.is_open()) {
            throw std::runtime_error("无法打开本地文件");
        }

        std::streamsize local_size = local_file.tellg();
        local_file.seekg(0, std::ios::beg);
        std::vector<char> local_data(local_size);
        local_file.read(local_data.data(), local_size);

        // 获取仓库文件数据
        const char* repo_data = (const char*)git_blob_rawcontent(blob);
        size_t repo_size = git_blob_rawsize(blob);

        // 比较内容
        different = (local_size != repo_size) || 
                   (memcmp(local_data.data(), repo_data, local_size) != 0);
    } catch (const std::exception& e) {
        std::cerr << "比较文件差异时出错: " << e.what() << std::endl;
    }

    // 清理资源
    git_blob_free(blob);
    git_tree_entry_free(entry);
    git_tree_free(tree);
    git_commit_free(commit);

    return different;
}

// 递归获取Git仓库中的所有文件路径
void getRepoFilePaths(git_repository* repo, git_tree* tree, const std::string& base_path, std::vector<std::string>& file_paths) {
    size_t entry_count = git_tree_entrycount(tree);
    for (size_t i = 0; i < entry_count; ++i) {
        const git_tree_entry* entry = git_tree_entry_byindex(tree, i);
        std::string entry_name = git_tree_entry_name(entry);
        std::string full_path = base_path.empty() ? entry_name : base_path + "/" + entry_name;

        if (git_tree_entry_type(entry) == GIT_OBJ_TREE) {
            git_tree* subtree = nullptr;
            if (git_tree_entry_to_object((git_object**)&subtree, repo, entry) == 0) {
                getRepoFilePaths(repo, subtree, full_path, file_paths);
                git_tree_free(subtree);
            }
        } else if (git_tree_entry_type(entry) == GIT_OBJ_BLOB) {
            file_paths.push_back(full_path);
        }
    }
}

// 主函数：比较本地文件夹与Git仓库差异
std::vector<std::string> compareLocalWithRepo(const std::string& file_address, const std::string& hub_address) {
    std::vector<std::string> different_files;
    git_repository* repo = nullptr;
    git_commit* commit = nullptr;
    git_tree* tree = nullptr;

    initLibGit2();

    try {
        // 确保文件夹存在
        if (!fs::exists(file_address)) {
            throw std::runtime_error("Local folder does not exist: " + file_address);
        }

        // 克隆远程仓库
        if (git_clone(&repo, hub_address.c_str(), "/tmp/lisa_git_repo", nullptr) != 0) {
            throw std::runtime_error("Failed to clone repository: " + hub_address);
        }

        // 获取HEAD提交
        if (git_revparse_single((git_object**)&commit, repo, "HEAD") != 0) {
            throw std::runtime_error("Failed to get HEAD commit");
        }

        // 获取提交树
        if (git_commit_tree(&tree, commit) != 0) {
            throw std::runtime_error("Failed to get commit tree");
        }

        // 遍历本地文件夹中的所有文件
        for (const auto& entry : fs::recursive_directory_iterator(file_address)) {
            if (fs::is_regular_file(entry.path())) {
                std::string rel_path = fs::relative(entry.path(), file_address).string();
                
                // 检查文件是否存在于仓库中
                if (fileExistsInRepo(rel_path, repo)) {
                    // 比较文件内容
                    if (hasFileDifference(rel_path, repo, entry.path().string())) {
                        different_files.push_back("Different: " + rel_path);
                    }
                } else {
                    different_files.push_back("New: " + rel_path);
                }
            }
        }

        // 获取仓库中所有文件路径
        std::vector<std::string> repo_files;
        getRepoFilePaths(repo, tree, "", repo_files);

        // 检查仓库中存在但本地不存在的文件
        for (const auto& repo_file : repo_files) {
            fs::path local_path = file_address + "/" + repo_file;
            if (!fs::exists(local_path)) {
                different_files.push_back("Missing: " + repo_file);
            }
        }

    } catch (const std::exception& e) {
        different_files.push_back("Error: " + std::string(e.what()));
    }

    // 清理资源
    git_tree_free(tree);
    git_commit_free(commit);
    git_repository_free(repo);
    std::system("rm -rf /tmp/lisa_git_repo");

    return different_files;
}

/*int main() {
    std::string local_folder = "/path/to/local/folder";
    std::string git_repo = "https://github.com/username/repo.git";
    
    std::vector<std::string> diffs = compareLocalWithRepo(local_folder, git_repo);
    
    std::cout << "Found " << diffs.size() << " differences:" << std::endl;
    for (const auto& diff : diffs) {
        std::cout << diff << std::endl;
    }
    
    return 0;
}*/
