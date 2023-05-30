#ifndef _EBCUNKNOWN_HPP_
#define _EBCUNKNOWN_HPP_

#include "EbcController.hpp"


class EbcUnknown : public EbcController
{
    public:

        EbcUnknown();
        virtual const char *GetModel() const;
        virtual std::vector<Parameter> GetCommandParameters(Command_t cmd) const;
        virtual std::vector<Parameter> GetResponseParameters(Mode_t responseMode) const;
        // virtual Command CreateCommand(Command_t cmd, std::vector<Parameter> parameter);
        virtual const char* ModeAsString() const;
        virtual const char* CommandToString(Command_t cmd) const;
        virtual bool ModeIsActive() const;
        virtual bool ModeIsStopped() const;
        virtual bool ModeIsFinished() const;
        virtual bool Decode(uint16_t source, double& out, ParameterPacking pp) const;
        virtual bool Encode(double source, uint16_t& out, ParameterPacking pp) const;

};

#endif // _EBCUNKNOWN_HPP_