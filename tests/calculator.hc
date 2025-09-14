/*
 * Calculator test program for SchismC
 * Tests function definitions and Print statements
 */

I64 add(I64 a, I64 b) {
    return a + b;
}

I64 subtract(I64 a, I64 b) {
    return a - b;
}

I64 multiply(I64 a, I64 b) {
    return a * b;
}

I64 divide(I64 a, I64 b) {
    return a / b;
}

I64 main() {
    I64 a = 10;
    I64 b = 3;
    
    "Calculator Demo\n";
    "==============\n";
    "%d + %d = %d\n", a, b, add(a, b);
    "%d - %d = %d\n", a, b, subtract(a, b);
    "%d * %d = %d\n", a, b, multiply(a, b);
    "%d / %d = %d\n", a, b, divide(a, b);
    
    return 0;
}
