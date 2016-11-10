template <typename>
struct A
{
  struct B
  {
    typedef int intermediate_type;
    typedef intermediate_type type;
  };
};
typedef A<int>::B::type start;
