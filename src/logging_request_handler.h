#pragma once

#include "request_handler.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace net = boost::asio;
using tcp = net::ip::tcp;

template<class SomeRequestHandler>
class LoggingRequestHandler {
public:

    explicit LoggingRequestHandler(SomeRequestHandler& handler) : decorated_{handler} {}
    
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, const tcp::endpoint& remote_endpoint, Send&& send) {
        
        // log request
        boost::json::value log_json_req = {{"ip", remote_endpoint.address().to_string()}, {"URI", static_cast<std::string>(req.target())}, {"method", req.method_string()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, log_json_req) << "request received";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        decorated_(std::move(req), std::move(send), start);
    }
    
private:
     SomeRequestHandler& decorated_;
};
