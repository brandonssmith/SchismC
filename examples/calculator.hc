// Simple calculator in HolyC
I64 add(I64 a, I64 b) {
    return a + b;
}

I64 subtract(I64 a, I64 b) {
    return a - b;
}

I64 multiply(I64 a, I64 b) {
    return a * b;
}

F64 divide(F64 a, F64 b) {
    if (b != 0) {
        return a / b;
    } else {
        "Error: Division by zero!\n";
        return 0;
    }
}

I64 main() {
    I64 a = 10;
    I64 b = 3;
    
    "Calculator Demo\n";
    "==============\n";
    "%d + %d = %d\n", a, b, add(a, b);
    "%d - %d = %d\n", a, b, subtract(a, b);
    "%d * %d = %d\n", a, b, multiply(a, b);
    "%d / %d = %.2f\n", a, b, divide(a, b);
    
    return 0;
}
