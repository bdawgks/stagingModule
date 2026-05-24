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
  void Init(const char* event)
  {
    _cmpStr = Event::GetStringValue(event);
  }

  bool IsInMessage(String msgStr)
  {
    String headerStr = msgStr.substring(0, 7);
    String eventStr = msgStr.substring(11, 27);

    return (headerStr == ":X195B4" && eventStr == _cmpStr);
  }

private:
  String _cmpStr;
};

template <typename E>
class ConsumedEventMap
{
public:
  ConsumedEventMap(int size, E undefined) : _size(size), _undefined(undefined)
  {
    _values = new E[size];
    _events = new ConsumedEvent[size];
  }

  void Assign(const char* event, E value)
  {
    if (_nextIdx >= _size)
      return;

    _events[_nextIdx].Init(event);
    _values[_nextIdx] = value;

    _nextIdx++;
  }

  E GetValue(String data)
  {
    for (int i = 0; i < _nextIdx; i++)
    {
      if (_events[i].IsInMessage(data))
        return _values[i];
    }

    return _undefined;
  }

private:
  int _size;
  int _nextIdx = 0;
  E* _values;
  ConsumedEvent* _events;
  E _undefined;
};