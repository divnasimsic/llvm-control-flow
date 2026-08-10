int test(int x) {
    if (x > 10) {
        return 1;
    }

    goto end;

    unused:
        x = x + 100;

    end:
        return x;
}