#pragma once

#include <stdint.h>
#include <stddef.h>

enum MessageType : uint8_t {
    DATA,
    ERROR,
    PING
};

struct Message {
    static constexpr size_t MaxPayloadSize = 64;   // tweak as needed

    MessageType type;
    char payload[MaxPayloadSize];
    uint8_t length;  // how many bytes in payload are actually used

    Message(MessageType t)
    : type(t), payload{0}, length(0) {}
};