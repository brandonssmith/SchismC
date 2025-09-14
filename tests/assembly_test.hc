// Assembly Backend Test
// Tests basic arithmetic operations with assembly generation

I64 add(I64 a, I64 b) {
    return a + b;
}

I64 multiply(I64 a, I64 b) {
    return a * b;
}

main() {
    I64 result1 = add(5, 3);
    I64 result2 = multiply(4, 7);
    
    "Addition result: %d\n", result1;
    "Multiplication result: %d\n", result2;
}
