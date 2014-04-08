template <typename T> class base {
protected:
  T data;
  base();
  void operator=(base const& a) {
    this->data = a.data;
  }
};
class start: public base<int const> {};
