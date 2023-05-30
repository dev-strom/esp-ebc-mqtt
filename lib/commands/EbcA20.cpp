#include "EbcA20.hpp"

using namespace std;


enum Commands {
    Cmd_Connect    = 0x05, // b0000 0101
    Cmd_Disconnect = 0x06, // b0000 0110
    Cmd_Stop       = 0x02, // b0000 0010
    Cmd_Continue   = 0x18, // b0001 1000
    Cmd_D_CC       = 0x01, // b0000 0001
    Cmd_D_CP       = 0x11, // b0001 0001
    Cmd_C_CV       = 0x21, // b0010 0001
};

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

    // alternative values we don't know what's there purpose (and ignorig them):
    A_D_CC_ACTIVE  = 0x6e, // while D-CC running
    A_D_CC_STOPPED = 0x64, // while D-CC stopped
    A_D_CP_ACTIVE  = 0x6f, // while D-CP running
    A_D_CP_STOPPED = 0x65, // while D-CP stopped
    A_C_CV_ACTIVE  = 0x70, // while C-CV running
    A_C_CV_STOPPED = 0x66, // while C-CV stopped
};

/*
    Cmd_D_CC       = 0x01, // 0000 0001
    Cmd_D_CP       = 0x11, // 0001 0001
    Cmd_C_CV       = 0x21, // 0010 0001

    D_CC_ACTIVE    = 0x0a, // 0000 1010
    D_CP_ACTIVE    = 0x0b, // 0000 1011
    C_CV_ACTIVE    = 0x0c, // 0000 1100

    D_CC_STOPPED   = 0x00, // 0000 0000
    D_CP_STOPPED   = 0x01, // 0000 0001
    C_CV_STOPPED   = 0x02, // 0000 0010

    D_CC_FINISHED  = 0x14, // 0001 0100
    D_CP_FINISHED  = 0x15, // 0001 0101
    C_CV_FINISHED  = 0x16, // 0001 0110

    A_D_CC_ACTIVE  = 0x6e, // 0110 1110
    A_D_CP_ACTIVE  = 0x6f, // 0110 1111
    A_C_CV_ACTIVE  = 0x70, // 0111 0000

    A_D_CC_STOPPED = 0x64, // 0110 0100
    A_D_CP_STOPPED = 0x65, // 0110 0101
    A_C_CV_STOPPED = 0x66, // 0110 0110
*/

bool EbcA20::ModeIsActive() const
{
    return (mode & 0xf8) == 0x08;
}

bool EbcA20::ModeIsStopped() const
{
    return (mode & 0xf8) == 0x00;
}

bool EbcA20::ModeIsFinished() const
{
    return (mode & 0xf8) == 0x10;
}

const char* EbcA20::ModeAsString() const
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

const char* EbcA20::CommandToString(Command_t cmd) const
{
    switch (cmd)
    {
    case Cmd_Connect:             return "Connect";
    case Cmd_Disconnect:          return "Disconnect";
    case Cmd_Stop:                return "Stop";
    case Cmd_Continue:            return "Continue";
    case Cmd_C_CV:                return "C-CV";
    case Cmd_D_CC:                return "D-CC";
    case Cmd_D_CP:                return "D-CP";
    case Command::InvalidCommand: return "Invalid";
    default:                      return "UNKNOWN";
    }
}



const char *EbcA20::GetModel() const
{
    return "EBC-A20";
};


EbcA20::EbcA20()
    : EbcController()
{
    // this is a kind of transition matrix
    // the esp can always send every command
    // every line shows what kind of responses are valid after the given command is send
    commands.clear();
    commands.push_back(CommandDescript(Cmd_Connect, vector<Mode_t> {D_CC_ACTIVE, D_CC_STOPPED, D_CP_ACTIVE, D_CP_STOPPED, C_CV_ACTIVE, C_CV_STOPPED}));
    commands.push_back(CommandDescript(Cmd_Disconnect, vector<Mode_t> {}));
    commands.push_back(CommandDescript(Cmd_C_CV, C_CV_ACTIVE, C_CV_FINISHED, C_CV_STOPPED));
    commands.push_back(CommandDescript(Cmd_D_CC, D_CC_ACTIVE, D_CC_FINISHED, D_CC_STOPPED));
    commands.push_back(CommandDescript(Cmd_D_CP, D_CP_ACTIVE, D_CP_FINISHED, D_CP_STOPPED));
    commands.push_back(CommandDescript(Cmd_Stop, vector<Mode_t> {D_CC_STOPPED, D_CP_STOPPED, C_CV_STOPPED}));
    // commands.push_back(CommandDescript(Cmd_Continue, vector<Mode_t> {C_CV_ACTIVE, C_CV_STOPPED, C_CV_FINISHED}));
}

std::vector<Parameter> EbcA20::GetCommandParameters(Command_t cmd) const
{
    switch (cmd)
    {
    case Cmd_C_CV:
        return vector<Parameter> {
            Parameter(0, ParameterName::currentA, ParameterPacking::PP_A_set),
            Parameter(1, ParameterName::voltageV, ParameterPacking::PP_V_set), // 0.00-30.00V, stepper 0.01V (In charge mode, the maximum voltage is18V)
            Parameter(2, ParameterName::cutoffA, ParameterPacking::PP_A_set), // In charge, 0.10-5.00A, stepper 0.01A (max current depends on power current)
        };
    case Cmd_D_CC:
        return vector<Parameter> {
            Parameter(0, ParameterName::currentA, ParameterPacking::PP_A_set),
            Parameter(1, ParameterName::cutoffV, ParameterPacking::PP_V_set),
            Parameter(2, ParameterName::maxTimeM, ParameterPacking::PP_T, false),
        };
    case Cmd_D_CP:
        return vector<Parameter> {
            Parameter(0, ParameterName::powerW, ParameterPacking::PP_P),
            Parameter(1, ParameterName::cutoffV, ParameterPacking::PP_V_set),
            Parameter(2, ParameterName::maxTimeM, ParameterPacking::PP_T, false),
        };
    
    default:
        return vector<Parameter>();
    }
}

