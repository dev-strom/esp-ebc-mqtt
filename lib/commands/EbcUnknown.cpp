#include "EbcUnknown.hpp"

using namespace std;

enum Commands {
    Cmd_Connect    = 0x05, // b0000 0101
    Cmd_Disconnect = 0x06, // b0000 0110
    Cmd_Stop       = 0x02, // b0000 0010
};

// this values are hijacked from the EBC-A20 controller, maybe they are not as universal as we wish
enum ResponseModes {
    //used values:
    D_CC_ACTIVE   = 0x0a, // discharge constant current
    D_CC_STOPPED  = 0x00, // discharge constant current stopped (idle)
    D_CC_FINISHED = 0x14, // discharge constant current finished (idle)
    D_CP_ACTIVE   = 0x0b, // discharge constant power
    D_CP_STOPPED  = 0x01, // discharge constant power stopped (idle)
    D_CP_FINISHED = 0x15, // discharge constant power finished (idle)
    C_CV_ACTIVE   = 0x0c, // charging constant voltage
    C_CV_STOPPED  = 0x02, // charging constant voltage stopped (idle)
    C_CV_FINISHED = 0x16, // charging constant voltage finished (idle)

    // alternative values we don't know what's there purpose:
    A_D_CC_ACTIVE  = 0x6e, // while D-CC running
    A_D_CC_STOPPED = 0x64, // while D-CC stopped
    A_D_CP_ACTIVE  = 0x6f, // while D-CP running
    A_D_CP_STOPPED = 0x65, // while D-CP stopped
    A_C_CV_ACTIVE  = 0x70, // while C-CV running
    A_C_CV_STOPPED = 0x66, // while C-CV stopped
};

bool EbcUnknown::ModeIsActive() const
{
    return (mode & 0xf8) == 0x08;
}

bool EbcUnknown::ModeIsStopped() const
{
    return (mode & 0xf8) == 0x00;
}

bool EbcUnknown::ModeIsFinished() const
{
    return (mode & 0xf8) == 0x10;
}

const char* EbcUnknown::ModeAsString() const
{
    switch (mode)
    {
    case D_CC_ACTIVE:             return "D-CC (active)";
    case D_CC_STOPPED:            return "D-CC (stopped)";
    case D_CC_FINISHED:           return "D-CC (finished)";
    case D_CP_ACTIVE:             return "D-CP (active)";
    case D_CP_STOPPED:            return "D-CP (stopped)";
    case D_CP_FINISHED:           return "D-CP (finished)";
    case C_CV_ACTIVE:             return "C-CV (active)";
    case C_CV_STOPPED:            return "C-CV (stopped)";
    case C_CV_FINISHED:           return "C-CV (finished)";
    case Response::InvalidMode:   return "Invalid";
    default:                      return "Unknown";
    }
}

const char* EbcUnknown::CommandToString(Command_t cmd) const
{
    switch (cmd)
    {
    case Cmd_Connect:             return "Connect";
    case Cmd_Disconnect:          return "Disconnect";
    case Cmd_Stop:                return "Stop";
    case Command::InvalidCommand: return "Invalid";
    default:                      return "UNKNOWN";
    }
}

const char *EbcUnknown::GetModel() const
{
    return "EBC-???"; 
}


EbcUnknown::EbcUnknown()
    : EbcController()
{
    // this is a kind of transition matrix
    // the esp can always send every command
    // every line shows what kind of responses are valid after the given command is send
    commands.clear();
    commands.push_back(CommandDescript(Cmd_Connect, vector<Mode_t> {D_CC_STOPPED, D_CP_STOPPED, C_CV_STOPPED}));
    commands.push_back(CommandDescript(Cmd_Disconnect, vector<Mode_t> {}));
    commands.push_back(CommandDescript(Cmd_Stop, vector<Mode_t> {D_CC_STOPPED, D_CP_STOPPED, C_CV_STOPPED}));
}

std::vector<Parameter> EbcUnknown::GetCommandParameters(Command_t cmd) const
{
    return vector<Parameter>();
}

std::vector<Parameter> EbcUnknown::GetResponseParameters(Mode_t responseMode) const
{
    return vector<Parameter>();
}

bool EbcUnknown::Decode(uint16_t source, double& out, ParameterPacking pp) const
{
    return false;
}

bool EbcUnknown::Encode(double source, uint16_t& out, ParameterPacking pp) const
{
    return false;
}
