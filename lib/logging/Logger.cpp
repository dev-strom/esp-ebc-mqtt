
#include "Logger.hpp"



Logger::Logger()
    : log(nullptr)
{
}


void Logger::SetLogger(LogDelegate l)
{
    log = l;
}

void Logger::Log(LogSeverity l, const String& msg) const
{
    Log(l, msg.c_str());
}

void Logger::Log(LogSeverity l, const char* msg) const
{
    if (log != nullptr)
    {
        log(l, msg);
    }
}

void Logger::LogE(const char* msg)
{
    GetInstance().Log(Error, msg);
}

void Logger::LogM(const char* msg)
{
    GetInstance().Log(Message, msg);
}

void Logger::LogD(const char* msg)
{
    GetInstance().Log(Debug, msg);
}

void Logger::LogE(const String& msg)
{
    GetInstance().Log(Error, msg);
}

void Logger::LogM(const String& msg)
{
    GetInstance().Log(Message, msg);
}

void Logger::LogD(const String& msg)
{
    GetInstance().Log(Debug, msg);
}
