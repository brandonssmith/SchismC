I64 add(I64 a, I64 b) {
    return a + b;
}

I64 multiply(I64 x, I64 y) {
    return x * y;
}

I64 square(I64 n) {
    return multiply(n, n);
}

I64 main() {
    I64 result1 = add(10, 20);
    I64 result2 = multiply(5, 6);
    I64 result3 = square(4);
    
    return result1;
}

