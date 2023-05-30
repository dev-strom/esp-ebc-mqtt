#include "Processor.hpp"
#include "Logger.hpp"


Processor::Processor()
:   command(nullptr),
    report(nullptr),
    running(false),
    currentStep(0)
{
    timer = timer_create_default();
}

void Processor::SetCommmander(CommandDelegate c)
{
    command = c;
}

void Processor::SetReportHandler(ReportDelegate r)
{
    report = r;
}

void Processor::SetEventHandler(EventDelegate e)
{
    event = e;
}

bool Processor::Report(const String& key, const String& value)
{
    if (report == nullptr) {
        Logger::LogD(F("report failed (nullptr)"));
        return false;
    }
    return report(key, value);
}

void Processor::Clear()
{
    if (running)
        Stop();

    steps.clear();
    currentStep = 0;
    results.clear();
    name = "";
    Report("state", "idle");
}


void Processor::Load(const EbcController& controller, const String& jsonStr)
{
    Clear();

    StaticJsonDocument<1024> doc;
    // DynamicJsonDocument doc(1024);

    DeserializationError error = deserializeJson(doc, jsonStr);

    if (error) {
        Logger::LogE(String(F("deserializeJson() failed: ")) + String(error.f_str()));
        return;
    }

    name = (const char*) doc["name"];
    JsonArray jsteps = doc["steps"];

    for (JsonVariant v : jsteps) {
        String command = v["command"];

        if (command == "Wait") {
            unsigned int seconds = v["seconds"];
            if (5 <= seconds) {
                AddStepWait(seconds);
            } else {
                unsigned int minutes = v["minutes"];
                if (1 <= minutes) {
                    AddStepWait(minutes*60);
                } else {
                    Logger::LogE(String(F("program \"")) + name + F("\": invalid program step ") + steps.size() + F(": duration"));
                    Clear();
                    return;
                }
            }
        } else
        if (command == "Cycle") {
            unsigned int step_index = v["step"];
            unsigned int count = v["count"];
            if (steps.size() <= step_index) {
                Logger::LogE(String(F("program \"")) + name + F("\": invalid program step ") + steps.size() + F(": destination"));
                Clear();
                return;
            }
            if (((unsigned short)(-1)) < count) {
                Logger::LogE(String(F("program \"")) + name + F("\": invalid program step ") + steps.size() + F(": count"));
                Clear();
                return;
            }
            AddStepCycle(step_index, count);
        } else {
            // command: D-CC, D-CP, C-CV
            JsonObject obj = v.as<JsonObject>();
            if (obj.isNull()) {
                Logger::LogE(String(F("program \"")) + name + F("\": invalid program step ") + steps.size() + F(": cannot cast to object"));
                Clear();
                return;
            }
            auto cmd = controller.CreateCommand(obj);
            if (cmd.GetCommand() == Command::InvalidCommand) {
                Logger::LogE(String(F("program \"")) + name + F("\": invalid program step ") + steps.size() + F(": invalid command"));
                Clear();
                return;
            }

            StopCondition stopCond;

            JsonObject j = v["stopCondition"];
            if (!j.isNull()) {
                for (const auto& kv : j) {
                    stopCond.parameterName = kv.key().c_str();
                    // currently we do only support "capacityAh"
                    if (stopCond.parameterName != ParameterName::capacityAh) {
                        Logger::LogE(String(F("program \"")) + name + F("\": invalid program step ") + steps.size() + F(": invalid parameter name of stop condition"));
                        Clear();
                        return;
                    }
                    if (kv.value().is<double>()) {
                        stopCond.kind = StopCondition::Condition_Absolute;
                        stopCond.value = kv.value().as<double>();
                    } else
                    if (kv.value().is<int>()) {
                        stopCond.kind = StopCondition::Condition_Absolute;
                        stopCond.value = kv.value().as<int>();
                    } else
                    if (kv.value().is<const char*>()) {
                        String value = kv.value().as<const char*>();
                        stopCond.kind = StopCondition::Condition_Absolute;
                        int pos = value.indexOf('%');
                        if (0 <= pos) {
                            value = value.substring(0, pos);
                            stopCond.kind = StopCondition::Condition_Percent;
                        }
                        stopCond.value = value.toDouble();
                    }
                }
            }

            AddStepCommand(cmd, stopCond);
        }
    }
    if (!Report("program", jsonStr)) {
        Logger::LogD(String(F("program \"")) + name + F("\": report failed"));
    } else {
        Report("state", "loaded");
        Logger::LogD(String(F("program \"")) + name + F("\": has ") + steps.size() + F(" steps, controller is ") + controller.GetModel());
    }
}

