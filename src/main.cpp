#include <Arduino.h>
#include <Homie.h>
#include <arduino-timer.h>

#ifdef ESP8266
#include <SoftwareSerial.h>
#endif

#include <Fsm.h>

#include <queue>
#include "Logger.hpp"
#include "Command.hpp"
#include "Response.hpp"
#include "Processor.hpp"
#include "EbcController.hpp"
#include "fw_version.h"


#ifdef ESP8266
SoftwareSerial ebcSerial(4, 5);     // RX, TX
#else
HardwareSerial ebcSerial = Serial2;
#endif



static Command        nextCommand;
static Command        activeCommand;
static Response       response;
static EbcController* controller = &EbcController::GetController();
static ParameterStore store;
static Processor      processor;
static String         cpuProgramLoadPending;

#ifdef ESP8266
HomieNode esp("esp", "ESP8266", "system");
#else
HomieNode esp("esp", "ESP32", "system");
#endif
HomieNode raw("raw", "RAW data", "raw");
HomieNode ebc("controller", "Controller", "controller");
HomieNode cpu("cpu", "Processor", "cpu");

// Home callback functions
bool connectionHandler(const HomieRange& range, const String& value);
bool cpuProgramLoadHandler(const HomieRange& range, const String& value);
bool cpuProgramRunHandler(const HomieRange& range, const String& value);

// FSM callback functions
void on_enter_disconnected();
void on_enter_connected();
void on_inject_data();
void on_data();
void on_load();

// FSM events
enum Event {
  Evt_init,
  Evt_connect,
  Evt_disconnect,
  Evt_command,
  Evt_command_finished,
  Evt_data,
  Evt_response,
  Evt_load,
  Evt_run,
  Evt_stop,
  Evt_end
};

// FSM states
//                                 State(void (*on_enter)(),      void (*on_state)(),  void (*on_exit)());
State SX_null                           (NULL,                    NULL,                NULL);
State S0_disconnected                   (&on_enter_disconnected,  NULL,                NULL);
State S1_connecting                     (NULL,                    NULL,                NULL);
State S2_connecting_load                (NULL,                    NULL,                &on_load);
State S3_connected                      (&on_enter_connected,     NULL,                NULL);
State S4_command_queued                 (NULL,                    NULL,                NULL);
State S5_command_issued                 (&on_data,                NULL,                NULL);
State S6_disconnect_queued              (NULL,                    NULL,                NULL);
State S7_disconnecting                  (NULL,                    NULL,                NULL);

State S10_running                       (NULL,                    NULL,                NULL);  // program is running inactive (Wait)
State S11_running_command_queued        (NULL,                    NULL,                NULL);
State S12_running_command_issued        (&on_data,                NULL,                NULL);
State S13_running_active                (NULL,                    NULL,                NULL);  // program is running active (Command)
State S14_running_active_command_queued (NULL,                    NULL,                NULL);
State S15_running_active_command_issued (&on_data,                NULL,                NULL);


Fsm fsm(&SX_null);

std::queue<Event> eventQueue;

void advertise()
{
  esp.advertise("debug").setDatatype("string");
  esp.advertise("message").setDatatype("string");
  esp.advertise("error").setDatatype("string");

  raw.advertise("in").setName("RawIn").setDatatype("string");
  raw.advertise("out").setName("RawOut").setDatatype("string");

  ebc.advertise("connection").setName("Connection").setDatatype("enum").setUnit("on,off").settable(connectionHandler);
  ebc.advertise("model").setName("Model").setDatatype("string");
  ebc.advertise("mode").setName("Mode").setDatatype("string");
  ebc.advertise("response").setName("Response").setDatatype("string").setFormat("text/json");
  ebc.advertise("voltage").setName("Voltage").setDatatype("float").setUnit("V");
  ebc.advertise("current").setName("Current").setDatatype("float").setUnit("A");
  ebc.advertise("capacity").setName("Capacity").setDatatype("float").setUnit("Ah");

  cpu.advertise("program").setDatatype("string").setFormat("text/json").settable(cpuProgramLoadHandler);
  cpu.advertise("run").setDatatype("enum").setUnit("on,off").settable(cpuProgramRunHandler);
  cpu.advertise("state").setDatatype("enum").setUnit("idle,loaded,running,stopped,end");
  cpu.advertise("step").setDatatype("integer");
  cpu.advertise("result").setDatatype("string").setFormat("text/json");
}


