class base
{
  virtual int method(int);
};
class start : public base
{
  int method(int) final;
};
