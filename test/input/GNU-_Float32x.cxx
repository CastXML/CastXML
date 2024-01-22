_Float32x start(_Float32x);

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
struct distinct<_Float32x>
{
};