void Logging(Logger::LogSeverity l, const char* msg)
{
  switch (l)
  {
    case Logger::Debug:
      esp.setProperty("debug").send(msg);
      break;
    case Logger::Message:
      esp.setProperty("message").send(msg);
      break;
    case Logger::Error:
      esp.setProperty("error").send(msg);
      break;
    default:
      break;
  }
}

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::MQTT_READY:
        eventQueue.push(Evt_init);
      break;
  }
}

bool send(const Command& cmd)
{
  if (cmd.Send(ebcSerial)) {
    // ebcSerial.flush();
    raw.setProperty("out").send(cmd.ToHexString());
    return true;
  }
  return false;
}

void ebcSendProperty(const char* name, const String& value)
{
  uint16_t packetId = ebc.setProperty(name).send(value);
  if (packetId == 0) {
    Logger::LogE(String(F("ebc: Cannot send property ")) + String(name) + F(" (") + value + F(")"));
  }
}

bool connectionHandler(const HomieRange& range, const String& value)
{
  if (value == "on") {
    eventQueue.push(Evt_connect);
    ebcSendProperty("connection", value);
  } else
  if (value == "off") {
    eventQueue.push(Evt_disconnect);
  } else {
    return false;
  }

  return true;
}

bool cpuCommandHandler(const Command& cmd)
{
  if (cmd.GetCommand() == Command::InvalidCommand) {
    Logger::LogE("command is invalid");
    return false;
  }
  nextCommand = cmd;
  eventQueue.push(Evt_command);
  return true;
}

bool cpuReportHandler (const String& key, const String& value)
{
  uint16_t packetId = cpu.setProperty(key).send(value);
  if (packetId == 0) {
    Logger::LogE(String(F("cpu: Cannot send property key: ")) + key);
    Logger::LogE(String(F("cpu: Cannot send property value: ")) + value);
    return false;
  }
  return true;
}

void cpuEventHandler(Processor::CpuEvent e)
{
  switch (e) {
    case Processor::CpuEvent::Cpu_Command_Finished:
      eventQueue.push(Evt_command_finished);
      break;
    case Processor::CpuEvent::Cpu_Program_End:
      eventQueue.push(Evt_end);
      break;
    default:
      break;
  }
}

bool cpuProgramLoadHandler(const HomieRange& range, const String& value)
{
  cpuProgramLoadPending = value;
  eventQueue.push(Evt_load);
  return true;
}

bool cpuProgramRunHandler(const HomieRange& range, const String& value)
{
  if (value == "on") {
    eventQueue.push(Evt_run);
  } else
  if (value == "off") {
    eventQueue.push(Evt_stop);
  } else {
      return false;
  }
  return true;
}

void on_initialize() {
  esp.setProperty("debug").send("");
  esp.setProperty("message").send("");
  esp.setProperty("error").send("");

  // on_enter_disconnected() <-- this will be executed by sure in the next step
  ebcSendProperty("response", "{}"); // needs to be a json object!

  cpuReportHandler("run", "off");
  cpuReportHandler("state", "idle");
  cpuReportHandler("program", "{}");  // needs to be a json object!
  cpuReportHandler("result", "[]");  // needs to be a json array!
}

void on_enter_disconnected() {
  ebcSendProperty("connection", "off");
  ebcSendProperty("mode", "");
  ebcSendProperty("model", "");
  ebcSendProperty("voltage", "0.0");
  ebcSendProperty("current", "0.0");
  ebcSendProperty("capacity", "0.0");
  Homie.setIdle(true);
}

void on_enter_connected() {
    ebcSendProperty("connection", "on");
    Homie.setIdle(false);
}

void on_connect() {
  // send connect
  send(EbcController::GetController().CreateConnect());
}
void on_disconnect() {
  // send disconnect
  send(EbcController::GetController().CreateDisconnect());
}

void on_inject_data() {
  on_data();
  // push the response to the running processor
  if (controller->IsValidResponseForCommand(activeCommand.GetCommand())) {
    if (processor.IsRunning()) {
      processor.InjectData(*controller);
    }
  }
}

