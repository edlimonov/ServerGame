#include "log.h"

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {

    boost::json::object result;
    auto ts = *rec[timestamp];

    result["timestamp"] = to_iso_extended_string(ts);
    
    if (auto data = rec[additional_data]) {
        result["data"] = *data;
    }

    result["message"] = rec[logging::expressions::smessage]->c_str();
    
    strm << boost::json::serialize(result);
}

void InitBoostLogFilter() {
    logging::add_common_attributes();

    logging::add_console_log(
        std::cout,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
    );
}

