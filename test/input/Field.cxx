class start
{
  int field;
  unsigned bit_field : 2;
  mutable int mutable_field;
public:
  start(start const&);
  start& operator=(start const&);
  start();
  ~start();
};
