#include "common_handler.h"

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type, bool cache) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    
    if (cache) {
        response.set(http::field::cache_control, "no-cache");
    }
    
    return response;
}

FileResponse MakeFileResponse(http::status status, http::file_body::value_type&& body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type) {
    FileResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.content_length(body.size());
    response.body() = std::move(body);
    response.keep_alive(keep_alive);
    return response;
}

