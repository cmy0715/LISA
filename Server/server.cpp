#include "server.h"
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace httplib;
using namespace lisa::server;

Server::Server(const Config& config, GitHandler& git_handler, CompilationHandler& compilation_handler)
    : config_(config), git_handler_(git_handler), compilation_handler_(compilation_handler) {}

bool Server::start() {
    return http_server_.listen(config_.host().c_str(), config_.port());
}

void Server::set_routes(Server& server) {
    auto& svr = server.http_server_;
    auto& git_handler = server.git_handler_;
    auto& compilation_handler = server.compilation_handler_;

    // 提交编译任务
    svr.Post("/api/submit", [&](const Request& req, Response& res) {
        handle_submit(req, res, git_handler, compilation_handler);
    });

    // 查询编译状态
    svr.Get("/api/status/(:job_id)", [&](const Request& req, Response& res) {
        handle_status(req, res, compilation_handler);
    });

    // 获取编译结果
    svr.Get("/api/result/(:job_id)", [&](const Request& req, Response& res) {
        handle_result(req, res, compilation_handler);
    });

    // 健康检查
    svr.Get("/health", [](const Request& req, Response& res) {
        res.status = 200;
        res.set_content("OK", "text/plain");
    });
}

void Server::handle_submit(const Request& req, Response& res, GitHandler& git_handler, CompilationHandler& compilation_handler) {
    try {
        if (req.headers.find("Content-Type") == req.headers.end() || 
            req.headers["Content-Type"] != "application/json") {
            res.status = 400;
            res.set_content("Invalid Content-Type. Expected application/json", "text/plain");
            return;
        }

        json req_data = json::parse(req.body);
        std::string repo_url = req_data["repo_url"];
        std::string branch = req_data.value("branch", "main");
        std::string commit_hash = req_data.value("commit_hash", "");

        // 克隆或拉取代码
        std::string repo_path = git_handler.clone_or_pull(repo_url, branch, commit_hash);

        // 创建编译任务
        std::string job_id = compilation_handler.create_job(repo_path, req_data);

        // 返回任务ID
        json response_data = {
            {"status", "success"},
            {"job_id", job_id},
            {"message", "Compilation job created successfully"}
        };

        res.status = 201;
        res.set_content(response_data.dump(), "application/json");
    } catch (const json::exception& e) {
        res.status = 400;
        res.set_content("Invalid JSON format: " + std::string(e.what()), "text/plain");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content("Server error: " + std::string(e.what()), "text/plain");
    }
}

void Server::handle_status(const Request& req, Response& res, CompilationHandler& compilation_handler) {
    try {
        std::string job_id = req.matches[1];
        auto status = compilation_handler.get_job_status(job_id);

        if (!status) {
            res.status = 404;
            res.set_content("Job not found", "text/plain");
            return;
        }

        json response_data = {
            {"job_id", job_id},
            {"status", status->status},
            {"progress", status->progress},
            {"started_at", status->started_at}
        };

        if (status->completed) {
            response_data["completed_at"] = status->completed_at;
        }

        res.status = 200;
        res.set_content(response_data.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content("Server error: " + std::string(e.what()), "text/plain");
    }
}

void Server::handle_result(const Request& req, Response& res, CompilationHandler& compilation_handler) {
    try {
        std::string job_id = req.matches[1];
        auto result = compilation_handler.get_job_result(job_id);

        if (!result) {
            res.status = 404;
            res.set_content("Job not found", "text/plain");
            return;
        }

        if (!result->completed) {
            res.status = 409;
            res.set_content("Job is still in progress", "text/plain");
            return;
        }

        json response_data = {
            {"job_id", job_id},
            {"status", result->status},
            {"exit_code", result->exit_code},
            {"output", result->output},
            {"completed_at", result->completed_at}
        };

        res.status = 200;
        res.set_content(response_data.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content("Server error: " + std::string(e.what()), "text/plain");
    }
}