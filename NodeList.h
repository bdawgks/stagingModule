template <typename T>
class NodeList
{
private:
  class Node
  {
  public:
    Node(T& item) : _item(item) {}

    void Add(T& item)
    {
      if (_next)
        _next->Add(item);
      else
        _next = new Node(item);
    }

    T& Get() {return _item;}

    Node* Next() {return _next;}

  private:
    T& _item;
    Node* _next = nullptr;
  };

public:
  class Iter
  {
  public:
    Iter(Node* first) : _first(first) {}
    bool Iterate()
    {
      if (idx < 0)
        _current = _first;
      else
      {
        _current = _current->Next();
      }

      idx++;

      return _current != nullptr;
    }
    
    T* Get()
    {
      return &(_current->Get());
    }

  private:
    Node* _first;
    Node* _current;
    int idx = -1;
  };

  void Add(T& item)
  {
    if (_first)
      _first->Add(item);
    else
      _first = new Node(item);
  }

  Iter GetIterator()
  {
    return Iter(_first);
  }

private:
  Node* _first = nullptr;
};