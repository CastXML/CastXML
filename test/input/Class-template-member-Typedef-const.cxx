template <typename T> class start {
  typedef T IntConst;
public:
  T method(IntConst);
};
template class start<int const>;