void on_first_data() {
  ebcSendProperty("model", controller->GetModel());
  ebcSendProperty("voltage", String(store.GetValue(ParameterName::voltageV), 3));
  ebcSendProperty("current", String(store.GetValue(ParameterName::currentA), 3));
  ebcSendProperty("capacity", String(store.GetValue(ParameterName::capacityAh), 3));
}

void on_data() {
  if (store.HasChanged(ParameterName::voltageV))
    ebcSendProperty("voltage", String(store.GetValue(ParameterName::voltageV), 3));
  if (store.HasChanged(ParameterName::currentA))
    ebcSendProperty("current", String(store.GetValue(ParameterName::currentA), 3));
  if (store.HasChanged(ParameterName::capacityAh))
    ebcSendProperty("capacity", String(store.GetValue(ParameterName::capacityAh), 3));
}

void on_command() {
  // send command
  send(nextCommand);
  activeCommand = nextCommand;
  Logger::LogD(String(F("command ")) + String(activeCommand.GetCommandStr()) + F(" started"));
}

void on_command_finished() {
  Logger::LogD(String(F("command ")) + String(activeCommand.GetCommandStr()) + F(" finished"));
}

void on_load() {
  // load program into processor
  if (!cpuProgramLoadPending.isEmpty())
  {
    // EbcController& controller = EbcController::GetController(response);
    processor.Load(*controller, cpuProgramLoadPending);
    cpuProgramLoadPending.clear();
  }
}

void on_run() {
  // run the program
  processor.Run();
}

void on_stop() {
  // stop the program if running
  processor.Stop();
}


void readFromController() {
  // read input
  if (response.Read(ebcSerial)) {
    raw.setProperty("in").send(response.ToHexString());
    controller = &EbcController::GetController(response);

    if (controller->IsValidResponseForCommand(activeCommand.GetCommand())) {
      store.Push(controller->GetResponseParameters());
      ebcSendProperty("mode", controller->ModeAsString());
      ebcSendProperty("response", controller->GetResponseJson());
      //eventQueue.push(Evt_data);
      eventQueue.push(Evt_response);
    } else
    if (controller->IsValidData()) {
      eventQueue.push(Evt_data);
    } else {
    }
  }
}

