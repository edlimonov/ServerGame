#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/beast/http.hpp>
#include <string_view>
#include <variant>

namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;

using namespace std::literals;

using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using VariantResponse = std::variant<StringResponse, FileResponse>;

struct Response {
    int code;
    std::string content_type;
};

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
};

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML, bool cache = false);

FileResponse MakeFileResponse(http::status status, http::file_body::value_type&& body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML);

