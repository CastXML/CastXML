_Float64 start(_Float64);

double conversions(double x)
{
  return start(x);
}

template <typename T>
struct distinct;
template <>
struct distinct<double>
{
};
template <>
struct distinct<_Float64>
{
};
