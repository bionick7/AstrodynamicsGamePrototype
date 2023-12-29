#ifndef ID_ALLOCATOR_H
#define ID_ALLOCATOR_H
#include <basic.hpp>
#include "logging.hpp"


#define UNIT64 ((uint64_t)1ull)
template<typename T>
struct IDAllocatorList {

    struct Iterator { 
        entity_id_t index;
        int iterator;
        int count;
        const IDAllocatorList<T>* list_ptr;

        bool IsIterGoing() const {
            return iterator < count && index < list_ptr->capacity;
        }

        operator bool () const { return IsIterGoing(); }

        void operator++(int) {
            iterator++;
            do index++; while(index < list_ptr->capacity && 
                !(list_ptr->verifier_array[index/64] & (UNIT64 << (index % 64)))
            ); 
        }
    };

    ~IDAllocatorList() {
        _Destroy();
    }

    void Init() {
        alloc_count = 0;
        capacity = 32;
        array = (T*)malloc(sizeof(T) * capacity);
        free_index_array = (entity_id_t*)malloc(sizeof(entity_id_t) * capacity);
        verifier_array = (uint64_t*)malloc(sizeof(uint64_t) * ceil(capacity / 64.));
        for(int i = 0; i < capacity; i++) free_index_array[i] = i;
        for(int i = 0; i < ceil(capacity / 64.); i++) verifier_array[i] = 0;
    }

    entity_id_t Allocate(T** ret_ptr=NULL) {
        if (alloc_count >= capacity) {
            capacity += 32;
            array = (T*)realloc(array, sizeof(T) * capacity);
            free_index_array = (entity_id_t*)realloc(free_index_array, sizeof(entity_id_t) * capacity);
            verifier_array = (uint64_t*)realloc(verifier_array, sizeof(uint64_t) * ceil(capacity / 64.));
            for(entity_id_t i = capacity-32; i < capacity; i++) {
                free_index_array[i] = i;
                verifier_array[i/64] &= ~(UNIT64 << (i % 64));
            }
        }
        entity_id_t free_index = free_index_array[alloc_count];
        verifier_array[free_index/64] |= UNIT64 << (free_index % 64);
        if (ret_ptr != NULL) {
            *ret_ptr = Get(free_index);
        }
        *Get(free_index) = T();
        alloc_count++;
        return free_index;
    }

    void Erase(entity_id_t index) {
        Get(index)->~T();
        if ((verifier_array[index/64] & (UNIT64 << (index % 64))) == 0) {
            return;  // already erased
        }
        alloc_count--;
        free_index_array[alloc_count] = index;
        verifier_array[index/64] &= ~(UNIT64 << (index % 64));  // set bit to 0
    }
    
    inline T* Get(entity_id_t index) const { return &array[index]; }
    inline T* operator[] (entity_id_t index) { return &array[index]; }
    inline T* Get(Iterator iter) const { return &array[iter.index]; }
    inline T* operator[] (Iterator iter) { return &array[iter.index]; }

    entity_id_t Count() const { return alloc_count; }

    void Clear() {
        _Destroy();
        Init();
    }

    bool IsValidIndex(entity_id_t index) const {
        return index < capacity && (verifier_array[index/64] & (UNIT64 << (index % 64)));
    }

    Iterator GetIter() const {
        int start_index = 0;
        if (alloc_count != 0)
            while (!IsValidIndex(start_index)) start_index++;
        Iterator it;
        it.index = start_index;
        it.iterator = 0;
        it.count = alloc_count;
        it.list_ptr = this;
        return it;
    }

    void Inpsect() {
        SHOW_I(free_index_array[0])
        SHOW_I(verifier_array[0])
        SHOW_I(alloc_count)
        SHOW_I(capacity)
    }

    void _Destroy() {
        free(array);
        free(free_index_array);
        free(verifier_array);
    }

    T* array;
    entity_id_t* free_index_array;
    uint64_t* verifier_array;
    entity_id_t alloc_count;
    entity_id_t capacity;
};
#undef UNIT64
int IDAllocatorListTests();

#endif  // ID_ALLOCATOR_H