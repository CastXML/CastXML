_Float32 start(_Float32);

float conversions(float x)
{
  return start(x);
}

template <typename T>
struct distinct;
template <>
struct distinct<float>
{
};
template <>
struct distinct<_Float32>
{
};
