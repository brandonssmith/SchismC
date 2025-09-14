/* 
 * Complete Function Test Program for SchismC
 * Demonstrates all implemented function features
 */

/* Function to add two numbers */
I64 add(I64 a, I64 b) {
    return a + b;
}

/* Function to multiply two numbers */
I64 multiply(I64 x, I64 y) {
    return x * y;
}

/* Function to calculate factorial */
I64 factorial(I64 n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

/* Function to find maximum of two numbers */
I64 max(I64 a, I64 b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

/* Main function that demonstrates all features */
I64 main() {
    /* Test basic arithmetic functions */
    I64 sum = add(10, 20);
    I64 product = multiply(5, 6);
    
    /* Test function composition */
    I64 result = add(multiply(2, 3), multiply(4, 5));
    
    /* Test recursive function */
    I64 fact = factorial(5);
    
    /* Test conditional function */
    I64 maximum = max(100, 200);
    
    /* Return a combination of results */
    return add(sum, product);
}
