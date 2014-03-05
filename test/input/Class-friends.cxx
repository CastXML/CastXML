class A {};
void f(void);
class start {
  friend class A;
  friend void f(void);
};
