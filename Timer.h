class Timer
{
public:
  Timer(float startTime)
  {
    _time = _startTime = startTime;
  }

  void Update(float deltaT)
  {
    if (!_running)
      return;

    _time -= min(deltaT, _time);
    if (_time <= 0.0f)
    {
      Stop();
      if (_func)
        _func(*this);
    }
  }

  float Get() { return _time; }

  bool Completed() { return _time <= 0.f; }

  void Reset() { _time = _startTime; }

  void Restart()
  {
    Reset();
    Start();
  }

  void Start() { _running = true; }

  void Stop() { _running = false; }

  bool IsRunning() { return _running; }

  void OnCompleted(void (*func)(Timer&))
  {
    _func = func;
  }

private:
  float _time;
  float _startTime;
  bool _running = false;
  void (*_func)(Timer&);
};