#include "basic.h"

void swap(void* lhs, void* rhs) {
    void* tmp = rhs;
    rhs = lhs;
    lhs = tmp;
}