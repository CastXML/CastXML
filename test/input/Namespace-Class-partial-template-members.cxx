namespace start {
  template <typename T> class A;
  template <typename T> class A<T&> {
    static int data;
  };
  template <typename T> int A<T&>::data;
}
