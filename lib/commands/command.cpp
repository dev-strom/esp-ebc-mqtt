#include "Command.hpp"
#include "EbcController.hpp"
#include "Logger.hpp"


// EBC-A20:
// fa 21 0064 01b4 000a 0af8  C-CV - 4.2V, Current 1.0A, CutOff 0.1A
// fa 21 0064 01aa 000a e4f8  C-CV - 4.1V, Current 1.0A, CutOff 0.1A
// fa 21 0032 01aa 000a b2f8  C-CV - 4.1V, Current 0.5A, CutOff 0.1A
// fa 21 0032 01aa 0014 acf8  C-CV - 4.1V, Current 0.5A, CutOff 0.2A

// fa 01 0023 010a 000f 26f8    D-CC TestVal 0.35A, Cutoff 2.5V, MaxTime 15
// fa 01 0023 010a 0010 39f8    D-CC TestVal 0.35A, Cutoff 2.5V, MaxTime 16
// fa 01 0024 010a 0010 3ef8    D-CC TestVal 0.36A, Cutoff 2.5V, MaxTime 16
// fa 01 0024 0114 0010 20f8    D-CC TestVal 0.36A, Cutoff 2.6V, MaxTime 16

// fa 11 0001 010a 000a 11f8    D-CP TestVal 1.0W, Cutoff 2.5V, MaxTime 10
// fa 11 0001 0114 000a 0ff8    D-CP TestVal 1.0W, Cutoff 2.6V, MaxTime 10
// fa 11 0001 0114 000b 0ef8    D-CP TestVal 1.0W, Cutoff 2.6V, MaxTime 11
// fa 11 000b 0114 000b 04f8    D-CP TestVal  11W, Cutoff 2.6V, MaxTime 11

using namespace std;

Command::Command()
    : Message(10), controller(nullptr)
{
    command = InvalidCommand;
    values.assign(DATA_VALUES_LEN, 0);
}

Command::Command(const void *c, Command_t cmd)
    : Message(10), controller(c)
{
    command = cmd;
    values.assign(DATA_VALUES_LEN, 0);
    CalcCRC();
}

Command::Command(const void *c, Command_t cmd, const vector<Parameter>& parameters)
    : Message(10), controller(c)
{
    command = cmd;
    values.assign(DATA_VALUES_LEN, 0);
    for (auto & p : parameters)
    {
        SetParameter(p);
    }
    CalcCRC();
}


Command::Command(const Command& that)
    : Message(10)
{
    *this = that;
}

Command& Command::operator=(const Command& that)
{
    if (this != &that)
    {
        controller = that.controller;
        command = that.command;
        values = that.values;
        CalcCRC();
    }
    return *this;
}

Command_t Command::GetCommand() const
{
    return static_cast<Command_t> (command);
}

const char* Command::GetCommandStr() const
{
    if (controller == nullptr) {
        return "?";
    } else {
        return ((EbcController*)controller)->CommandToString(command);
    }

}

void Command::FillBytes()
{
    std::vector<uint8_t>& bytes = buffer;

    bytes[0] = startTag;
    bytes[1] = command;
    for (size_t i = 0; i < values.size(); i++) {
        bytes[(2*i)+2] = values[i] >> 8;
        bytes[(2*i)+3] = values[i] & 0xff;
    }
    bytes[8] = crc;
    bytes[9] = endTag;
}

void Command::SetParameter(const Parameter& parameter)
{
    if (parameter.index < 0 || values.size() <= parameter.index) {
        Logger::LogE(String(F("command parameter \"")) + parameter.name + F("\": index out of range"));
        return; // skip this parameter, because it is not valid
    }
    uint16_t& value = this->values[parameter.index];

    if (! ((EbcController*)controller)->Encode(parameter.value, value, parameter.packing)) {
        Logger::LogE(String(F("there is no encoder for parameter ")) + parameter.name + F(" in command message 0x") + String(command, HEX));
    }
}

