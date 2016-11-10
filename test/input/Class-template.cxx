template <typename T>
class start
{
};
template <typename T>
struct start<T&>
{
}; // partial specialization
template <>
class start<char>;          // specialization
template class start<int>;  // instantiation
template class start<int&>; // instantiation of partial specialization
