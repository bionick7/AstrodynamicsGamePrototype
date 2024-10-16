#ifndef LIST_H
#define LIST_H

#include "logging.hpp"
#include "basic.hpp"

namespace _current_fn {
    extern void* _current_fn;
}

template <typename T>
struct List {
    typedef int SortFn(T, T);

    int capacity;
    int size;

    T* buffer;
        
    List() {
        Init();
    }

    List(int initial_capacity) {
        capacity = initial_capacity;
        size = 0;
        buffer = (T*) malloc(sizeof(T) * capacity);
    }

    List(const List& other) {
        CopyList(&other, this);
    }

    ~List() {
        Clear();
    }

    void Resize(int new_capacity) {
        if (new_capacity == 0) {
            Clear();
        } else {
            buffer = (buffer == NULL) ? (T*) malloc(new_capacity * sizeof(T))
                                      : (T*) realloc(buffer, new_capacity * sizeof(T));
        }
        capacity = new_capacity;
    }

    void Append(T id) {
        int index = AllocForAppend();
        buffer[index] = id;
    }

    int AllocForAppend() {
        if (size >= capacity) {
            int extension = capacity/2;
            if (extension < 5) extension = 5;
            Resize(capacity + extension);
        }
        int res = size;
        new (&buffer[res]) T();  // Size of array must always be constructed
        size++;
        return res;
    }

    void EraseAt(int index) {
        // O(n) expensive
        if (index >= size) return;
        for(int i=index; i < size; i++) {
            buffer[i] = buffer[i+1];
        }
        size--;
    }

    T Get(int index) const {
        if (index < 0) return buffer[size + index];
        return buffer[index];
    }
    
    T* GetPtr(int index) const {
        if (index < 0) return &buffer[size + index];
        return &buffer[index];
    }
    
    int Find(T item) const {
        for (int i=0; i < size; i++) {
            if(buffer[i] == item) return i;
        }
        return -1;
    }

    int Count() const {
        return size;
    }

    void Init() {
        capacity = 0;
        size = 0;
        buffer = NULL;
    }

    void Clear() {
        for (int i=0; i < size; i++) {
            buffer[i].~T();  // Manually call constructor only if the whole buffer is freed
        }

        free(buffer);
        buffer = NULL;
        capacity = 0;
        size = 0;
    }

    List& operator=(const List<T>& other) {
        CopyList(&other, this);
        return *this;
    }

    T& operator[](int index) {
        return buffer[index];
    }

    T operator[](int index) const {
        return buffer[index];
    }

    void Sort(SortFn *fn) {
        if (size <= 1) return;
        _current_fn::_current_fn = (void*)fn;
        qsort(buffer, size, sizeof(T), _cmp_func);
        _current_fn::_current_fn = NULL;
    }

    static int _cmp_func(const void* a, const void* b) {
        if (_current_fn::_current_fn == NULL) {
            FAIL("May only be called inside of Sort");
        }
        //SHOW_I(*(T*)a)
        //SHOW_I(*(T*)b)
        return ((SortFn*)_current_fn::_current_fn)(*(T*)a, *(T*)b);
    }
};

template<typename T>
void CopyList(const List<T>* from, List<T>* to) {
    to->Resize(from->size);
    to->size = from->size;
    for (int i=0; i < from->size; i++) {
        to->buffer[i] = from->buffer[i];
    }
}

int ListTests();

#endif  // LIST_H