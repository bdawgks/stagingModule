#include "Config.h"
#include "Timer.h"
#include "Event.h"
#include "Display.h"

#include <WiFi.h>

#define PIN_RDY_LED   D2
#define PIN_RDY_BTN   D3
#define PIN_BUZZER    D4
#define PIN_SENSOR    A1

enum LEDPhase
{
  LED_Off,
  LED_On,
  LED_FlashOn,
  LED_FlashOff
};

enum TransitState
{
  TS_idle,
  TS_departing,
  TS_arriving
};

enum ProducedEventEnum
{
  PE_outBlockOccupied,
  PE_outBlockClear,
  PE_inBlockOccupied,
  PE_inBlockClear,
  PE_switchBlockOccupied,
  PE_switchBlockClear,
  PE_mainBlockOccupied,
  PE_mainBlockClear
};

LEDPhase readyLEDphase;
int readyButtonValue;
int buzzerBeats = 0;

ProducedEvent producedEvents[8];
ConsumedEvent outSignalClearEvent(G_Event_OutSignalClear);
ConsumedEvent outSignalStopEvent(G_Event_OutSignalStop);
ConsumedEvent inSignalClearEvent(G_Event_InSignalClear);
ConsumedEvent inSignalStopEvent(G_Event_InSignalStop);
ConsumedEvent physicalBlockEnteredEvent(G_Event_ActualBlockEntered);
ConsumedEvent distantAspectStopEvent(G_Event_DistSigAspectStop);
ConsumedEvent distantAspectClear1Event(G_Event_DistSigAspectClear1);
ConsumedEvent distantAspectClear2Event(G_Event_DistSigAspectClear2);

TransitState currentState = TS_idle;

bool outboundTrainReady = false;
bool inboundSignalClear = false;
bool outboundSignalClear = false;
bool trainWaitingAtInboundSignal = false;
bool outboundReadyBlocked = false;

// Timers for departure
Timer timerOutboundDepartureReady(G_TimeDepartureReady);
Timer timerOutboundEnterSwitch(G_TimeTransitOutboundBlock);
Timer timerOutboundExitOutBlock(G_TimeTransitOutboundBlock + G_TimeBlockExit);
Timer timerOutboundEnterMain(G_TimeTransitSwitchBlock);
Timer timerOutboundExitSwitch(G_TimeTransitSwitchBlock + G_TimeBlockExit);
Timer timerOutboundTransitMain(G_TimeTransitMainline / (float)G_ClockRate);
Timer timerOutboundExitMain(G_TimeBlockExit);

// Timers for arrival
Timer timerInboundTransitMain(G_TimeTransitMainline / (float)G_ClockRate);
Timer timerInboundEnterSwitch(G_TimeDepartureReady);
Timer timerInboundExitMain(G_TimeBlockExit);
Timer timerInboundEnterInBlock(G_TimeTransitSwitchBlock);
Timer timerInboundExitSwitch(G_TimeTransitSwitchBlock + G_TimeBlockExit);
Timer timerInboundExitInBlock(G_TimeTransitInboundBlock + G_TimeBlockExit);

// Misc timers
Timer timerReadyLED(G_TimeLEDBlink);
Timer timerBuzzerOn(G_TimeBuzzerOn);
Timer timerBuzzerDelay(G_TimeBuzzerDelay);
Timer timerSensor(G_SensorMinTime);

Timer* allTimers[] =
{
  &timerOutboundDepartureReady, &timerOutboundEnterSwitch, &timerOutboundExitOutBlock, &timerOutboundEnterMain, &timerOutboundExitSwitch,
  &timerOutboundTransitMain, &timerOutboundExitMain,
  &timerInboundTransitMain, &timerInboundEnterSwitch, &timerInboundExitMain, &timerInboundEnterInBlock, &timerInboundExitSwitch,
  &timerInboundExitInBlock,
  &timerReadyLED, &timerBuzzerOn, &timerBuzzerDelay, &timerSensor
};
int numTimers = 17;

// Display
Display display;
DistantAspect distantAspect = DA_Caution;

WiFiClient client;

void setup() 
{
  pinMode(PIN_RDY_LED, OUTPUT);
  pinMode(PIN_RDY_BTN, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);

  display.Init();

  Serial.begin(115200);
  
  // Establish WiFi connection
  Serial.println("Connecting:");

  WiFi.begin(G_NetworkName, G_NetworkPW);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to network");

  
  if (client.connect(G_CSIP, G_Port))
  {
    Serial.println("Connected to host");
  }

  initTimers();

  producedEvents[PE_outBlockOccupied].Init(G_Event_BlockOutOccupied, G_LCC_SourceAlias);
  producedEvents[PE_outBlockClear].Init(G_Event_BlockOutClear, G_LCC_SourceAlias);
  producedEvents[PE_inBlockOccupied].Init(G_Event_BlockInOccupied, G_LCC_SourceAlias);
  producedEvents[PE_inBlockClear].Init(G_Event_BlockInClear, G_LCC_SourceAlias);
  producedEvents[PE_switchBlockOccupied].Init(G_Event_BlockSwitchOccupied, G_LCC_SourceAlias);
  producedEvents[PE_switchBlockClear].Init(G_Event_BlockSwitchClear, G_LCC_SourceAlias);
  producedEvents[PE_mainBlockOccupied].Init(G_Event_BlockMainOccupied, G_LCC_SourceAlias);
  producedEvents[PE_mainBlockClear].Init(G_Event_BlockMainClear, G_LCC_SourceAlias);

  display.DrawStopSign();
}

