// LCC WiFi network parameters
const char* G_NetworkName = "Sarntalbahn";
const char* G_NetworkPW = "locodriver";
const char* G_CSIP  = "10.0.0.1";
const uint16_t G_Port = 12021;

// Simulation data
const uint8_t G_ClockRate = 10;
const float G_TimeDepartureReady = 3.f;
const float G_TimeTransitMainline = 5.f * 60.f;
const float G_TimeTransitOutboundBlock = 5.f;
const float G_TimeTransitInboundBlock = 5.f;
const float G_TimeTransitSwitchBlock = 5.f;
const float G_TimeBlockExit = 5.f;

// LCC Event IDs
const char* G_Event_OutSignalClear = "55.BB.00.11.22.33.00.00";
const char* G_Event_OutSignalStop = "55.BB.00.11.22.33.00.01";
const char* G_Event_InSignalClear = "55.BB.00.11.22.33.00.02";
const char* G_Event_InSignalStop = "55.BB.00.11.22.33.00.03";
const char* G_Event_BlockMainOccupied = "55.BB.00.11.22.33.00.04";
const char* G_Event_BlockMainClear = "55.BB.00.11.22.33.00.05";
const char* G_Event_BlockSwitchOccupied = "55.BB.00.11.22.33.00.06";
const char* G_Event_BlockSwitchClear = "55.BB.00.11.22.33.00.07";
const char* G_Event_BlockOutOccupied = "55.BB.00.11.22.33.00.08";
const char* G_Event_BlockOutClear = "55.BB.00.11.22.33.00.09";
const char* G_Event_BlockInOccupied = "55.BB.00.11.22.33.00.10";
const char* G_Event_BlockInClear = "55.BB.00.11.22.33.00.11";
const char* G_Event_ActualBlockEntered = "02.01.57.11.00.CA.00.2D";
const char* G_LCC_SourceAlias = "5b2";