void Processor::AddStepWait(unsigned short seconds)
{
    Step step(Step::Step_Wait);
    step.seconds = seconds;
    steps.push_back(step);
    Logger::LogD(F("added step: Wait"));
}

void Processor::AddStepCycle(unsigned short step_index, unsigned short count)
{
    if (0 <= step_index && step_index < steps.size()) {
        Step step(Step::Step_Cycle);
        step.step_index = step_index;
        step.count = count;
        step.current_cycle = 0;
        steps.push_back(step);
        Logger::LogD(F("added step: Cycle"));
    } else {
        Logger::LogE(F("invalid step: Cycle"));
    }
}

void Processor::AddStepCommand(Command command, StopCondition stop_cond)
{
    Step step(Step::Step_Command);
    step.command = command;
    step.stop_condition = stop_cond;
    steps.push_back(step);
    Logger::LogD(String(F("added step: ")) + String(command.GetCommandStr()) + F(" / ") + stop_cond.ToString());
}

String Processor::StopCondition::ToString()
{
    String ret;

    switch (kind)
    {
        case Condition_Absolute:
            ret += String(F("stop condition \"absolute\" (\"")) + parameterName + F("\" = ") + String(value, 3) + F(")");
            break;
        case Condition_Percent:
            ret += String(F("stop condition \"percent\" (\"")) + parameterName + F("\" = ") + String(value, 0) + F("%)");;
            break;
        case Condition_None:
            ret += F("stop condition \"none\"");
            break;
        default:
            break;
    }

    return ret;
}

bool Processor::Run()
{
    results.clear();
    Report("state", "running");
    Report("run", "on");
    running = true;
    StartStep(0);
    return true;
}

void Processor::Stop()
{
    timer.cancel();
    running = false;
    if ((currentStep < steps.size()) && steps[currentStep].command_active) {
        command(EbcController::GetController().CreateStop());
        Report("state", "stopped");
    }
    Report("run", "off");
}

bool Processor::IsRunning()
{
    return running;
}

bool Processor::WaitTimeout(void *p)
{
    return ((Processor*)p)->WaitTimeout();
}

bool Processor::WaitTimeout()
{
    StartStep(currentStep + 1);
    return true;
}

void Processor::ReportStep(size_t index)
{
    auto& step = steps[index];

    StaticJsonDocument<192> doc;

    JsonObject root = doc.to<JsonObject>();
    root["step"] = index;

    switch (step.action) {
        case Step::Step_Wait:
            root["command"] = "Wait";
            if (0 < step.seconds) {
                if (step.seconds % 60 != 0) {
                    root["duration"] = String(step.seconds) + "s";
                } else {
                    root["duration"] = String(step.seconds/60) + "m";
                }
            }
            break;
        case Step::Step_Cycle:
            root["command"] = "Cycle";
            root["cycle_step"] = step.step_index;
            root["num"] = step.current_cycle;
            root["count"] = step.count;
            break;
        case Step::Step_Command:
            root["command"] = step.command.GetCommandStr();
            root["capacityAh"] = step.capacity;
            break;
    }

    String output;
    serializeJson(doc, output);
    results.push_back(output);

    bool addComma = false;
    output = "[";
    for (auto & r : results) {
        if (addComma) {
            output += ",";
        } else {
            addComma = true;
        }
        output += r;
    }
    output += "]";
    
    Report("result", output);
}

void Processor::StartStep(size_t index)
{
    this->currentStep = index;
    if (this->running) {
        // start only the first step immediate, the later got a pause of 5s
        timer.in((0 < index) ? 5000 : 0, Processor::RunNow, this);
    }
}

bool Processor::RunNow(void *p)
{
    ((Processor*)p)->PerformStep();
    return true;
}

