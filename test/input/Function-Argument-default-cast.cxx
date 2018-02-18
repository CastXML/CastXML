namespace start {
struct Base
{
  Base();
  virtual ~Base();
};
struct Derived : public Base
{
  Derived();
  Derived(Derived const&);
  Derived& operator=(Derived const&);
  virtual ~Derived();
};
Base* b();
Base const* bc();
typedef int Int;
void f(Int = (Int)0, Base* = (Base*)0, Base* = static_cast<Base*>(0),
       Base* = reinterpret_cast<Base*>(0), Base* = const_cast<Base*>(bc()),
       Derived* = dynamic_cast<Derived*>(b()));
}
