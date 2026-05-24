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
};

ConfigSettings G_Config;