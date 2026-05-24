#include "Config.h"
#include "Timer.h"
#include "Event.h"
#include "Display.h"

#include <WiFi.h>
#include <SD.h>

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

enum ConsumedEventEnum
{
  CE_UNRECOGNIZED,
  CE_outSignalClearEvent,
  CE_outSignalStopEvent,
  CE_inSignalClearEvent,
  CE_inSignalStopEvent,
  CE_physicalBlockEnteredEvent,
  CE_distantAspectStopEvent,
  CE_distantAspectClear1Event,
  CE_distantAspectClear2Event
};

LEDPhase readyLEDphase;
int readyButtonValue;
int buzzerBeats = 0;

ProducedEvent producedEvents[8];
ConsumedEventMap<ConsumedEventEnum> consumedEvents(8, CE_UNRECOGNIZED);

TransitState currentState = TS_idle;

bool outboundTrainReady = false;
bool inboundSignalClear = false;
bool outboundSignalClear = false;
bool trainWaitingAtInboundSignal = false;
bool outboundReadyBlocked = false;

TimerList allTimers;

// Timers for departure
Timer timerOutboundDepartureReady(allTimers);
Timer timerOutboundEnterSwitch(allTimers);
Timer timerOutboundExitOutBlock(allTimers);
Timer timerOutboundEnterMain(allTimers);
Timer timerOutboundExitSwitch(allTimers);
Timer timerOutboundTransitMain(allTimers); 
Timer timerOutboundExitMain(allTimers);

// Timers for arrival
Timer timerInboundTransitMain(allTimers);
Timer timerInboundEnterSwitch(allTimers);
Timer timerInboundExitMain(allTimers);
Timer timerInboundEnterInBlock(allTimers);
Timer timerInboundExitSwitch(allTimers);
Timer timerInboundExitInBlock(allTimers);

// Misc timers
Timer timerReadyLED(allTimers);
Timer timerBuzzerOn(allTimers);
Timer timerBuzzerDelay(allTimers);
Timer timerSensor(allTimers);

// Display
Display display;
DistantAspect distantAspect = DA_Caution;

WiFiClient client;

