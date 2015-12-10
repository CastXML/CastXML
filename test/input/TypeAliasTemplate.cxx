template <typename T> struct A { typedef T type; };
namespace start {
  template <typename T> using type = typename A<T>::type;
}
