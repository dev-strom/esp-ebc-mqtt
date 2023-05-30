# EBC-MQTT-Controller

A firmware for the ESP-32 microcontroller to act as a gateway between an EBC charger and an MQTT broker. Currently this software supports only an EBC-A20 charger/discharger from ZKETech, but may easily ported for other models.

The software uses the arduino framework and the homie library. So it complies to the homie 3.0.1 specification. The software is written for an ESP-32 microcontroller but may run with limitations on an ESP-8266 as well.

## Compiling

Use [Visual Studio Code](https://code.visualstudio.com/) and the [PlatformIO extension](https://platformio.org/) to compile the source code.

## Wiring

The ESP-32 and the ESP8266 uses different voltages as the EBC chargers. So you have to use a logic level shifter between this components. The ESP uses 3.3V and the EBC uses 5V. Here is an example for an ESP development board, which contains a voltage regulator from 5V to 3.3V.

```text
Power supply   ESP-32      level shifter       EBC (mini USB port / pin, color)

GND ----------- GND  ------ GND -- GND --------- GND (4/5, black)
5V  ----------- 5V   ------------- HV  --------- VCC (1, red)
                3.3V ------ LV
                TX2  ------ LV1    HV1 --------- D-  (2, white)
                RX2  ------ LV2    HV2 --------- D+  (3, green)
```

If you use an ESP development board, you can use the onboard USB port for the 5V power supply.

## Configuration

See [configuration for Homie 3.0.1](https://homieiot.github.io/homie-esp8266/docs/3.0.1/configuration/json-configuration-file/).

## User Interface

This Software comes with no user interface. It is controlled entirly throug the MQTT interface. Use a home automation system of your choice to build your own user interface.
An example of an user interface in node-red can be found in the source code folder [ui/node-red](file://ui/node-red).

## MQTT Topics

### Logging

#### homie/ebc-control/esp/debug

Debug messages.

#### homie/ebc-control/esp/message

Informational messages.

#### homie/ebc-control/esp/error

Error messages.

### Raw data

#### homie/ebc-control/raw/out

Binary dump of every message send from the ESP controller to the EBC charger.

#### homie/ebc-control/raw/in

Binary dump of every message received by the ESP controller from the EBC charger.

### State of the controller

#### homie/ebc-control/controller/connection

Connection state (on or off).

#### homie/ebc-control/controller/model

If connected, the model of the EBC charger.

#### homie/ebc-control/controller/mode

The current operation mode of the EBC charger (charging, discharging, stopped).

#### homie/ebc-control/controller/response

An object, formatted as json string, containing all received values from the EBC charger.

#### homie/ebc-control/controller/voltage

The actual voltage (V).

#### homie/ebc-control/controller/current

The actual current (A).

#### homie/ebc-control/controller/capacity

The calculated capacity (Ah).

### Program

#### homie/ebc-control/cpu/program

The loaded program. This topic can be set by ```homie/ebc-control/cpu/program/set``` and contains the program running on the ESP. The program is formatted as a json string and looks like this example:

```json
{
    "id":"LF280",
    "name":"LiFePo4, 280Ah, 3.60V",
    "steps":
    [
        {
            "command":"C-CV",
            "parameters":
            {
                "voltageV":3.6,
                "currentA":5,
                "cutoffA":0.5
            }
        },
        {
            "command":"D-CC",
            "parameters":
            {
                "cutoffV":2.6,
                "currentA":20
            }
        },
        {
            "command":"C-CV",
            "parameters":
            {
                "voltageV":3.3,
                "currentA":5,
                "cutoffA":0.5
            }
        }
    ]
}
```

This program has three steps.

1. The first step runs the command ***C-CV*** (charge constant voltage) at 3.6V and 5.0A. The cut off current is 0.5A.

2. The second step runs the command ***D-CC*** (discharge constant current) at 20A. The cut off voltage is 2.6V.

3. The third step runs the command ***C-CV*** (charge constant voltage) at 3.3V and 5.0A. The cut off current is 0.5A.

The list of available commands differs and depends on the used EBC charger. There are two commands that are always available:

1. ***Wait***: It performs a pause. It has one parameter ***seconds*** or ***minutes*** which specify the duration.

2. ***Cycle***: It cycles to a previous step. It has two parameters ***step*** and ***count***. ***step*** specifies the step number to jump to (first step has number 0). ***count*** tells how often the backcycle should be performed. ***count*** must be between 0 and 65535.

See section ***Program Commands*** for a list of available commands and parameters.

Remark: loading a program is only possible if an EBC charger is connected. If not in the connected state the gateway tries to connect first, load the program and disconnects at last step. If there is no EBC charger pluged to the gateway, the program loading fails!

#### homie/ebc-control/cpu/run

Run state (on or off).

#### homie/ebc-control/cpu/state

The state of the current program.

#### homie/ebc-control/cpu/step

Actual step of a running program.

#### homie/ebc-control/cpu/result

The results of the program. This topic is updated after each program step and contains the results of all finished steps.

Example:

```json
[
    {"step":0,"command":"C-CV","capacityAh":199.01},
    {"step":1,"command":"D-CC","capacityAh":267.2},
    {"step":2,"command":"C-CV","capacityAh":68.48}
]
```

## Program Commands

| Controller | Command | Parameters                   |
| ---------- | ------- | ---------------------------- |
| all        | Wait    | seconds or minutes           |
| all        | Cycle   | step, count                  |
| EBC-A20    | C-CV    | currentA, voltageV, cutoffA  |
| EBC-A20    | D-CC    | currentA, cutoffV,  maxTimeM |
| EBC-A20    | D-CP    | powerW,   cutoffV,  maxTimeM |

### Additional stop condition

This software has support for additional stop conditions on every program step.

Here is a full program example:

```json
{
  "name": "Li-Ion, 700mAh, 3.7V, 70%",
  "steps": [
    {
      "command": "C-CV",
      "parameters": {
        "voltageV": 4.2,
        "currentA": 0.35,
        "cutoffA": 0.1
      }
    },
    {
      "command": "D-CC",
      "parameters": {
        "cutoffV": 2.75,
        "currentA": 0.35
      }
    },
    {
      "command": "C-CV",
      "parameters": {
        "voltageV": 4.2,
        "currentA": 0.35,
        "cutoffA": 0.1
      },
      "stopCondition": {
        "capacityAh": "70%"
      }
    }
  ]
}
```

The last step stops if the capacity reaches 70% of the previous step. So this program do charge the cell to 100% SOC, than it discharges it to 0% SOC and measures the full capacity of the cell. In the last step it charges the cell to 70% of it's capacity.

The stop condition ***capacityAh*** can be relativ (percent) like in this example or it can be an absolute value in Ah like ```"capacityAh": 0.65```.

## Porting to other chargers

This software supports only the EBC-A20 charger but can be expanded to support more chargers from ZKETech. To do so, you only have to create a copy of the files ```EbcA20.hpp``` and ```EbcA20.cpp``` and modify them to meet the protocol of your desired charger. Expand the method ```EbcController::GetController(uint8_t id)``` in ```EbcController.hpp``` as well.
