#ifndef COMPILATION_CONFIG_H
#define COMPILATION_CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace lisa {

/**
 * 编译配置结构
 * 对应.lisa.yaml配置文件格式
 */
struct CompilerConfig {
    std::string type;          // 编译器类型，如gcc、clang
    std::string version;       // 编译器版本
    std::vector<std::string> options; // 编译选项
};

struct BuildConfig {
    std::string command;       // 构建命令
    std::string working_dir;   // 工作目录
};

struct EnvironmentConfig {
    std::map<std::string, std::string> variables; // 环境变量键值对
};

struct CompilationConfig {
    CompilerConfig compiler;   // 编译器配置
    BuildConfig build;         // 构建配置
    EnvironmentConfig env;     // 环境配置
};

/**
 * 编译配置管理器
 * 负责加载和解析.lisa.yaml配置文件
 */
class CompilationConfigManager {
private:
    CompilationConfig config_;  // 配置数据
    std::string config_path_;   // 配置文件路径

    /**
     * 解析YAML配置文件
     * @param file_path YAML文件路径
     * @return 是否解析成功
     */
    bool parseYaml(const std::string& file_path);

public:
    /**
     * 构造函数
     * @param config_path 配置文件路径，默认为当前目录下的.lisa.yaml
     */
    explicit CompilationConfigManager(const std::string& config_path = ".lisa.yaml");

    /**
     * 加载配置文件
     * @return 是否加载成功
     */
    bool loadConfig();

    /**
     * 获取编译器配置
     * @return 编译器配置
     */
    const CompilerConfig& getCompilerConfig() const { return config_.compiler; }

    /**
     * 获取构建配置
     * @return 构建配置
     */
    const BuildConfig& getBuildConfig() const { return config_.build; }

    /**
     * 获取环境配置
     * @return 环境配置
     */
    const EnvironmentConfig& getEnvironmentConfig() const { return config_.env; }

    /**
     * 检查配置是否有效
     * @return 配置是否有效
     */
    bool isValid() const;
};

} // namespace lisa

#endif // COMPILATION_CONFIG_H