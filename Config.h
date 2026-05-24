#include <SPI.h>
#include <SD.h>

struct NetworkSettings
{
  const char* name = "Sarntalbahn";
  const char* password = "locodriver";
  const char* hostAddress  = "10.0.0.1";
  uint16_t hostPort = 12021;
};

struct HardwareSettings
{
  float timeLEDBlink = 0.2f;
  float timeBuzzerOn = 0.4f;
  float timeBuzzerDelay = 0.2f;
  float sensorMinTime = 0.1f;
  int departBuzzerBeats = 2;
  int buzzerTone = 1500;
  int sensorThreshold = 30;
};

struct SimulationSettings
{
  uint8_t clockRate = 10;
  float timeDepartureReady = 3.f;
  float timeTransitMainline = 5.f * 60.f;
  float timeTransitOutboundBlock = 5.f;
  float timeTransitInboundBlock = 5.f;
  float timeTransitSwitchBlock = 5.f;
  float timeBlockExit = 5.f;
};

struct LCCSettings
{
  const char* event_outSignalClear = "55.BB.00.11.22.33.00.00";
  const char* event_outSignalStop = "55.BB.00.11.22.33.00.01";
  const char* event_inSignalClear = "55.BB.00.11.22.33.00.02";
  const char* event_inSignalStop = "55.BB.00.11.22.33.00.03";
  const char* event_blockMainOccupied = "55.BB.00.11.22.33.00.04";
  const char* event_blockMainClear = "55.BB.00.11.22.33.00.05";
  const char* event_blockSwitchOccupied = "55.BB.00.11.22.33.00.06";
  const char* event_blockSwitchClear = "55.BB.00.11.22.33.00.07";
  const char* event_blockOutOccupied = "55.BB.00.11.22.33.00.08";
  const char* event_blockOutClear = "55.BB.00.11.22.33.00.09";
  const char* event_blockInOccupied = "55.BB.00.11.22.33.00.10";
  const char* event_blockInClear = "55.BB.00.11.22.33.00.11";
  const char* event_actualBlockEntered = "02.01.57.11.00.CA.00.2D";
  const char* event_distSigAspectStop = "55.BB.00.11.22.33.00.12";
  const char* event_distSigAspectClear1 = "55.BB.00.11.22.33.00.13";
  const char* event_distSigAspectClear2 = "55.BB.00.11.22.33.00.14";
  const char* sourceAlias = "5b2";
};

struct ConfigSettings
{
  NetworkSettings network;
  HardwareSettings hardware;
  SimulationSettings simulation;
  LCCSettings lcc;

  void ReadFromSD(String path)
  {
    if (!SD.exists(path))
      return;

    File cfgFile = SD.open(path);
    if (cfgFile)
    {
      String param;
      String value;
      bool readValue = false;
      while (cfgFile.available())
      {
        char c = cfgFile.read();
        if (c == '=')
          readValue = true;
        else if (c == ';')
        {
          param.trim();
          value.trim();
          Parse(param, value);
        }
        else
        {
          if (readValue)
            value += c;
          else
            param += c;
        }
      }

      cfgFile.close();
    }
  }

private:
  void AssignString(const char*& par, String value)
  {
    par = value.c_str();
  }
  void ParseInt(int& par, String value)
  {
    par = value.toInt();
  }
  void ParseFloat(float& par, String value)
  {
    par = value.toFloat();
  }

  void Parse(String param, String value)
  {
    #define QUOTE(seq) #seq
    #define PARSEF(func, par) \
      Serial.print(param); \
      Serial.print("=="); \
      Serial.println(QUOTE(par)); \
      if (param == QUOTE(par)) \
        func(par, value);

    #define PARSEINT(par) \
      if (param == QUOTE(par)) \
      { \
        int v = par; \
        ParseInt(v, value); \
        par = v; \
      }
  
    PARSEF(AssignString, network.name)
    PARSEF(AssignString, network.password)
    PARSEF(AssignString, network.hostAddress)
    PARSEINT(network.hostPort)
    PARSEF(ParseFloat, hardware.timeLEDBlink)
    PARSEF(ParseFloat, hardware.timeBuzzerOn)
    PARSEF(ParseFloat, hardware.timeBuzzerDelay)
    PARSEF(ParseFloat, hardware.sensorMinTime)
    PARSEF(ParseInt, hardware.departBuzzerBeats)
    PARSEF(ParseInt, hardware.buzzerTone)
    PARSEF(ParseInt, hardware.sensorThreshold)
    PARSEINT(simulation.clockRate)
    PARSEF(ParseFloat, simulation.timeDepartureReady)
    PARSEF(ParseFloat, simulation.timeTransitMainline)
    PARSEF(ParseFloat, simulation.timeTransitOutboundBlock)
    PARSEF(ParseFloat, simulation.timeTransitInboundBlock)
    PARSEF(ParseFloat, simulation.timeTransitSwitchBlock)
    PARSEF(ParseFloat, simulation.timeBlockExit)
    PARSEF(AssignString, lcc.event_outSignalClear)
    PARSEF(AssignString, lcc.event_outSignalStop)
    PARSEF(AssignString, lcc.event_inSignalClear)
    PARSEF(AssignString, lcc.event_inSignalStop)
    PARSEF(AssignString, lcc.event_blockMainOccupied)
    PARSEF(AssignString, lcc.event_blockMainClear)
    PARSEF(AssignString, lcc.event_blockSwitchOccupied)
    PARSEF(AssignString, lcc.event_blockSwitchClear)
    PARSEF(AssignString, lcc.event_blockOutOccupied)
    PARSEF(AssignString, lcc.event_blockOutClear)
    PARSEF(AssignString, lcc.event_blockInOccupied)
    PARSEF(AssignString, lcc.event_blockInClear)
    PARSEF(AssignString, lcc.event_actualBlockEntered)
    PARSEF(AssignString, lcc.event_distSigAspectStop)
    PARSEF(AssignString, lcc.event_distSigAspectClear1)
    PARSEF(AssignString, lcc.event_distSigAspectClear2)
    PARSEF(AssignString, lcc.sourceAlias)
  }
};

ConfigSettings G_Config;