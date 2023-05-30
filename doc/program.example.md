
# example 1

{
    "name": "Example-1",
    "steps":
    [
        {
            "command": "C-CV",
            "parameters":
            {
                "voltageV": 4.1,
                "currentA": 0.35,
                "cutoffA": 0.1
            }
        },
        {
            "command": "D-CC",
            "parameters":
            {
                "cutoffV": 3.81,
                "currentA": 0.35
            }
        },
        {
            "command": "C-CV",
            "parameters":
            {
                "voltageV": 4.1,
                "currentA": 0.35,
                "cutoffA": 0.1
            },
            "stopCondition":
            {
                "capacityAh": 0.6
            }
        }
    ]
}

# example 2

{
    "name": "Example-2",
    "steps":
    [
        {
            "command": "Wait",
            "seconds": 20
        },
        {
            "command": "Cycle",
            "step": 0,
            "count": 3
        },
        {
            "command": "Wait",
            "minutes": 2
        }
    ]
}

# example 3

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

# command example D-CP

{
  "command": "D-CP",
  "parameters":
  {
    "cutoffV": 3.81,
    "powerW": 1
  }
}

# command example C-CV

{
  "command": "C-CV",
  "parameters":
  {
    "voltageV": 4.1,
    "currentA": 0.35,
    "cutoffA": 0.1
  }
}
