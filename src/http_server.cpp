#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

void SessionBase::Run() {
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

SessionBase::SessionBase(tcp::socket&& socket)
    : stream_(std::move(socket)) {
}

void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
    
    if (ec) {
        
        boost::json::value log_json = {{"code", ec.value()}, {"text", ec.message()}, {"where", "write"}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, log_json) << "error";
        
        return;
    }

    if (close) {
        // Семантика ответа требует закрыть соединение
        return Close();
    }

    // Считываем следующий запрос
    Read();
}

void SessionBase::Read() {
    using namespace std::literals;
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    http::async_read(stream_, buffer_, request_,
    // По окончании операции будет вызван метод OnRead
    beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    using namespace std::literals;
    if (ec == http::error::end_of_stream) {
        // Нормальная ситуация - клиент закрыл соединение
        return Close();
    }
    if (ec) {
        
        boost::json::value log_json = {{"code", ec.value()}, {"text", ec.message()}, {"where", "read"}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, log_json) << "error";
        
        return;
    }
    HandleRequest(std::move(request_), stream_.socket().remote_endpoint());
}

void SessionBase::Close() {
    stream_.socket().shutdown(tcp::socket::shutdown_send);
}

}