void Processor::PerformStep()
{
    if (0 < currentStep) {
        ReportStep(currentStep - 1);
    }
    if (steps.size() <= currentStep) {
        // finished
        Logger::LogM(String(F("program \"")) + name + F("\": end "));
        Report("step", "");
        Report("run", "off");
        Report("state", "end");
        running = false;
        if (event != nullptr) {
            event(Cpu_Program_End);
        }
        return;
    }
    Report("step", String(currentStep));
    auto& step = steps[currentStep];
    switch (step.action) {
        case Step::Step_Wait:
            {
                String duration;
                if (step.seconds % 60 != 0) {
                    duration = String(step.seconds) + F(" seconds");
                } else {
                    duration = String(step.seconds/60) + F(" minutes");
                }
                Logger::LogM(String(F("program \"")) + name + F("\": perform step ") + currentStep + F(": Wait ") + duration);
            }
            timer.in(step.seconds * 1000, WaitTimeout, this);
            break;
        case Step::Step_Cycle:
            if (step.count == step.current_cycle) {
                // next step
                step.current_cycle = 0;
                Logger::LogM(String(F("program \"")) + name + F("\": perform step ") + currentStep + F(": Cycle elapsed"));
                StartStep(currentStep + 1);
            } else {
                // cycle back
                Logger::LogM(String(F("program \"")) + name + F("\": perform step ") + currentStep
                    + F(": Cycle to step ") + step.step_index + F(" (") + (step.current_cycle+1) + F("/") + step.count + F(")"));
                step.current_cycle++;
                StartStep(step.step_index);
            }
            break;
        case Step::Step_Command:
            Logger::LogM(String(F("program \"")) + name + F("\": perform step ") + currentStep + F(": Command ") + step.command.GetCommandStr() );
            if (command != nullptr) {
                bool success = command(step.command);
                step.command_active = true;
                if (!success) {
                    Logger::LogE(String(F("program \"")) + name + F("\": step ") + currentStep + F(": failed to send command"));
                    Stop();
                    return;
                }
            }
            break;
    }
}

void Processor::InjectData(const EbcController& controller)
{
    if (steps.size() <= currentStep) {
        // finished
        return;
    }
    auto& step = steps[currentStep];
    if (step.action != Step::Step_Command) {
        return;
    }
    if (!step.command_active) {
        return;
    }

    Command_t cmd = step.command.GetCommand();
    if (controller.IsActiveResponseForCommand(cmd)) 
    {
        const std::vector<Parameter>& parameters = controller.GetResponseParameters();
        for (auto & p : parameters) {
            if (p.name == ParameterName::capacityAh) {
                step.capacity = p.value;
                break; // for
            }
        }
    }

    // check additional stop condition here!
    bool shouldActionBeStopped = false;
    if (step.stop_condition.kind != StopCondition::Condition_None)
    {
        const std::vector<Parameter>& parameters = controller.GetResponseParameters();
        for (auto & p : parameters) {
            auto pvalue = p.value;
            if (p.name == step.stop_condition.parameterName) {
                switch (step.stop_condition.kind)
                {
                case StopCondition::Condition_Absolute:
                    if (step.stop_condition.value <= pvalue) {
                        Logger::LogD(String(F("program \"")) + name + F("\": step ") + currentStep
                         + F(": absolute stop condition hit: ") + String(step.stop_condition.value, 3) + F(" <= ") + String(pvalue, 3));
                        shouldActionBeStopped = true;
                    }
                    break;
                case StopCondition::Condition_Percent:
                    {
                        double capacity = 0.0;
                        // find the value to compare
                        for (int idx = currentStep-1; 0 <= idx; --idx) {
                            auto& s = steps[idx];
                            if (s.action == Step::Step_Command) {
                                // Logger::LogD(F("program \"") + name + F("\": step ") + currentStep + F(": found capacity to compare in step ") + idx + F(": value = ") + s.capacity);
                                capacity = s.capacity;
                                break; // for;
                            }
                        }
                        double percent_value = (capacity * step.stop_condition.value / 100.0);
                        if (percent_value <= pvalue) {
                            Logger::LogD(String(F("program \"")) + name + F("\": step ") + currentStep
                            + F(": relative stop condition hit: (") + step.stop_condition.value + F("% of ") + String(capacity, 3) + F(" = ") + String(percent_value, 3) + F(") <= ") + String(pvalue, 3));
                            shouldActionBeStopped = true;
                        }
                    }
                    break;
                
                default:
                    break;
                }
                break; // for
            }
        }
    }

    if (shouldActionBeStopped) {
        // stop!
        command(controller.CreateStop());
        step.stop_issued = true;
    }

    if (controller.IsFinishedResponseForCommand(cmd)) {
        step.command_active = false;
        if (event != nullptr) {
            event(Cpu_Command_Finished);
        }
        StartStep(currentStep + 1);
        return;
    }

    if (controller.IsStoppedResponseForCommand(cmd)) {
        if (!step.stop_issued) {
            Logger::LogD(String(F("program \"")) + name + F("\": step ") + currentStep + F(": missed finishd message for command ") + step.command.GetCommandStr());
        }
        step.command_active = false;
        if (event != nullptr) {
            event(Cpu_Command_Finished);
        }
        StartStep(currentStep + 1);
        return;
    }
}
