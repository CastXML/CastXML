namespace start {
void native(int);
void __attribute__((ms_abi)) foreign(int);
typedef void (*NativeCallback)(int);
typedef void(__attribute__((ms_abi)) * ForeignCallback)(int);
}
