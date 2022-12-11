#pragma clang diagnostic ignored "-Wundefined-inline"
constexpr void* __cdecl __builtin_assume_aligned(const void*, size_t,
                                                 ...) noexcept;

int* check_assume_aligned(int* p)
{
  __builtin_assume_aligned(p, 4);
  return p;
}
