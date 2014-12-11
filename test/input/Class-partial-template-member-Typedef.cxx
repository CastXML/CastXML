template <typename T> class start;
template <typename T> class start<T&> {
  typedef T Int;
public:
  T method(Int);
};
template class start<int&>;
