#ifndef _PARAMETERS_HPP_
#define _PARAMETERS_HPP_

#include <Arduino.h>
#include <vector>


// voltage, set voltage, current, set current, power, time (minutes), capacity
enum ParameterPacking { PP_None, PP_V, PP_V_set, PP_A, PP_A_set, PP_P, PP_T, PP_Ah };

struct ParameterName
{
    static const char* currentA;
    static const char* voltageV;
    static const char* cutoffA;
    static const char* cutoffV;
    static const char* powerW;
    static const char* maxTimeM;
    static const char* capacityAh;
    static const char* currentSetA;
    static const char* voltageSetV;
    static const char* powerSetW;
    static const char* maxTimeSetM;
    static const char* unknown;
};

struct Parameter
{
    Parameter(size_t i, const char* n, ParameterPacking p, bool m = true)
        : index(i), name(n), packing(p), source(0), value(0.0), mandatory(m) {}
    size_t              index;         // the position of this parameter in the 7 values
    String              name;          // the name of this parameter (also used for json)
    ParameterPacking    packing;       // a hint how the packing of this parameter is performed
    uint16_t            source;        // the original received value (bit for bit) from the pdu
    double              value;         // value (unpacked)
    bool                mandatory;     // is this parameter mandatory or optional in a command pdu
};

class ParameterStore
{
    public:

        ParameterStore();

        void Push(const std::vector<Parameter>& parameters);
        bool HasChanged(const char* parameterName);
        double GetValue(const char* parameterName);

    private:

        std::vector<Parameter> nullParameters;
        std::vector<Parameter> actualParameters;
        std::vector<Parameter> lastParameters;
};

#endif // _PARAMETERS_HPP_