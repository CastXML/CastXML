class base {
protected:
  int f(int);
};
class start: public base {
  using base::f;
  int f(char);
};
