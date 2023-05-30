#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <Arduino.h>

class Logger
{
    public:

        enum LogSeverity {Debug, Message, Error};
        typedef void (*LogDelegate) (LogSeverity level, const char* msg);

        static Logger& GetInstance()
        {
            static Logger instance;
            return instance;
        }

        Logger(Logger const&) = delete;
        void operator=(Logger const&) = delete;

        void SetLogger(LogDelegate l);
        void Log(LogSeverity l, const String& msg) const;
        void Log(LogSeverity l, const char* msg) const;

        static void LogE(const char* msg); // log errors
        static void LogM(const char* msg); // log info messages
        static void LogD(const char* msg); // log debug messages
        static void LogE(const String& msg); // log errors
        static void LogM(const String& msg); // log info messages
        static void LogD(const String& msg); // log debug messages

    private:

        Logger();

        LogDelegate log;
};

#endif // _LOGGER_HPP_