// Protocol implementation
#include "tradex/net/net.h"
#include <sstream>
#include <iostream>

namespace tradex {
namespace net {

Message Protocol::createSubmitOrder(int64_t request_id,
    const std::string& symbol, int64_t qty, double price,
    const std::string& side, const std::string& type) {

    std::ostringstream oss;
    oss << "{"
        << "\"request_id\":" << request_id << ","
        << "\"symbol\":\"" << symbol << "\","
        << "\"qty\":" << qty << ","
        << "\"price\":" << price << ","
        << "\"side\":\"" << side << "\","
        << "\"type\":\"" << type << "\""
        << "}";

    Message msg;
    msg.header.magic = MsgHeader::MAGIC;
    msg.body = oss.str();
    msg.header.length = msg.body.size();
    msg.header.type = static_cast<uint16_t>(MsgType::SubmitOrder);
    msg.header.sequence = 0;
    msg.header.timestamp = 0;

    return msg;
}

Message Protocol::createCancelOrder(int64_t request_id, int64_t order_id) {
    std::ostringstream oss;
    oss << "{\"request_id\":" << request_id << ",\"order_id\":" << order_id << "}";

    Message msg;
    msg.header.magic = MsgHeader::MAGIC;
    msg.body = oss.str();
    msg.header.length = msg.body.size();
    msg.header.type = static_cast<uint16_t>(MsgType::CancelOrder);
    msg.header.sequence = 0;
    msg.header.timestamp = 0;

    return msg;
}

Message Protocol::createQueryOrders(int64_t request_id) {
    std::ostringstream oss;
    oss << "{\"request_id\":" << request_id << "}";

    Message msg;
    msg.header.magic = MsgHeader::MAGIC;
    msg.body = oss.str();
    msg.header.length = msg.body.size();
    msg.header.type = static_cast<uint16_t>(MsgType::QueryOrders);
    msg.header.sequence = 0;
    msg.header.timestamp = 0;

    return msg;
}

Message Protocol::createQueryPositions(int64_t request_id, int64_t account_id) {
    std::ostringstream oss;
    oss << "{\"request_id\":" << request_id << ",\"account_id\":" << account_id << "}";

    Message msg;
    msg.header.magic = MsgHeader::MAGIC;
    msg.body = oss.str();
    msg.header.length = msg.body.size();
    msg.header.type = static_cast<uint16_t>(MsgType::QueryPositions);
    msg.header.sequence = 0;
    msg.header.timestamp = 0;

    return msg;
}

Message Protocol::createPing(int64_t request_id) {
    std::ostringstream oss;
    oss << "{\"request_id\":" << request_id << "}";

    Message msg;
    msg.header.magic = MsgHeader::MAGIC;
    msg.body = oss.str();
    msg.header.length = msg.body.size();
    msg.header.type = static_cast<uint16_t>(MsgType::Ping);
    msg.header.sequence = 0;
    msg.header.timestamp = 0;

    return msg;
}

bool Protocol::parseOrderResponse(const Message& msg, std::string& order_id,
    bool& success, std::string& message) {

    // Simple parsing - just check success field
    success = (msg.body.find("\"success\":true") != std::string::npos);
    return true;
}

bool Protocol::parsePositionsResponse(const Message& msg, std::string& positions_json) {
    positions_json = msg.body;
    return true;
}

} // namespace net
} // namespace tradex