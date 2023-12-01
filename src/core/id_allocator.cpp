#include "id_allocator.hpp"
#include "basic.hpp"
#include "logging.hpp"

IDAllocatorList::IDAllocatorList(int p_type_size) {
    alloc_count = 0;
    capacity = 32;
    __type_size = p_type_size;
    array = malloc(__type_size * capacity);
    free_index_array = (int*)malloc(sizeof(int) * capacity);
    for(int i = 0; i < capacity; i++) free_index_array[i] = i;
}

IDAllocatorList::~IDAllocatorList() {
    free(array);
    free(free_index_array);
}

int IDAllocatorList::Allocate(void** ret_ptr) {
    if (alloc_count >= capacity) {
        capacity += 32;
        free_index_array = (int*)realloc(free_index_array, sizeof(int) * capacity);
        array = realloc(array, __type_size * capacity);
        for(int i = capacity-32; i < capacity; i++) free_index_array[i] = i;
    }
    int free_index = free_index_array[alloc_count];
    if (ret_ptr != NULL) {
        *ret_ptr = Get(free_index);
    }
    alloc_count++;
    return free_index;
}

void IDAllocatorList::Erase(int index) {
    alloc_count--;
    free_index_array[alloc_count] = index;
}

void* IDAllocatorList::Get(int index) {
    return array + index * __type_size;
}

bool IDAllocatorList::IsValidIndex(int index) {
    return index < alloc_count;  // TODO: more sophisticated checks
}

#define LIST_TEST_ASSERT_WITH_ERROR(expr) if (!(expr)) {ERROR("expression %s not true", #expr); return 1; }
int IDAllocatorListTests() {
    IDAllocatorList list = IDAllocatorList(sizeof(Matrix));
    Matrix *mm[40];
    for(int i=0; i < 40; i++) {
        list.Allocate((void**) &mm[i]);
        *mm[i] = MatrixScale((float)i, 1, 1);
    }
    LIST_TEST_ASSERT_WITH_ERROR(((Matrix*)list.Get(23))->m0 == 23);
    list.Erase(2);
    list.Erase(3);
    LIST_TEST_ASSERT_WITH_ERROR(list.Allocate(NULL) == 3);
    LIST_TEST_ASSERT_WITH_ERROR(list.Allocate(NULL) == 2);
    LIST_TEST_ASSERT_WITH_ERROR(list.Allocate(NULL) == 40);
    LIST_TEST_ASSERT_WITH_ERROR(((Matrix*)list.Get(23))->m0 == 23);
    return 0;
}