void setFsm() {
// note:
  // commands are not send immediate to the ebc charger.
  // they are queued and send directly after the next data pdu from the charger to avoid
  // collisions between send and receive on the UART interface. the charger seems to NOT support
  // full duplex communication.

  // FSM on normal operation
  fsm.add_transition(&SX_null, &S0_disconnected, Evt_init, &on_initialize);
  fsm.add_timed_transition(&SX_null, &S0_disconnected, 5000, &on_initialize); // if Evt_init doesn't fire

  fsm.add_transition(&S0_disconnected, &S3_connected, Evt_data, NULL);
  fsm.add_transition(&S0_disconnected, &S3_connected, Evt_response, &on_first_data);
  fsm.add_transition(&S0_disconnected, &S1_connecting, Evt_connect, &on_connect);
  fsm.add_timed_transition(&S1_connecting, &S1_connecting, 3000, &on_connect);
  fsm.add_transition(&S1_connecting, &S3_connected, Evt_response, &on_first_data);
  fsm.add_transition(&S1_connecting, &S0_disconnected, Evt_disconnect, &on_disconnect);
  
  fsm.add_transition(&S3_connected, &S4_command_queued, Evt_command, NULL);
  fsm.add_transition(&S4_command_queued, &S5_command_issued, Evt_data, &on_command);
  fsm.add_transition(&S4_command_queued, &S5_command_issued, Evt_response, &on_command);
  fsm.add_timed_transition(&S5_command_issued, &S4_command_queued, 3000, &on_command);
  fsm.add_transition(&S5_command_issued, &S3_connected, Evt_response, &on_data);
  fsm.add_transition(&S3_connected, &S3_connected, Evt_response, &on_data);
  fsm.add_transition(&S3_connected, &S3_connected, Evt_load, &on_load);
  fsm.add_transition(&S3_connected, &S10_running, Evt_run, &on_run);

  fsm.add_transition(&S10_running, &S3_connected, Evt_stop, &on_stop);

  fsm.add_transition(&S10_running, &S11_running_command_queued, Evt_command, NULL);
  fsm.add_transition(&S11_running_command_queued, &S12_running_command_issued, Evt_data, &on_command);
  fsm.add_transition(&S11_running_command_queued, &S12_running_command_issued, Evt_response, &on_command);
  fsm.add_timed_transition(&S12_running_command_issued, &S11_running_command_queued, 3000, &on_command);
  fsm.add_transition(&S12_running_command_issued, &S13_running_active, Evt_response, &on_inject_data);

  fsm.add_transition(&S10_running, &S3_connected, Evt_end, NULL);
  fsm.add_transition(&S11_running_command_queued, &S4_command_queued, Evt_end, NULL);
  fsm.add_transition(&S12_running_command_issued, &S5_command_issued, Evt_end, NULL);

  fsm.add_transition(&S13_running_active, &S13_running_active, Evt_response, &on_inject_data);
  fsm.add_transition(&S13_running_active, &S10_running, Evt_command_finished, &on_command_finished);
  fsm.add_transition(&S13_running_active, &S3_connected, Evt_stop, &on_stop);
  fsm.add_transition(&S13_running_active, &S6_disconnect_queued, Evt_disconnect, &on_stop); // this can trigger a stop command

  fsm.add_transition(&S13_running_active, &S14_running_active_command_queued, Evt_command, NULL);
  fsm.add_transition(&S14_running_active_command_queued, &S15_running_active_command_issued, Evt_data, &on_command);
  fsm.add_transition(&S14_running_active_command_queued, &S15_running_active_command_issued, Evt_response, &on_command);
  fsm.add_timed_transition(&S15_running_active_command_issued, &S14_running_active_command_queued, 3000, &on_command);
  fsm.add_transition(&S15_running_active_command_issued, &S13_running_active, Evt_response, &on_inject_data);
  
  fsm.add_transition(&S13_running_active, &S3_connected, Evt_end, NULL);
  fsm.add_transition(&S14_running_active_command_queued, &S4_command_queued, Evt_end, NULL);
  fsm.add_transition(&S15_running_active_command_issued, &S5_command_issued, Evt_end, NULL);
  
  fsm.add_transition(&S3_connected, &S3_connected, Evt_stop, &on_stop);
  fsm.add_transition(&S3_connected, &S3_connected, Evt_end, NULL);
  fsm.add_transition(&S3_connected, &S6_disconnect_queued, Evt_disconnect, &on_stop); // this can trigger a stop command
  fsm.add_transition(&S6_disconnect_queued, &S7_disconnecting, Evt_response, &on_disconnect);
  fsm.add_transition(&S6_disconnect_queued, &S6_disconnect_queued, Evt_command, &on_command); // we have to handle the stop command
  fsm.add_timed_transition(&S7_disconnecting, &S0_disconnected, 3000, NULL);
  fsm.add_transition(&S7_disconnecting, &S7_disconnecting, Evt_response, &on_disconnect);

  // FSM on load only operation
  fsm.add_transition(&S0_disconnected, &S2_connecting_load, Evt_load, &on_connect);
  fsm.add_timed_transition(&S2_connecting_load, &S2_connecting_load, 3000, &on_connect);
  fsm.add_transition(&S2_connecting_load, &S7_disconnecting, Evt_response, &on_disconnect);
}


// called by Homie if normal operation mode enters
void normalModeSetup() {

  Serial.begin(9600);
#ifdef ESP8266
  ebcSerial.begin(9600, SWSERIAL_8O1);
#else
  ebcSerial.begin(9600, SERIAL_8O1);
#endif
  delay(200);
  Logger::GetInstance().SetLogger(Logging);
  processor.SetCommmander(cpuCommandHandler);
  processor.SetReportHandler(cpuReportHandler);
  processor.SetEventHandler(cpuEventHandler);
}

// called periodically by Homie in normal operation mode
void normalModeLoop() {
  readFromController();
  if (!eventQueue.empty()) {
    fsm.trigger(eventQueue.front());
    eventQueue.pop();
  }
  fsm.run_machine();
  processor.Tick();
}


void setup() {
  Homie_setFirmware(firmwareName, firmwareVersion);
  Homie.setSetupFunction(normalModeSetup);
  Homie.setLoopFunction(normalModeLoop);
  // Homie.disableLedFeedback();
  Homie.disableLogging();

  advertise();
  setFsm();

  Homie.onEvent(onHomieEvent);
  Homie.setup();
}

void loop() {
  Homie.loop();
}