void setup() 
{
  pinMode(PIN_RDY_LED, OUTPUT);
  pinMode(PIN_RDY_BTN, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);

  Serial.begin(115200);

  if (SD.begin(10))
  {
    Serial.println("FOUND CARD");
  }

  display.Init();
  
  // Establish WiFi connection
  Serial.println("Connecting:");

  WiFi.begin(G_Config.network.name, G_Config.network.password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to network");

  
  if (client.connect(G_Config.network.hostAddress, G_Config.network.hostPort))
  {
    Serial.println("Connected to host");
  }

  initTimers();

  producedEvents[PE_outBlockOccupied].Init(G_Config.lcc.event_blockOutOccupied, G_Config.lcc.sourceAlias);
  producedEvents[PE_outBlockClear].Init(G_Config.lcc.event_blockOutClear, G_Config.lcc.sourceAlias);
  producedEvents[PE_inBlockOccupied].Init(G_Config.lcc.event_blockInOccupied, G_Config.lcc.sourceAlias);
  producedEvents[PE_inBlockClear].Init(G_Config.lcc.event_blockInClear, G_Config.lcc.sourceAlias);
  producedEvents[PE_switchBlockOccupied].Init(G_Config.lcc.event_blockSwitchOccupied, G_Config.lcc.sourceAlias);
  producedEvents[PE_switchBlockClear].Init(G_Config.lcc.event_blockSwitchClear, G_Config.lcc.sourceAlias);
  producedEvents[PE_mainBlockOccupied].Init(G_Config.lcc.event_blockMainOccupied, G_Config.lcc.sourceAlias);
  producedEvents[PE_mainBlockClear].Init(G_Config.lcc.event_blockMainClear, G_Config.lcc.sourceAlias);
  
  consumedEvents.Assign(G_Config.lcc.event_outSignalClear, CE_outSignalClearEvent);
  consumedEvents.Assign(G_Config.lcc.event_outSignalStop, CE_outSignalStopEvent);
  consumedEvents.Assign(G_Config.lcc.event_inSignalClear, CE_inSignalClearEvent);
  consumedEvents.Assign(G_Config.lcc.event_inSignalStop, CE_inSignalStopEvent);
  consumedEvents.Assign(G_Config.lcc.event_actualBlockEntered, CE_physicalBlockEnteredEvent);
  consumedEvents.Assign(G_Config.lcc.event_distSigAspectStop, CE_distantAspectStopEvent);
  consumedEvents.Assign(G_Config.lcc.event_distSigAspectClear1, CE_distantAspectClear1Event);
  consumedEvents.Assign(G_Config.lcc.event_distSigAspectClear2, CE_distantAspectClear2Event);

  display.DrawStopSign();
}

void initTimers()
{
  // Timers for departure
  timerOutboundDepartureReady.Init(G_Config.simulation.timeDepartureReady);
  timerOutboundEnterSwitch.Init(G_Config.simulation.timeTransitOutboundBlock);
  timerOutboundExitOutBlock.Init(G_Config.simulation.timeTransitOutboundBlock + G_Config.simulation.timeBlockExit);
  timerOutboundEnterMain.Init(G_Config.simulation.timeTransitSwitchBlock);
  timerOutboundExitSwitch.Init(G_Config.simulation.timeTransitSwitchBlock + G_Config.simulation.timeBlockExit);
  timerOutboundTransitMain.Init(G_Config.simulation.timeTransitMainline / (float)G_Config.simulation.clockRate);
  timerOutboundExitMain.Init(G_Config.simulation.timeBlockExit);

  // Timers for arrival
  timerInboundTransitMain.Init(G_Config.simulation.timeTransitMainline / (float)G_Config.simulation.clockRate);
  timerInboundEnterSwitch.Init(G_Config.simulation.timeDepartureReady);
  timerInboundExitMain.Init(G_Config.simulation.timeBlockExit);
  timerInboundEnterInBlock.Init(G_Config.simulation.timeTransitSwitchBlock);
  timerInboundExitSwitch.Init(G_Config.simulation.timeTransitSwitchBlock + G_Config.simulation.timeBlockExit);
  timerInboundExitInBlock.Init(G_Config.simulation.timeTransitInboundBlock + G_Config.simulation.timeBlockExit);

  // Misc timers
  timerReadyLED.Init(G_Config.hardware.timeLEDBlink);
  timerBuzzerOn.Init(G_Config.hardware.timeBuzzerOn);
  timerBuzzerDelay.Init(G_Config.hardware.timeBuzzerDelay);
  timerSensor.Init(G_Config.hardware.sensorMinTime);

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
  if (analogRead(PIN_SENSOR) < G_Config.hardware.sensorThreshold)
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
    buzzerBeats = G_Config.hardware.departBuzzerBeats;

  tone(PIN_BUZZER, G_Config.hardware.buzzerTone);
  timerBuzzerOn.Restart();
}

void clientRead()
{
  if (client.available()) 
  {
    String data = client.readStringUntil('\r');

    ConsumedEventEnum eventType = consumedEvents.GetValue(data);
    switch (eventType)
    {
      case CE_outSignalClearEvent:
        setDepartureSignal(true);
        break;
      case CE_outSignalStopEvent:
        setDepartureSignal(false);
        break;
      case CE_inSignalClearEvent:
        setArrivalSignal(true);
        break;
      case CE_inSignalStopEvent:
        setArrivalSignal(false);
        break;
      case CE_physicalBlockEnteredEvent:
        onTrainDeparted();
        break;
      case CE_distantAspectStopEvent:
        updateDistantAspect(DA_Caution);
        break;
      case CE_distantAspectClear1Event:
        updateDistantAspect(DA_Clear1);
        break;
      case CE_distantAspectClear2Event:
        updateDistantAspect(DA_Clear2);
        break;
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
  TimerList::Iter iterator = allTimers.GetIterator();
  while(iterator.Iterate())
  {
    iterator.Get()->Update(seconds);
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