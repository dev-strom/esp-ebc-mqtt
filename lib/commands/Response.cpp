#include "Response.hpp"


Response::Response()
    : Message(19)
{
    mode = 0x00;
    values.assign(DATA_VALUES_LEN, 0);
    id = 0x00;
}

void Response::FillFromBytes()
{
    std::vector<uint8_t>& bytes = buffer;

    startTag              = bytes[0];
    mode                  = bytes[1];
    for (size_t i = 0; i < values.size(); i++) {
        values[i]  = (bytes[(2*i)+2] << 8) + bytes[(2*i)+3];
    }
    id                    = bytes[16];
    crc                   = bytes[17];
    endTag                = bytes[18];
}

std::vector<uint16_t> Response::GetData() const
{
    return values;
}

uint8_t Response::GetId() const
{
    return id;
}