void initTimers()
{
  timerOutboundExitOutBlock.OnCompleted(completedOutboundExitOutBlock);
  timerOutboundEnterSwitch.OnCompleted(completedOutboundEnterSwitch);
  timerOutboundExitSwitch.OnCompleted(completedOutboundExitSwitch);
  timerOutboundEnterMain.OnCompleted(completedOutboundEnterMain);
  timerOutboundTransitMain.OnCompleted(completedOutboundTransitMain);
  timerOutboundExitMain.OnCompleted(completedOutboundExitMain);

  timerInboundTransitMain.OnCompleted(completedInboundTransitMain);
  timerInboundEnterSwitch.OnCompleted(completedInboundEnterSwitch);
  timerInboundExitMain.OnCompleted(completedInboundExitMain);
  timerInboundEnterInBlock.OnCompleted(completedInboundEnterInBlock);
  timerInboundExitSwitch.OnCompleted(completedInboundExitSwitch);
  timerInboundExitInBlock.OnCompleted(completedInboundExitInBlock);

  timerReadyLED.OnCompleted(completedReadyLED);
  timerReadyLED.Start();

  timerBuzzerOn.OnCompleted(completedBuzzerOn);
  timerBuzzerDelay.OnCompleted(completedBuzzerDelay);

  timerSensor.OnCompleted(completedSensor);
}

void loop() 
{
  if (analogRead(PIN_SENSOR) < G_SensorThreshold)
  {
    if (!timerSensor.IsRunning())
      timerSensor.Start();
  }
  else
  {
    if (timerSensor.IsRunning())
      timerSensor.Reset();
  }

  if (digitalRead(PIN_RDY_BTN) != readyButtonValue)
  {
    readyButtonValue = digitalRead(PIN_RDY_BTN);
    if (readyButtonValue == LOW)
      setDepartureReady(!outboundTrainReady);
  }

  if (readyLEDphase == LED_On || readyLEDphase == LED_FlashOn)
    digitalWrite(PIN_RDY_LED, HIGH);
  else
    digitalWrite(PIN_RDY_LED, LOW);

  clientRead();

  simulate(100);

  if (Serial.available())
  {
    String input = Serial.readString();
    input.trim();
    if (input == "dep_ready")
    {
      setDepartureReady(!outboundTrainReady);
    }
    else if (input == "dep_clear")
    {
      setDepartureSignal(!outboundSignalClear);
    }
    else if (input == "enter_block")
    {
      onTrainDeparted();
    }
    else if (input == "arrive_clear")
    {
      setArrivalSignal(!inboundSignalClear);
    }
    else if (input = "pass_sensor")
    {
      onTrainArriving();
    }
  }
}

void startBuzzer(bool restart = false)
{
  if (restart)
    buzzerBeats = G_DepartBuzzerBeats;

  tone(PIN_BUZZER, G_BuzzerTone);
  timerBuzzerOn.Restart();
}

void clientRead()
{
  if (client.available()) 
  {
    String data = client.readStringUntil('\r');

    if (outSignalClearEvent.IsInMessage(data))
    {
      setDepartureSignal(true);
    }
    else if (outSignalStopEvent.IsInMessage(data))
    {
      setDepartureSignal(false);
    }
    else if (inSignalClearEvent.IsInMessage(data))
    {
      setArrivalSignal(true);
    }
    else if (inSignalStopEvent.IsInMessage(data))
    {
      setArrivalSignal(false);
    }
    else if (physicalBlockEnteredEvent.IsInMessage(data))
    {
      onTrainDeparted();
    }
    else if (distantAspectStopEvent.IsInMessage(data))
    {
      updateDistantAspect(DA_Caution);
    }
    else if (distantAspectClear1Event.IsInMessage(data))
    {
      updateDistantAspect(DA_Clear1);
    }
    else if (distantAspectClear2Event.IsInMessage(data))
    {
      updateDistantAspect(DA_Clear2);
    }
  }
}

void simulate(int ms)
{
  checkArrival();
  checkDeparture();

  delay(ms);

  // Update timers
  float seconds = (float)ms / 1000.f;
  for (int i = 0; i < numTimers; i++)
  {
    allTimers[i]->Update(seconds);
  }

  // Show outbound timer on display
  if (currentState == TS_departing)
  {
    if (timerOutboundTransitMain.Completed())
    {
      display.DrawTimerText("GO");
    }
    else if (timerOutboundTransitMain.IsRunning())
    {
      int timeInt = ceil(timerOutboundTransitMain.Get());
      display.DrawTimer(timeInt);
    }
    else
    {
      display.DrawTimerText("WAIT");
    }
  }
  else if (outboundTrainReady)
  {
    display.DrawTimerText("WAIT");
  }
  else
  {
    display.ClearTimer();
  }
}

