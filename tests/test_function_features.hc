I64 calculate(I64 a, I64 b, I64 c) {
    return a + b * c;
}

I64 factorial(I64 n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

I64 fibonacci(I64 n) {
    if (n <= 1) {
        return n;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

calculate(1, 2, 3);
factorial(5);
fibonacci(10);
