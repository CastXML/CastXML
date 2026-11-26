namespace constraints {
template<class T> concept Sized = sizeof(T) > 0;
template<class T> concept Both = Sized<T> && Sized<T*>;
}

template<constraints::Both T>
void start(T) requires constraints::Sized<T>;
