template <typename T>
class start
{
  template <typename U>
  T method(U)
  {
    return T();
  }
};
template int start<int>::method<char>(char); // instantiation
