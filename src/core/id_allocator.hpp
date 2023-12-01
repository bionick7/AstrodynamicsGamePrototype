#ifndef ID_ALLOCATOR_H
#define ID_ALLOCATOR_H

struct IDAllocatorList {
    void* array;
    int* free_index_array;
    int alloc_count;
    int capacity;
    int __type_size;

    IDAllocatorList(int p_type_size);
    ~IDAllocatorList();
    void Erase(int index);
    int Allocate(void**);
    void* Get(int index);
    bool IsValidIndex(int index);
};

int IDAllocatorListTests();

#endif  // ID_ALLOCATOR_H