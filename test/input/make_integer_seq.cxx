template <typename _T, _T...> struct seq;
typedef __make_integer_seq<seq, int, 3> seq_A;
typedef seq<int,0,1,2> seq_B;
template <typename A, typename B> struct assert_same;
template <typename A> struct assert_same<A,A> {};
assert_same<seq_A,seq_B> enforce;
