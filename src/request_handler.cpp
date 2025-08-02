#include "request_handler.h"
#include <cstdint>  // Для платформонезависимых типов

namespace http_handler {

std::string FromUrlEncoding(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.size(); ++i) {
        const char c = str[i];
        
        if (c == '+') {
            result += ' ';
        }
        else if (c != '%') {
            result += c;
        }
        else {
            // Проверяем, что осталось достаточно символов для %XX
            if (i + 2 >= str.size()) {
                result += c;
                continue;
            }
            
            const char h1 = str[i+1];
            const char h2 = str[i+2];
            
            if (!std::isxdigit(h1) || !std::isxdigit(h2)) {
                result += c;
                continue;
            }
            
            // Конвертируем два hex-символа в число
            char hex[3] = {h1, h2, '\0'};
            char* end = nullptr;
            const auto code = static_cast<uint8_t>(std::strtoul(hex, &end, 16));
            
            if (end != hex + 2) {
                result += c;
                continue;
            }
            
            result += static_cast<char>(code);
            i += 2; // Пропускаем два обработанных символа
        }
    }
    
    return result;
}

} // namespace http_handler
