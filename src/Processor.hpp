#ifndef _PROCESSOR_HPP_
#define _PROCESSOR_HPP_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <arduino-timer.h>

#include <vector>
#include "Command.hpp"
#include "Response.hpp"
#include "Parameter.hpp"
#include "EbcController.hpp"


class Processor
{
    public:

        enum CpuEvent { Cpu_Command_Finished, Cpu_Program_End };

        typedef bool (*CommandDelegate) (const Command& cmd);
        typedef bool (*ReportDelegate) (const String& key, const String& value);
        typedef void (*EventDelegate) (CpuEvent e);

        struct StopCondition
        {
            enum ConditionType {Condition_None, Condition_Absolute, Condition_Percent};

            StopCondition()
                : kind(Condition_None) {}

            String        parameterName;
            ConditionType kind;
            double        value;

            String ToString();
        };

        struct Step
        {
            enum Step_t {Step_Command, Step_Wait, Step_Cycle};

            Step(Step_t c)
                : action(c), seconds(0), step_index(0), count(0),
                  current_cycle(0), command(), command_active(false), stop_issued(false), capacity(0.0) {}

            Step_t                  action;
            unsigned short          seconds;        // used by Step_Wait
            unsigned short          step_index;     // used by Step_Cycle
            unsigned short          count;          // used by Step_Cycle
            unsigned short          current_cycle;  // used by Step_Cycle
            Command                 command;        // used by Step_Command
            bool                    command_active; // used by Step_Command
            StopCondition           stop_condition; // used by Step_Command
            bool                    stop_issued;    // used by Step_Command
            double                  capacity;       // on command step the accumulated data (Ah)
            private:
            Step() {}
        };

        Processor();

        void SetCommmander(CommandDelegate c);
        void SetReportHandler(ReportDelegate r);
        void SetEventHandler(EventDelegate e);

        void Clear();
        void Load(const EbcController& controller, const String& json);
        void AddStepWait(unsigned short seconds);
        void AddStepCycle(unsigned short step, unsigned short count);
        void AddStepCommand(Command command, StopCondition stop_cond = StopCondition());

        bool Run();
        void Stop();
        bool IsRunning();

        void InjectData(const EbcController& controller);
        void Tick() {timer.tick();}

    private:

        CommandDelegate         command;
        ReportDelegate          report;
        EventDelegate           event;

        String name;
        std::vector<Step> steps;
        std::vector<String> results;
        bool running;
        size_t currentStep;
        Timer<> timer;

        static bool WaitTimeout(void *p);
        bool WaitTimeout();

        static bool RunNow(void *p);

        void ReportStep(size_t index);
        void StartStep(size_t index);
        void PerformStep();

        bool Report(const String& key, const String& value);
};

#endif // _PROCESSOR_HPP_