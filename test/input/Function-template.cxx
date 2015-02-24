template <typename T> T start(T) { return T(); }
template <> char start<char>(char); // specialization
template int start<int>(int); // instantiation
