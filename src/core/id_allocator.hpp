#ifndef ID_ALLOCATOR_H
#define ID_ALLOCATOR_H
#include <basic.hpp>
#include "logging.hpp"
#include "id_system.hpp"
#include "datanode.hpp"

#define UNIT64 ((uint64_t)1ull)  // make sure it's always exactly 64-bit
template<typename T, EntityType E>
struct IDAllocatorList {
    T* array;
    uint32_t* free_index_array;
    uint64_t* verifier_array;
    uint32_t alloc_count;
    uint32_t capacity;

    class Iterator : public std::iterator<
        std::input_iterator_tag,
        RID, int, RID*, RID> {

        const IDAllocatorList<T, E>* _list_ptr;
    public:
        uint32_t index;
        uint32_t counter;

        explicit Iterator(int p_index, int p_counter, const IDAllocatorList<T, E>* p_list_ptr) { 
            index = p_index; 
            counter = p_counter; 
            _list_ptr = p_list_ptr;
        }

        Iterator& operator++() {
            counter++;
            do {
                index++;
                if (index >= _list_ptr->capacity) {
                    index = _list_ptr->capacity;
                    return *this;
                }
            } while( !(_list_ptr->verifier_array[index/64] & (UNIT64 << (index % 64))) ); 
            return *this;
        }

        Iterator operator++(int) { 
            Iterator retval = *this;
            ++(*this); 
            return retval; 
        }

        bool operator==(Iterator other) const { 
            return counter == other.counter && _list_ptr == other._list_ptr;
        }

        bool operator!=(Iterator other) const { return !(*this == other); }
        RID operator*() const { return RID(index, E); }

        operator bool () const {
            return counter < _list_ptr->alloc_count && index < _list_ptr->capacity;
        }

        RID GetId() const { return RID(index, E); }
    };

    /*struct Iterator { 
        const IDAllocatorList<T, E>* list_ptr;
        uint32_t index;
        uint32_t counter;
        uint32_t count;

        bool IsIterGoing() const {
            return counter < count && index < list_ptr->capacity;
        }

        RID GetId() {
            return RID(index, E);
        }

        operator bool () const { return IsIterGoing(); }

        void operator++(int) {
            counter++;
            do index++; while(index < list_ptr->capacity && 
                !(list_ptr->verifier_array[index/64] & (UNIT64 << (index % 64)))
            ); 
        }
    };*/

    ~IDAllocatorList() {
        _Destroy();
    }

    void Init() {
        alloc_count = 0;
        capacity = 32;
        array = (T*)malloc(sizeof(T) * capacity);
        free_index_array = (uint32_t*)malloc(sizeof(uint32_t) * capacity);
        verifier_array = (uint64_t*)malloc(sizeof(uint64_t) * ceil(capacity / 64.));
        for(int i = 0; i < capacity; i++) free_index_array[i] = i;
        for(int i = 0; i < ceil(capacity / 64.); i++) verifier_array[i] = 0;
    }

    bool AllocateAtID(RID id, T** ret_ptr=NULL) {
        if (IdGetType(id) != E) {
            return false;
        }
        if (ContainsID(id)) {
            return false;
        }
        uint32_t index = IdGetIndex(id);
        if (index > 128 + capacity) {
            WARNING("Forcing the allocation of an RID will excessively extend (>128) the IDallocator. Index is %d, capacity is %d", index, capacity)
        }
        while (index >= capacity) {
            capacity += 32;
            array = (T*)realloc(array, sizeof(T) * capacity);
            free_index_array = (uint32_t*)realloc(free_index_array, sizeof(uint32_t) * capacity);
            verifier_array = (uint64_t*)realloc(verifier_array, sizeof(uint64_t) * ceil(capacity / 64.));
            for(uint32_t i = capacity-32; i < capacity; i++) {
                free_index_array[i] = i;
                verifier_array[i/64] &= ~(UNIT64 << (i % 64));
            }
        }
        verifier_array[index/64] |= UNIT64 << (index % 64);
        if (ret_ptr != NULL) {
            *ret_ptr = &array[index];
        }
        array[index] = T();
        alloc_count++;
        return true;
    }

    RID Allocate(T** ret_ptr=NULL) {
        if (alloc_count >= capacity) {
            capacity += 32;
            array = (T*)realloc(array, sizeof(T) * capacity);
            free_index_array = (uint32_t*)realloc(free_index_array, sizeof(uint32_t) * capacity);
            verifier_array = (uint64_t*)realloc(verifier_array, sizeof(uint64_t) * ceil(capacity / 64.));
            for(uint32_t i = capacity-32; i < capacity; i++) {
                free_index_array[i] = i;
                verifier_array[i/64] &= ~(UNIT64 << (i % 64));
            }
        }
        uint32_t free_index = free_index_array[alloc_count];
        verifier_array[free_index/64] |= UNIT64 << (free_index % 64);
        if (ret_ptr != NULL) {
            *ret_ptr = &array[free_index];
        }
        array[free_index] = T();
        alloc_count++;
        return RID(free_index, E);
    }

    void Erase(RID id) {
        Get(id)->~T();
        uint32_t index = IdGetIndex(id);
        if (index >= capacity) {
            return;
        }
        if ((verifier_array[index/64] & (UNIT64 << (index % 64))) == 0) {
            return;  // already erased
        }
        alloc_count--;
        free_index_array[alloc_count] = index;
        verifier_array[index/64] &= ~(UNIT64 << (index % 64));  // set bit to 0
    }
    
    inline T* Get(RID id) const { 
        if (!ContainsID(id)) {
            return NULL;
        }
        return &array[IdGetIndex(id)]; 
    }
    inline T* operator[] (RID id) { return Get(id); }
    inline T* Get(Iterator iter) const { return Get(iter.GetId()); }
    inline T* operator[] (Iterator iter) { return Get(iter.GetId()); }

    uint32_t Count() const { return alloc_count; }

    void Clear() {
        _Destroy();
        Init();
    }

    bool ContainsID(RID id) const {
        if (IdGetType(id) != E) {
            return false;
        }
        uint32_t index = IdGetIndex(id);
        if (index >= capacity) {
            return false;
        }
        return verifier_array[index/64] & (UNIT64 << (index % 64));
    }

    Iterator Begin() const {
        int start_index = 0;
        if (alloc_count != 0) {
            while (start_index < capacity 
                && !(verifier_array[start_index/64] & (UNIT64 << (start_index % 64)))
            ) start_index++;
        }
        return Iterator(start_index, 0, this);
    }

    Iterator GetIter() const { return Begin(); }

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

    typedef void SerializationFn(DataNode*, const T*);
    typedef void DeserializationFn(const DataNode*, T*);

    void DeserializeFrom(const DataNode* dn, const char* key, DeserializationFn* fn) {
        Clear();
        for(int i=0; i < dn->GetArrayChildLen(key); i++) {
            DataNode* child = dn->GetArrayChild(key, i);
            T* d;
            AllocateAtID(RID(child->GetI("id")), &d);
            fn(child, d);
        }
    }
    
    void SerializeInto(DataNode* dn, const char* key, SerializationFn* fn) const {
        dn->SetArrayChild(key, alloc_count);
        for(auto it = GetIter(); it; it++) {
            DataNode* child = dn->SetArrayElemChild(key, it.counter, DataNode());
            child->SetI("id", it.GetId().AsInt());
            fn(child, Get(it));
        }        
    }

};
#undef UNIT64
int IDAllocatorListTests();

#endif  // ID_ALLOCATOR_H