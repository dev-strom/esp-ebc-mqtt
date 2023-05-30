
#include <ArduinoJson.h>
#include <vector>

#include "EbcController.hpp"
#include "EbcA20.hpp"
#include "EbcUnknown.hpp"
#include "Logger.hpp"

using namespace std;

// static EbcA05 ebcA05;
// static EbcA10H ebcA10H;
static EbcA20     ebcA20;
static EbcUnknown ebcUnkown;

enum Commands {
    Cmd_Connect    = 0x05, // b0000 0101
    Cmd_Disconnect = 0x06, // b0000 0110
    Cmd_Stop       = 0x02, // b0000 0010
};

EbcController::EbcController()
{
    commands.push_back(CommandDescript(Cmd_Connect, vector<Mode_t> {}));
    commands.push_back(CommandDescript(Cmd_Disconnect, vector<Mode_t> {}));
}

EbcController& EbcController::GetController()
{
    return ebcUnkown;
}

EbcController& EbcController::GetController(uint8_t id)
{
    switch (id) {
        // case 0x05: return ebcA05;
        // case 0x06: return ebcA10H;
        case 0x09:
            return ebcA20;
        default:
            return ebcUnkown;
    }
}

EbcController& EbcController::GetController(const Response& response)
{
    EbcController& controller = GetController(response.GetId());
    controller.SetData(response.GetData(), response.mode);
    return controller;
}

void EbcController::SetData(const std::vector<uint16_t>& data, Mode_t mode)
{
    value = data;
    this->mode = mode;
}


