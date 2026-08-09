int test_jump_threading(int cond, int val) {
    int x;
    if (cond) {
        x = 1;
    } else {
        x = 0;
    }

    if (x) {
        return val + 100;
    } else {
        return val - 100;
    }
}