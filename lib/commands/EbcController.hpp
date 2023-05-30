#ifndef _EBCCONTROLLER_HPP_
#define _EBCCONTROLLER_HPP_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "Command.hpp"
#include "Response.hpp"
#include "Parameter.hpp"



class EbcController
{
    public:

        struct CommandDescript
        {
            CommandDescript(Command_t c, const std::vector<Mode_t>& r) {
                command = c;
                responses = r;
                active = Response::InvalidMode;
                finish = Response::InvalidMode;
                stopped = Response::InvalidMode;
            };
            CommandDescript(Command_t c, Mode_t a, Mode_t f, Mode_t s)
                : command(c), active(a), finish(f), stopped(s)
            {
                responses.push_back(a);
                responses.push_back(f);
                responses.push_back(s);
            };
            Command_t           command;
            Mode_t              active;
            Mode_t              finish;
            Mode_t              stopped;
            std::vector<Mode_t> responses;
        };

        virtual const char *GetModel() const = 0;
        virtual const char* ModeAsString() const = 0;
        virtual const char* CommandToString(Command_t cmd) const = 0;
        virtual bool ModeIsActive() const = 0;
        virtual bool ModeIsStopped() const = 0;
        virtual bool ModeIsFinished() const = 0;

        virtual bool Decode(uint16_t source, double& out, ParameterPacking pp) const = 0;
        virtual bool Encode(double source, uint16_t& out, ParameterPacking pp) const = 0;


        Command CreateConnect() const;
        Command CreateDisconnect() const;
        Command CreateStop() const;

        Command CreateCommand(Command_t cmd, const std::vector<Parameter>& parameters) const;
        Command CreateCommand(JsonObject& jsonObj) const;
        Command CreateCommand(const String& jsonCommand);

        std::vector<Command_t> GetCommands() const;
        virtual std::vector<Parameter> GetCommandParameters(Command_t cmd) const = 0;
        Command_t GetCommand(const String& command) const;

        std::vector<Mode_t> GetResponses() const;
        virtual std::vector<Parameter> GetResponseParameters(Mode_t responseMode) const = 0;
        std::vector<Parameter> GetResponseParameters() const;
        String GetResponseJson() const;

        bool IsValidResponseForCommand(Command_t cmd) const;
        bool IsActiveResponseForCommand(Command_t cmd) const;
        bool IsFinishedResponseForCommand(Command_t cmd) const;
        bool IsStoppedResponseForCommand(Command_t cmd) const;
        bool IsValidData() const;

        static EbcController& GetController();
        static EbcController& GetController(uint8_t id);
        static EbcController& GetController(const Response& response);

    protected:

        EbcController();

        std::vector<EbcController::CommandDescript> commands;

        void SetData(const std::vector<uint16_t>& data, Mode_t mode);

        Mode_t mode;

        // length of vector is always Response::DATA_VALUES_LEN
        std::vector<uint16_t> value;
        // enum ValuePos_t { currentActual = 0, voltageActual, capacityActual, unknown, currentSet, voltageSet, currentCutoff};

    private:

        Command_t FindCommand(Command_t cmd) const;
};

#endif // _EBCCONTROLLER_HPP_