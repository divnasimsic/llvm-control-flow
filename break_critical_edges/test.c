int test(int a, int b) {
    if (a > 10) {
        b += 5;
    }
    if (b > 20) {
        a += 10;
    }
    return a + b;
}