void updateDistantAspect(DistantAspect aspect)
{
  DistantAspect prevAspect = distantAspect;
  distantAspect = aspect;
  if (prevAspect != distantAspect && currentState == TS_departing && timerOutboundTransitMain.Completed())
  {
    showDistantSignal();
  }
}

void showDistantSignal()
{
  display.DrawDistantSignal(distantAspect);
}

// Handle train ready on the outbound track departing when signal is cleared
void checkDeparture()
{
  if (currentState == TS_idle && outboundTrainReady && timerOutboundDepartureReady.Completed() && outboundSignalClear)
  {
    outboundTrainReady = false;
    currentState = TS_departing;
    timerOutboundEnterSwitch.Restart();
    timerOutboundExitOutBlock.Restart();
    readyLEDphase = LED_FlashOff;
    outboundReadyBlocked = true;
  }
}

// Handle train waiting at the inbound signal arriving when signal is cleared
void checkArrival()
{
  if (currentState == TS_arriving && trainWaitingAtInboundSignal && inboundSignalClear)
  {
    trainWaitingAtInboundSignal = false;
    timerInboundEnterSwitch.Restart();
  }
}

void trainPassingArrivalSignal()
{
    sendEvent(PE_switchBlockOccupied);
    timerInboundExitMain.Restart();
    timerInboundEnterInBlock.Restart();
    timerInboundExitSwitch.Restart();
}

void setDepartureSignal(bool isClear)
{
  outboundSignalClear = isClear;
}

void setArrivalSignal(bool isClear)
{
  inboundSignalClear = isClear;
}

void setDepartureReady(bool isReady)
{
  if (outboundReadyBlocked)
    return;

  outboundTrainReady = isReady;
  if (outboundTrainReady)
  {
    timerOutboundDepartureReady.Restart();
    sendEvent(PE_outBlockOccupied);
    readyLEDphase = LED_On;
  }
  else
  {
    timerOutboundDepartureReady.Reset();
    sendEvent(PE_outBlockClear);
    readyLEDphase = LED_Off;
  }
}

void onTrainDeparted()
{
  if (currentState == TS_departing)
    timerOutboundExitMain.Restart();
}

void onTrainArriving()
{
  if (currentState != TS_idle)
    return;

  currentState = TS_arriving;
  sendEvent(PE_mainBlockOccupied);
  timerInboundTransitMain.Restart();
}

void sendEvent(ProducedEventEnum event)
{
  String eventMsg = producedEvents[event].GetMessageString();

  if (client.connected())
  {
    client.println(eventMsg);
  }
}

// Timer OnCompleted callbacks
void completedOutboundExitOutBlock(Timer& timer)
{
  sendEvent(PE_outBlockClear);
}
void completedOutboundEnterSwitch(Timer& timer)
{
  sendEvent(PE_switchBlockOccupied);
  timerOutboundEnterMain.Restart();
  timerOutboundExitSwitch.Restart();
}
void completedOutboundExitSwitch(Timer& timer)
{
  sendEvent(PE_switchBlockClear);
}
void completedOutboundEnterMain(Timer& timer)
{
  sendEvent(PE_mainBlockOccupied);
  timerOutboundTransitMain.Restart();
}
void completedOutboundTransitMain(Timer& timer)
{
  showDistantSignal();
  startBuzzer(true);
}
void completedOutboundExitMain(Timer& timer)
{
  sendEvent(PE_mainBlockClear);
  timerOutboundTransitMain.Reset();
  currentState = TS_idle;
  display.DrawStopSign();
  outboundReadyBlocked = false;
  readyLEDphase = LED_Off;
}
void completedInboundTransitMain(Timer& timer)
{
  if (inboundSignalClear)
  {
    trainPassingArrivalSignal();
  }
  else
  {
    trainWaitingAtInboundSignal = true;
  }
}
void completedInboundEnterSwitch(Timer& timer)
{
  trainPassingArrivalSignal();
}
void completedInboundExitMain(Timer& timer)
{
  sendEvent(PE_mainBlockClear);
  currentState = TS_idle;
}
void completedInboundEnterInBlock(Timer& timer)
{
  sendEvent(PE_inBlockOccupied);
  timerInboundExitInBlock.Restart();
}
void completedInboundExitSwitch(Timer& timer)
{
  sendEvent(PE_switchBlockClear);
}
void completedInboundExitInBlock(Timer& timer)
{
  sendEvent(PE_inBlockClear);
}
void completedReadyLED(Timer& timer)
{
  if (readyLEDphase == LED_FlashOn)
    readyLEDphase = LED_FlashOff;
  else if (readyLEDphase == LED_FlashOff)
    readyLEDphase = LED_FlashOn;

  timer.Restart();
}
void completedBuzzerOn(Timer& timer)
{
  buzzerBeats--;
  noTone(PIN_BUZZER);
  timerBuzzerDelay.Restart();
}
void completedBuzzerDelay(Timer& timer)
{
  if (buzzerBeats > 0)
    startBuzzer();
}
void completedSensor(Timer& timer)
{
  onTrainArriving();
  timer.Reset();
}