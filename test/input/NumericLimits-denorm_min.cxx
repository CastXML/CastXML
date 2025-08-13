#include <limits>

namespace start {

// Test std::numeric_limits<float>::denorm_min()
float test_float_denorm_min() {
  return std::numeric_limits<float>::denorm_min();
}

// Test std::numeric_limits<double>::denorm_min()
double test_double_denorm_min() {
  return std::numeric_limits<double>::denorm_min();
}

// Test std::numeric_limits<long double>::denorm_min()
long double test_long_double_denorm_min() {
  return std::numeric_limits<long double>::denorm_min();
}

// Test usage in variable declarations
const float float_denorm_min_value = std::numeric_limits<float>::denorm_min();
const double double_denorm_min_value = std::numeric_limits<double>::denorm_min();
const long double long_double_denorm_min_value = std::numeric_limits<long double>::denorm_min();

// Test usage in template context
template<typename T>
T get_denorm_min() {
  return std::numeric_limits<T>::denorm_min();
}

// Explicit template instantiations
template float get_denorm_min<float>();
template double get_denorm_min<double>();
template long double get_denorm_min<long double>();

}
