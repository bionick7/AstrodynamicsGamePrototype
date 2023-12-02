#ifndef ID_ALLOCATOR_H
#define ID_ALLOCATOR_H
#include <basic.hpp>

struct IDAllocatorListIterator { int index, iterator; };

struct IDAllocatorList {
    void* array;
    size_t* free_index_array;
    uint64_t* verifier_array;
    size_t alloc_count;
    size_t capacity;
    size_t __type_size;

    IDAllocatorList(size_t p_type_size);
    ~IDAllocatorList();
    void Erase(size_t index);
    size_t Allocate(void** ret_ptr);
    void* Get(size_t index);
    bool IsValidIndex(size_t index);

    IDAllocatorListIterator GetIter();
    bool IsIterGoing(IDAllocatorListIterator iter);
    void IncIterator(IDAllocatorListIterator* iter);
};


int IDAllocatorListTests();

#endif  // ID_ALLOCATOR_H