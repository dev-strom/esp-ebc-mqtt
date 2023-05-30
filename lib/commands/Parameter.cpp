#include "Parameter.hpp"
#include "Logger.hpp"

const char* ParameterName::currentA = "currentA";
const char* ParameterName::voltageV = "voltageV";
const char* ParameterName::cutoffA = "cutoffA";
const char* ParameterName::cutoffV = "cutoffV";
const char* ParameterName::powerW = "powerW";
const char* ParameterName::maxTimeM = "maxTimeM";
const char* ParameterName::capacityAh = "capacityAh";
const char* ParameterName::currentSetA = "currentSetA";
const char* ParameterName::voltageSetV = "voltageSetV";
const char* ParameterName::powerSetW = "powerSetW";
const char* ParameterName::maxTimeSetM = "maxTimeSetM";
const char* ParameterName::unknown = "unknown";




ParameterStore::ParameterStore()
    :   nullParameters(),
        actualParameters(nullParameters),
        lastParameters(nullParameters)
{}

void ParameterStore::Push(const std::vector<Parameter>& parameters)
{
    lastParameters = actualParameters;
    actualParameters = parameters;
}

bool ParameterStore::HasChanged(const char* parameterName)
{
    for (auto & p : actualParameters) {
        if (p.name == parameterName) {
            for (auto & l : lastParameters) {
                if (l.name == parameterName) {
                    return l.source != p.source;
                }
            }
            Logger::LogD(String(F("parameter ")) + String(parameterName) + F(" not found in last store"));
            return true; // not found in last parameters -> so it has changed
        }
    }
    Logger::LogD(String(F("parameter ")) + String(parameterName) + F(" not found in store"));
    return false; // not found in parameters -> not changed
}

double ParameterStore::GetValue(const char* parameterName)
{
    for (auto & p : actualParameters) {
        if (p.name == parameterName) {
            return p.value;
        }
    }
    Logger::LogD(String(F("parameter ")) + String(parameterName) + F(" not found in value store"));
    return 0.0; // not found in parameters
}

