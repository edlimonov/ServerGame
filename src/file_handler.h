#pragma once

#include <boost/beast/http.hpp>          // Основной заголовок для HTTP
#include <boost/beast/core.hpp>          // Для file_body и других базовых функций
#include <boost/beast/version.hpp>       // Для работы с версией HTTP

#include <boost/json.hpp>                // Для работы с JSON (boost::json::object)
#include <boost/filesystem.hpp>          // Для работы с файловой системой (fs::path)
#include <boost/system/error_code.hpp>   // Для sys::error_code

#include <string>                       // Для std::string
#include <string_view>                   // Для std::string_view
#include <sstream>                       // Для std::ostringstream
#include <utility>                       // Для std::move
#include <filesystem>
#include "common_handler.h"

namespace file_handler {

namespace fs = std::filesystem;

std::string GetMimeType(const fs::path& path);
bool IsSubPath(fs::path path, fs::path base);

template <typename Request>
VariantResponse ProcessFile(Request&& req, std::string& url, const fs::path& base_path_) {
    
    const auto error_response = [&req](http::status status, std::string code, std::string message, std::string_view content_type) {
        
        boost::json::object res;
        
        res["code"] = code;
        res["message"] = message;
        
        std::string text = boost::json::serialize(res);
        
        return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type);
    };
    
    const auto text_response = [&req](http::status status, std::string_view text, std::string_view content_type) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type);
    };
    
    const auto file_response = [&req](http::status status, http::file_body::value_type&& body, std::string_view content_type) {
        return MakeFileResponse(status, std::move(body), req.version(), req.keep_alive(), content_type);
    };
    
    fs::path rel_path(url.substr(1));
    fs::path full_path = base_path_ / rel_path;
    
    if (!IsSubPath(full_path, base_path_)) {
        
        std::ostringstream out;
        
        out << "Full path: " << full_path.string() << std::endl;
        out << "Base path: " << base_path_.string() << std::endl;
        out << "Relative path: " << rel_path.string() << std::endl;
        
        return error_response(http::status::bad_request, "badRequest", out.str(), ContentType::TEXT_PLAIN);
    }
    
    full_path = fs::weakly_canonical(full_path);
    
    if (fs::is_directory(full_path)) {
        full_path /= "index.html";
    }
    
    if (!fs::exists(full_path)) {
        return error_response(http::status::not_found, "Not found", "File not found", ContentType::TEXT_PLAIN);
    }
    
    http::file_body::value_type file;
    sys::error_code ec;
    file.open(full_path.c_str(), beast::file_mode::read, ec);
    
    if (ec) {
        return error_response(http::status::not_found, "Not found", "File not found", ContentType::TEXT_PLAIN);
    }
    
    return file_response(http::status::ok, std::move(file), GetMimeType(full_path));
}

} // namespace file_handler
