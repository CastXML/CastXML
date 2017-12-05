template <bool>
struct Bool;
typedef Bool<__has_unique_object_representations(int)> BoolType;
