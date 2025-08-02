#pragma once
#include "http_server.h"
#include <boost/json.hpp>
#include <filesystem>
#include <boost/system/error_code.hpp>
#include <chrono>
#include "api_handler.h"
#include "file_handler.h"
#include "common_handler.h"
#include <iostream>
#include "log.h"

namespace net = boost::asio;

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;

using namespace std::literals;
namespace fs = std::filesystem;

std::string FromUrlEncoding(const std::string& str);

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    
    explicit RequestHandler(model::Game& game, std::string base_path, Strand& api_strand)
    : game_(game), base_path_(fs::path(base_path)), api_strand_(api_strand) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, std::chrono::time_point<std::chrono::high_resolution_clock> start_time) {
        
        std::string url = FromUrlEncoding(static_cast<std::string>(req.target()));
        
        try {
                    if (url.starts_with("/api/")) {
                        
                        auto handle = [self = shared_from_this(), send,
                                       req = std::forward<decltype(req)>(req), this, url, start_time] {
                            
                            StringResponse res = api_handler::ProcessApi(std::move(req), game_, url, start_time);
                            
                            auto end = std::chrono::high_resolution_clock::now();
                            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time);
                            
                            //log response
                            boost::json::value log_json_resp = {
                                {"response_time", duration.count()},
                                {"code", res.result_int()},
                                {"content_type", std::string(res[http::field::content_type])}
                                };
                            
                            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, log_json_resp) << "response sent";
                            
                            send(res);
                        };
                        
                        net::dispatch(api_strand_, handle);
                        
                    } else {
                        VariantResponse response_var = file_handler::ProcessFile(std::move(req), url, base_path_);
                        
                        if (std::holds_alternative<StringResponse>(response_var)) {
                            
                            StringResponse res = std::get<StringResponse>(std::move(response_var));
                            
                            // logging
                            auto end = std::chrono::high_resolution_clock::now();
                            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time);
                            
                            //log response
                            boost::json::value log_json_resp = {
                                {"response_time", duration.count()},
                                {"code", res.result_int()},
                                {"content_type", std::string(res[http::field::content_type])}
                            };
                            
                            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, log_json_resp) << "response sent";
                            
                            send(res);
                        }
                        else if (std::holds_alternative<FileResponse>(response_var)) {
                            
                            FileResponse res = std::get<FileResponse>(std::move(response_var));
                            
                            // logging
                            auto end = std::chrono::high_resolution_clock::now();
                            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time);
                            
                            boost::json::value log_json_resp = {
                                {"response_time", duration.count()},
                                {"code", res.result_int()},
                                {"content_type", std::string(res[http::field::content_type])}
                            };
                            
                            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, log_json_resp) << "response sent";
                            
                            send(res);
                        }
                    }
            
                } catch (const std::exception& e) {
                    std::cout << "Exception: " << e.what() << std::endl;
                }
    }

private:
    model::Game& game_;
    fs::path base_path_;
    Strand& api_strand_;
    
private:
    std::string GetMimeType(const fs::path& path);
};

}  // namespace http_handler
