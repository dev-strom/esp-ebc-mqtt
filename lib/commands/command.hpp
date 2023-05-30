#ifndef _COMMAND_HPP_
#define _COMMAND_HPP_

#include "Message.hpp"
#include "Parameter.hpp"
#include <vector>


#define Command_t uint8_t


class Command : public Message
{
    public:

        static const Command_t InvalidCommand = 0x00;
        static const size_t DATA_VALUES_LEN = 3;

        Command();
        Command(const Command&); // user-defined copy constructor
        Command& operator=(const Command&);

        Command_t GetCommand() const;
        const char* GetCommandStr() const;

    protected:

        void SetParameter(const Parameter& parameter);
        virtual void FillBytes();

    private:
        friend class EbcController;

        Command(const void *controller, Command_t cmd);
        Command(const void *controller, Command_t cmd, const std::vector<Parameter>& parameters);

        const void *controller;

        Command_t command;
        std::vector<uint16_t> values;
};

#endif // _COMMAND_HPP_