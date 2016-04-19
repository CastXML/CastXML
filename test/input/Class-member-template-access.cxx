class start {
  template <typename> class member {};
public:
  typedef member<char> member_char; // incomplete
};
template class start::member<int>; // instantiation
