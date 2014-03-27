template <typename T> class start {
public:
  start(start const&);
  template <typename U> start(start<U> const&);
};
start<int> instantiate_and_copy(start<int> const& x) { return x; }
