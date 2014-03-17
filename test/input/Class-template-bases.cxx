class non_dependent_base {};
template <typename T> class dependent_base {};
template <typename T> class start: public non_dependent_base, public dependent_base<T> {};
template class start<int>; // instantiation
