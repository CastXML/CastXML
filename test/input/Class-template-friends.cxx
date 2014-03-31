template <typename T> class A;
template <typename T> int f(T);
template <typename T> class start {
  friend class A<T>;
  friend int f<T>(T);
  template <typename U> friend int f(U);
};
template class start<int>;
