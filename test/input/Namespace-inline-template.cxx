#pragma clang diagnostic ignored "-Wc++11-extensions"
namespace start {
inline namespace level1 {
struct A;
}
template <typename T>
struct B;
typedef B<A> B_A;
}
