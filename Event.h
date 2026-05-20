class Event
{
public:
  static String GetStringValue(const char* idstr)
  {
    String str;
    int c = 0;
    while (idstr[c] != '\0')
    {
      if (idstr[c] != '.')
        str += idstr[c];

      c++;
    }

    return str;
  }
};

class ProducedEvent
{
public:
  ProducedEvent() {}
  ProducedEvent(const char* event, const char* sa)
  {
    Init(event, sa);
  }

  void Init(const char* event, const char* sa)
  {
    _msgStr = ":X195B4";
    String saStr = sa;
    saStr.toUpperCase();
    _msgStr += saStr + "N";
    _msgStr += Event::GetStringValue(event) + ";";
  }

  String GetMessageString() {return _msgStr;}
  
private:
  String _msgStr;
};

class ConsumedEvent
{
public:
  ConsumedEvent(const char* event)
  {
    _cmpStr = Event::GetStringValue(event);
  }

  bool IsInMessage(String msgStr)
  {
    String headerStr = msgStr.substring(0, 7);
    String eventStr = msgStr.substring(11, 27);

    /*Serial.println("COMPARING MSG STRINGS");
    Serial.print("header - ");
    Serial.println(headerStr);
    Serial.print("event - ");
    Serial.println(eventStr);
    Serial.print("cmpStr - ");
    Serial.println(_cmpStr);*/

    return (headerStr == ":X195B4" && eventStr == _cmpStr);
  }

private:
  String _cmpStr;
};