namespace start {
template<class T = int> struct Box {};
template<int N = 4> struct Buf {};
template<class T> concept Small = sizeof(T) <= 4;
template<Small T> void f(T, int) requires (Small<T> && sizeof(T) > 1);
template<class T> using Alias = T*;
template<class T> constexpr int value = sizeof(T);
template<class T> void walk(T) {}
extern template void walk<int>(int);
template void walk<char>(char);
template<> void walk<double>(double) {}
inline void use() { walk(1.f); }
using I = int;
template<> struct Box<I>;
template<template<class> class C = Box, class... Ts> struct Family {};
template<class T, class U = T> struct Dependent {};
}
