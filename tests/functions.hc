// Function definitions and calls
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
    I64 num = 5;
    I64 fact = factorial(num);
    I64 fib = fibonacci(num);
    
    "Factorial of %d is %d\n", num, fact;
    "Fibonacci of %d is %d\n", num, fib;
    
    return 0;
}
