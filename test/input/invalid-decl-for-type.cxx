template <typename T>
struct A
{
  T x;
};
template <typename T>
struct B
{
  B() { A<T> a; }
};
struct Incomplete;
struct start
{
  B<Incomplete> b;
  typedef A<Incomplete> type;
};
