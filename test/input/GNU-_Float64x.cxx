_Float64x start(_Float64x);

long double conversions(long double x)
{
  return start(x);
}

template <typename T>
struct distinct;
template <>
struct distinct<long double>
{
};
template <>
struct distinct<_Float64x>
{
};
