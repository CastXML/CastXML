namespace start {
template<class T> struct Box {};
extern template struct Box<int>;
template<class T> void run(T) {}
extern template void run<int>(int);
template void run<double>(double);
}
