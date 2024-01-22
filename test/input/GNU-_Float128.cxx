_Float128 start(_Float128);

__float128 conversions(__float128 x)
{
  return start(x);
}

template <typename T>
struct distinct;
template <>
struct distinct<__float128>
{
};
template <>
struct distinct<_Float128>
{
};
