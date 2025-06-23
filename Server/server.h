#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <memory>
#include <httplib.h>
#include "config.h"
#include "git_handler.h"
#include "compilation_handler.h"
#include "logger.h"

namespace lisa::server {

class Server {
public:
    Server(const Config& config, GitHandler& git_handler, CompilationHandler& compilation_handler);
    ~Server() = default;

    // 禁止拷贝构造和赋值
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // 启动服务器
    bool start();

    // 设置HTTP路由
    static void set_routes(httplib::Server& http_server, GitHandler& git_handler, CompilationHandler& compilation_handler);

private:
    Config config_;
    httplib::Server http_server_;
    GitHandler& git_handler_;
    CompilationHandler& compilation_handler_;

    // 处理代码提交请求
    static void handle_submit(httplib::Server& svr, const httplib::Request& req, httplib::Response& res, GitHandler& git_handler);

    // 处理编译状态查询请求
    static void handle_status(httplib::Server& svr, const httplib::Request& req, httplib::Response& res, CompilationHandler& compilation_handler);

    // 处理编译结果获取请求
    static void handle_result(httplib::Server& svr, const httplib::Request& req, httplib::Response& res, CompilationHandler& compilation_handler);

    // 处理健康检查请求
    static void handle_health(httplib::Server& svr, const httplib::Request& req, httplib::Response& res);
};

} // namespace lisa::server

#endif // SERVER_H