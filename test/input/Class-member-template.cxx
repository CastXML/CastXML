class start
{
  template <typename T>
  T method(T v)
  {
    return v;
  }
};
template int start::method<int>(int); // instantiation
