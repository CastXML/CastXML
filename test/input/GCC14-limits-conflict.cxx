// Test that reproduces GCC 14 limits header conflict
// Simulates the issue where numeric_limits specializations conflict

namespace std {
  // Simulate the standard numeric_limits template
  template<typename T>
  struct numeric_limits {
    static T min();
    static T max();
    static T denorm_min();
  };
  
  // Standard specializations that would exist
  template<>
  struct numeric_limits<float> {
    static float min() { return 0.0f; }
    static float max() { return 0.0f; }
    static float denorm_min() { return 0.0f; }
  };
  
  template<>
  struct numeric_limits<double> {
    static double min() { return 0.0; }
    static double max() { return 0.0; }
    static double denorm_min() { return 0.0; }
  };

  template<>
  struct numeric_limits<long double> {
    static long double min() { return 0.0L; }
    static long double max() { return 0.0L; }
    static long double denorm_min() { return 0.0L; }
  };

  // Test with CastXML's _Float32 and _Float64 types
  // This simulates the GCC 14 issue where __glibcxx_float_n macro
  // would try to create specializations for these types
  template<>
  struct numeric_limits<_Float32> {
    static _Float32 min() { return _Float32(0.0f); }
    static _Float32 max() { return _Float32(0.0f); }  
    static _Float32 denorm_min() { return _Float32(0.0f); }
  };
  
  template<>
  struct numeric_limits<_Float64> {
    static _Float64 min() { return _Float64(0.0); }
    static _Float64 max() { return _Float64(0.0); }
    static _Float64 denorm_min() { return _Float64(0.0); }
  };

#ifdef _Float128
  template<>
  struct numeric_limits<_Float128> {
    static _Float128 min() { return _Float128(0.0L); }
    static _Float128 max() { return _Float128(0.0L); }
    static _Float128 denorm_min() { return _Float128(0.0L); }
  };
#endif
}

namespace start {

// Test usage of numeric_limits with different floating point types
float test_float_limits() {
    return std::numeric_limits<float>::min();
}

double test_double_limits() {
    return std::numeric_limits<double>::min();
}

long double test_long_double_limits() {
    return std::numeric_limits<long double>::min();
}

float test_float_denorm_min() {
    return std::numeric_limits<float>::denorm_min();
}

double test_double_denorm_min() {
    return std::numeric_limits<double>::denorm_min();
}

long double test_long_double_denorm_min() {
    return std::numeric_limits<long double>::denorm_min();
}

// Test usage with extended floating point types
_Float32 test_float32_limits() {
    return std::numeric_limits<_Float32>::min();
}

_Float64 test_float64_limits() {
    return std::numeric_limits<_Float64>::min();
}

_Float32 test_float32_denorm_min() {
    return std::numeric_limits<_Float32>::denorm_min();
}

_Float64 test_float64_denorm_min() {
    return std::numeric_limits<_Float64>::denorm_min();
}

}