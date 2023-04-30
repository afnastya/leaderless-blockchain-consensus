#pragma once

#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// enum MessageType {
//     Transaction,
//     Error,
//     EST, // change to DBFT::EST
//     AUX, // change to DBFT::AUX
//     // types needed for reaching consensus
// };

class Message {
public:
    std::string type;
    int from, to; // node ids
    json data;

    Message() {
    }
    Message(std::string _type) : type(_type) {
    }
    Message(std::string _type, json _data) : type(_type), data(_data) {
    }

    // if _to == -1, then broadcast
    Message(std::string _type, json _data, int _from, int _to = -1)
        : type(_type), from(_from), to(_to), data(_data) {
    }

    bool operator==(const Message& other) const {
        return type == other.type && to == other.to && from == other.from 
               && data == other.data;
    }

    friend std::ostream& operator<<(std::ostream& out, Message msg) {
        out << "{" << msg.type << ", " << msg.data << "}";
        return out;
    }
};