// Variable declarations and operations
I64 x = 42;
F64 y = 3.14159;
String name = "HolyC";

I64 add(I64 a, I64 b) {
    return a + b;
}

I64 main() {
    I64 result = add(x, 10);
    "Result: %d\n", result;
    "Pi is approximately: %f\n", y;
    "Language: %s\n", name;
    return 0;
}