std::vector<Parameter> EbcA20::GetResponseParameters(Mode_t responseMode) const
{
    switch (responseMode)
    {
    case D_CC_ACTIVE:
    case D_CC_STOPPED:
    case D_CC_FINISHED:
        return vector<Parameter> {
            Parameter(0, ParameterName::currentA, ParameterPacking::PP_A),
            Parameter(1, ParameterName::voltageV, ParameterPacking::PP_V),
            Parameter(2, ParameterName::capacityAh, ParameterPacking::PP_Ah),
            // Parameter(3, ParameterName::unknown, ParameterPacking::PP_None),
            Parameter(4, ParameterName::currentSetA, ParameterPacking::PP_A_set),
            Parameter(5, ParameterName::voltageSetV, ParameterPacking::PP_V_set),
            Parameter(6, ParameterName::maxTimeM, ParameterPacking::PP_T),
        };
    case D_CP_ACTIVE:
    case D_CP_STOPPED:
    case D_CP_FINISHED:
        return vector<Parameter> {
            Parameter(0, ParameterName::currentA, ParameterPacking::PP_A),
            Parameter(1, ParameterName::voltageV, ParameterPacking::PP_V),
            Parameter(2, ParameterName::capacityAh, ParameterPacking::PP_Ah),
            // Parameter(3, ParameterName::unknown, ParameterPacking::PP_None),
            Parameter(4, ParameterName::powerSetW, ParameterPacking::PP_P),
            Parameter(5, ParameterName::voltageSetV, ParameterPacking::PP_V_set),
            Parameter(6, ParameterName::maxTimeSetM, ParameterPacking::PP_T),
        };
    case C_CV_ACTIVE:
    case C_CV_STOPPED:
    case C_CV_FINISHED:
        return vector<Parameter> {
            Parameter(0, ParameterName::currentA, ParameterPacking::PP_A),
            Parameter(1, ParameterName::voltageV, ParameterPacking::PP_V),
            Parameter(2, ParameterName::capacityAh, ParameterPacking::PP_Ah),
            // Parameter(3, ParameterName::unknown, ParameterPacking::PP_None),
            Parameter(4, ParameterName::currentSetA, ParameterPacking::PP_A_set),
            Parameter(5, ParameterName::voltageSetV, ParameterPacking::PP_V_set),
            Parameter(6, ParameterName::cutoffA, ParameterPacking::PP_A),
        };
    
    default:
        return vector<Parameter>();
    }
}

// only used by response messages
bool EbcA20::Decode(uint16_t source, double& value, ParameterPacking pp) const
{
    switch (pp) // PP_None, PP_V, PP_V_set, PP_A, PP_A_set, PP_P, PP_T, PP_Ah
    {
    case PP_V:
        // TODO: is PP_V the same way decoded as PP_Ah? (don't know, because I never had a voltage above 10V)
        value = (source == 0x0000) ? 0.0 : ((static_cast<double> (((source >> 8) * 240) + (source & 0xFF))) / 1000.0);
        break;
    case PP_Ah:
        if (source & 0x8000) {
            // capacity >= 10.0 Ah
            if ((source & 0xE000) == 0xE000) {
                // capacity >= 200.0 Ah
                value = ((static_cast<double> ((((source >> 8) & 0x3F) * 240) + (source & 0xFF) - 0x1C00)) / 10.0);
            } else {
                // capacity < 200.0 Ah
                value = ((static_cast<double> ((((source >> 8) & 0x7F) * 240) + (source & 0xFF) - 0x0800)) / 100.0);
            }
        } else {
            // capacity < 10.0 Ah
            value = (source == 0x0000) ? 0.0 : ((static_cast<double> (((source >> 8) * 240) + (source & 0xFF))) / 1000.0);
        }
        break;
    case PP_V_set:
    case PP_A_set:
    case PP_A:
        value = (source == 0x0000) ? 0.0 : ((static_cast<double> (((source >> 8) * 240) + (source & 0xFF))) / 100.0);
        break;
    case PP_T:
    case PP_P:
        value = static_cast<double> (source);
        break;
    case PP_None:
        break;
    default:
        return false;
    }
    return true;
}

// only used by command messages
bool EbcA20::Encode(double source, uint16_t& value, ParameterPacking pp) const
{
    switch (pp) // PP_None, PP_V, PP_V_set, PP_A, PP_A_set, PP_P, PP_T, PP_Ah
    {
        case PP_V_set:
        case PP_A_set:
            {
                uint16_t tmp = static_cast<uint16_t> (source * 100.0);
                value = ((tmp / 240) << 8) | (tmp % 240);
            }
            break;
        case PP_T:
        case PP_P:
            value = (static_cast<uint16_t> (source));
            break;
        default:
            return false;
    }
    return true;
}
