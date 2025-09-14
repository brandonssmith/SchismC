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

I64 power(I64 base, I64 exp) {
    I64 result = 1;
    I64 i = 0;
    while (i < exp) {
        result = result * base;
        i = i + 1;
    }
    return result;
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

I64 main() {
    I64 a = 10;
    I64 b = 5;
    
    I64 sum = add(a, b);
    I64 diff = subtract(a, b);
    I64 product = multiply(a, b);
    I64 quotient = divide(a, b);
    I64 power_result = power(2, 3);
    I64 fact_result = factorial(5);
    I64 fib_result = fibonacci(6);
    
    return sum + diff + product + quotient + power_result + fact_result + fib_result;
}

