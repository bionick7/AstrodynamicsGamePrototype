#include "id_allocator.hpp"
#include "basic.hpp"
#include "logging.hpp"


IDAllocatorList::IDAllocatorList(size_t p_type_size) {
    alloc_count = 0;
    capacity = 32;
    __type_size = p_type_size;
    array = malloc(__type_size * capacity);
    free_index_array = (size_t*)malloc(sizeof(size_t) * capacity);
    verifier_array = (uint64_t*)malloc(sizeof(uint64_t) * ceil(capacity / 64.));
    for(int i = 0; i < capacity; i++) free_index_array[i] = i;
    for(int i = 0; i < ceil(capacity / 64.); i++) verifier_array[i] = 0;
}

IDAllocatorList::~IDAllocatorList() {
    free(array);
    free(free_index_array);
}

size_t IDAllocatorList::Allocate(void** ret_ptr) {
    if (alloc_count >= capacity) {
        capacity += 32;
        array = realloc(array, __type_size * capacity);
        free_index_array = (size_t*)realloc(free_index_array, sizeof(size_t) * capacity);
        verifier_array = (uint64_t*)realloc(verifier_array, sizeof(uint64_t) * ceil(capacity / 64.));
        for(size_t i = capacity-32; i < capacity; i++) {
            free_index_array[i] = i;
            verifier_array[i/64] &= ~(1ul << (i % 64));
        }
    }
    size_t free_index = free_index_array[alloc_count];
    verifier_array[free_index/64] |= 1ul << (free_index % 64);
    if (ret_ptr != NULL) {
        *ret_ptr = Get(free_index);
    }
    alloc_count++;
    return free_index;
}

void IDAllocatorList::Erase(size_t index) {
    alloc_count--;
    free_index_array[alloc_count] = index;
    verifier_array[index/64] &= ~(1 << (index % 64));
}

void* IDAllocatorList::Get(size_t index) {
    return (void*) ((char*)array + index * __type_size);
}

bool IDAllocatorList::IsValidIndex(size_t index) {
    return index < capacity && (verifier_array[index/64] & (1 << (index % 64)));
}

IDAllocatorListIterator IDAllocatorList::GetIter() {
    return { 0, 0 };
}

bool IDAllocatorList::IsIterGoing(IDAllocatorListIterator iter) {
    return iter.iterator < alloc_count && iter.index < capacity; 
}

void IDAllocatorList::IncIterator(IDAllocatorListIterator* iter) {
    iter->iterator++;
    do iter->index++; while(!IsValidIndex(iter->index)); 
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
    list.Erase(17);
    list.Erase(8);
    list.Erase(15);
    list.Allocate(NULL);
    printf("%016zX\n", *list.verifier_array);
    for(auto i = list.GetIter(); list.IsIterGoing(i); list.IncIterator(&i)) {
        Matrix* m = (Matrix*) list.Get(i.index);
        printf("%f\n", m->m0);
    }
    return 0;
}