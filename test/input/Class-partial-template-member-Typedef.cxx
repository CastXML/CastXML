template <typename T> class start;
template <typename T> class start<T&> {
  typedef int Int;
public:
  int method(Int);
};
template class start<int&>;
