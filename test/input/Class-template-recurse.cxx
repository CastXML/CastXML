template <typename T> struct A;
template <typename T> struct C;
template <typename T> struct B {
  typedef C<T> type;
};
template <typename T> struct C {
  C() {
    static_cast<void>(sizeof(typename B< A<T> >::type));
  }
};
C<void> start;
