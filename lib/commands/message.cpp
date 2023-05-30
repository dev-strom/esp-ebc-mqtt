#include "Message.hpp"
#include "Logger.hpp"

Message::Message(size_t length)
{
    buffer.assign(length, 0x00);
    startTag = 0xfa;
    crc = 0x00;
    endTag = 0xf8;

    failCounter = 0;
    crcCheckDisabled = false;
}

// fa 0a 0023 101c 0000 0000 0023 018c 0000 09 82 f8

bool Message::CheckCRC()
{
    if (crcCheckDisabled)
        return true;

    uint8_t cs = 0;
    for (size_t i = 1; i < (buffer.size()-2); i++) {
        cs = cs ^ buffer[i];
    }
    bool isValid = (buffer[buffer.size()-2] == cs);
    if (!isValid) {
        // sometimes the EBC-A20 sends a slightly different checksum. we do accept it also!
        isValid = (((cs & 0xf0) == 0xf0) && ((cs & 0x0f) == buffer[buffer.size()-2]));
    }
    if (!isValid) {
        Logger::LogE(String(F("crc check failed: 0x")) + String(cs, HEX) + F(" != 0x") + String(buffer[buffer.size()-2], HEX));
        failCounter++;
    } else {
        failCounter = 0;
    }
    if (5 < failCounter) {
        crcCheckDisabled = true;
        Logger::LogE(F("crc check disabled!"));
        return true;
    }
    return isValid;
}

void Message::CalcCRC()
{
    FillBytes();
    uint8_t cs = 0;
    for (size_t i = 1; i < buffer.size()-2; i++) {
        cs = cs ^ buffer[i];
    }
    crc = cs;
    buffer[buffer.size()-2] = cs;
}

// fa0500000000000005f8
//                   fa    05    00    00    00    00    00    00    05    f8
//uint8_t myBuf[] = {0xfa, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0xf8};

bool Message::Send(Stream& stream) const
{
    size_t num = stream.write(buffer.data(), buffer.size());

    if (num < buffer.size()) {
        Logger::LogE("not enough data written");
        return false;
    }
    return true;
}

bool Message::Read(Stream& stream)
{
    if (stream.available() < 1) {
        return false;
    }

    // skip all bytes that are not 0xfa
    while (stream.peek() != startTag) {
        stream.read();
    }

    size_t num = stream.readBytes(buffer.data(), buffer.size());
    if (num < buffer.size()) {
        Logger::LogE(F("not enough data read"));
        return false;
    }

    if (CheckCRC()) {
        FillFromBytes();
        return true;
    }

    Logger::LogE(String(F("crc check fails on read data: ")) + ToHexString());
    return false;
}


String Message::ToHexString() const
{
    String s = "";
    char str[4];
    for (const auto& b : buffer) {
        sprintf(str, "%02x", b);
        s += str;
    }
    return s;
}
