#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

enum MessageType : uint8_t {
    DATA,
    ERROR,
    PING
};

enum MessageNode : uint8_t {
    NODE_UNKNOWN,
    NODE_ESP32,
    NODE_GPS_ARDUINO,
    NODE_SENSOR_ARDUINO,
    NODE_TM1638_ARDUINO
};

inline const char *messageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::DATA: return "DATA";
        case MessageType::ERROR: return "ERROR";
        case MessageType::PING: return "PING";
    }
    return "UNKNOWN";
}

inline const char *messageNodeToString(MessageNode node) {
    switch (node) {
        case MessageNode::NODE_ESP32: return "ESP32";
        case MessageNode::NODE_GPS_ARDUINO: return "GPS";
        case MessageNode::NODE_SENSOR_ARDUINO: return "SENS";
        case MessageNode::NODE_TM1638_ARDUINO: return "TM16";
        case MessageNode::NODE_UNKNOWN:
        default:
            return "UNK";
    }
}

inline MessageNode parseMessageNode(const char *rawNode) {
    if (!rawNode || rawNode[0] == '\0') {
        return MessageNode::NODE_UNKNOWN;
    }

    if (strcmp(rawNode, "ESP32") == 0) {
        return MessageNode::NODE_ESP32;
    }
    if (strcmp(rawNode, "GPS") == 0) {
        return MessageNode::NODE_GPS_ARDUINO;
    }
    if (strcmp(rawNode, "SENS") == 0) {
        return MessageNode::NODE_SENSOR_ARDUINO;
    }
    if (strcmp(rawNode, "TM16") == 0) {
        return MessageNode::NODE_TM1638_ARDUINO;
    }

    return MessageNode::NODE_UNKNOWN;
}

struct Message {
    static constexpr size_t MaxPayloadSize = 64;   // tweak as needed

    MessageType type;
    MessageNode source;
    MessageNode destination;
    char payload[MaxPayloadSize];
    uint8_t length;  // how many bytes in payload are actually used

    Message(MessageType t)
    : type(t), source(MessageNode::NODE_UNKNOWN), destination(MessageNode::NODE_UNKNOWN), payload{0}, length(0) {}
};
