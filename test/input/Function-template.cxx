template <typename T> T start(T) {}
template <> char start<char>(char); // specialization
template int start<int>(int); // instantiation
