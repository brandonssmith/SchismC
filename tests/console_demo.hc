I64 get_hello_value() {
    return 42;
}

I64 get_world_value() {
    return 24;
}

I64 calculate_sum() {
    return 66;
}

I64 main() {
    I64 hello = get_hello_value();
    I64 world = get_world_value();
    I64 sum = calculate_sum();
    
    return sum;
}

