class base {
protected:
  base();
  ~base();
private:
  base(base const&);
  base& operator=(base const&);
  mutable int data;
};
class start: public base {};
