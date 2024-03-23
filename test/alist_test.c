#include "../alist.h"
#include <criterion/criterion.h>

ArrayList *alist;

void new_array_list(void) { alist = alist_new(); }

void free_array_list(void) { alist_free(alist); }

TestSuite(array_list, .init = new_array_list, .fini = free_array_list);

Test(array_list, new) {
    cr_expect_eq(alist->used, 0);
    cr_expect_eq(alist->size, 32);
}

Test(array_list, pushing_two_items) {
    alist_push(alist, 42);
    alist_push(alist, 37);
    cr_expect_eq(alist->data[0], 42);
    cr_expect_eq(alist->data[1], 37);
}

Test(array_list, pushing_forty_two) {
    for (int i = 0; i < 42; i++) {
        alist_push(alist, i);
    }
    for (int i = 0; i < 42; i++) {
        cr_expect_eq(alist->data[i], i);
    }
}