bool EbcController::IsValidResponseForCommand(Command_t cmd) const
{
    cmd = FindCommand(cmd);
    for (auto & c : commands)
    {
        if (c.command == cmd)
        {
            for (auto & r : c.responses)
            {
                if (r == mode) {
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

bool EbcController::IsActiveResponseForCommand(Command_t cmd) const
{
    for (auto & c : commands)
    {
        if (c.command == cmd)
        {
            return (c.active != Response::InvalidMode) && (c.active == mode);
        }
    }
    return false;
}

bool EbcController::IsFinishedResponseForCommand(Command_t cmd) const
{
    for (auto & c : commands)
    {
        if (c.command == cmd)
        {
            return (c.finish != Response::InvalidMode) && (c.finish == mode);
        }
    }
    return false;
}

bool EbcController::IsStoppedResponseForCommand(Command_t cmd) const
{
    for (auto & c : commands)
    {
        if (c.command == cmd)
        {
            return (c.stopped != Response::InvalidMode) && (c.stopped == mode);
        }
    }
    return false;
}

bool EbcController::IsValidData() const
{
    for (auto & c : commands)
    {
        for (auto & r : c.responses)
        {
            if (r == mode) {
                return true;
            }
        }
        return false;
    }
    return false;
}

Command_t EbcController::FindCommand(Command_t cmd) const
{
    if (cmd != Command::InvalidCommand) {
        return cmd;
    }
    for (auto & c : commands)
    {
        for (auto & r : c.responses)
        {
            if (r == mode) {
                return c.command;
            }
        }
    }
    return cmd;
}


std::vector<Command_t> EbcController::GetCommands() const
{
    std::vector<Command_t> cmds;
    for (auto & c : commands)
    {
        cmds.push_back(c.command);
    }
    return cmds;
}

std::vector<uint8_t> EbcController::GetResponses() const
{
    std::vector<uint8_t> infos;
    for (auto & c : commands)
    {
        for (auto & r : c.responses)
        {
            infos.push_back(r);
        }
    }
    return infos;
}


Command EbcController::CreateConnect() const
{
    return Command(this, Cmd_Connect);
}

Command EbcController::CreateDisconnect() const
{
    return Command(this, Cmd_Disconnect);
}

Command EbcController::CreateStop() const
{
    return Command(this, Cmd_Stop);
}

Command EbcController::CreateCommand(Command_t c, const vector<Parameter>& parameters) const
{
    auto cmds = GetCommands();
    bool found = false;
    for (auto & cc : cmds)
    {
        if (c == cc) {
            found = true;
        }
    }

    if (!found) {
        Logger::LogE(String(F("command 0x")) + String(c, HEX) + F(" is not defined on controller ") + GetModel());
        return Command(); // Invalid
    }

    const vector<Parameter> params = GetCommandParameters(c);
    if (params.size() != parameters.size()) {
        Logger::LogE(String(F("command ")) + String(CommandToString(c)) + F(": parameter sizes do not match"));
        return Command(); // Invalid
    }

    for (auto itA = params.begin(), itB = parameters.begin(); (itA != params.end()) && (itB != parameters.end()); ++itA, ++itB)
    {
        if (itA->index != itB->index) {
            Logger::LogE(String(F("command ")) + String(CommandToString(c)) + F(": parameter index do not match"));
            return Command(); // Invalid
        }
        if (itA->name != itB->name) {
            Logger::LogE(String(F("command ")) + String(CommandToString(c)) + F(": parameter name do not match"));
            return Command(); // Invalid
        }
        if (itA->packing != itB->packing) {
            Logger::LogE(String(F("command ")) + String(CommandToString(c)) + F(": parameter packing do not match"));
            return Command(); // Invalid
        }
    }

    return Command(this, c, parameters);
}

Command_t EbcController::GetCommand(const String& command) const
{
    Command_t cmd = Command::InvalidCommand;
    auto allCommands = GetCommands();
    for (auto & c : allCommands) {
        if (command == CommandToString(c)) {
            cmd = c;
        }
    }
    return cmd;
}

Command EbcController::CreateCommand(JsonObject& jsonObj) const
{
    const char* command = jsonObj["command"]; // "C-CV"
    JsonObject jparameters = jsonObj["parameters"];
    Command_t cmd = GetCommand(command);

    auto parameters = GetCommandParameters(cmd);
    for (auto & p : parameters) {
        if (jparameters.containsKey(p.name)) {
            p.value = jparameters[p.name];
        } else {
            if (p.mandatory) {
                Logger::LogE(String(F("command ")) + String(CommandToString(cmd)) + F(": parameter not found in json object: ") + p.name);
                return Command(); // Invalid
            }
        }
    }

    return CreateCommand(cmd, parameters);
}

Command EbcController::CreateCommand(const String& jsonCommand)
{
    StaticJsonDocument<192> doc;

    DeserializationError error = deserializeJson(doc, jsonCommand);

    if (error) {
        Logger::LogE(String(F("deserializeJson() failed: ")) + String(error.f_str()));
        return Command(); // Invalid
    }

    JsonObject root = doc.to<JsonObject>();
    return CreateCommand(root);
}

std::vector<Parameter> EbcController::GetResponseParameters() const
{
    std::vector<Parameter> parameters = GetResponseParameters(mode);

    for (auto& p : parameters) {
        if (p.index < 0 || value.size() <= p.index) {
            Logger::LogE(String(F("parameter ")) + p.name + F(" not in range of values of response message 0x") + String(mode, HEX));
            continue; // skip this parameter, because it is not valid
        }
        p.source = value[p.index];

        if (!Decode(p.source, p.value, p.packing)) {
            Logger::LogE(String(F("there is no decoder for parameter ")) + p.name + F(" in response message 0x") + String(mode, HEX));
        }
    }

    return parameters;
}

/* e.g.
{
  "mode": "C-CV (active)",
  "parameters":
  {
    "voltageV": 3.961,
    "currentA": 0.355,
    "cutoffA": 0.1
  }
}
*/

String EbcController::GetResponseJson() const
{
    StaticJsonDocument<192> doc;

    JsonObject root = doc.to<JsonObject>();
    root["mode"] = ModeAsString();
	JsonObject parameters = root["parameters"].to<JsonObject>();

    for (auto & p : GetResponseParameters()) {
        if (p.packing == PP_None)
            continue; // skip undefined parameters

        parameters[p.name] = p.value;
    }

    String output;
    serializeJson(doc, output);
    return output;
}