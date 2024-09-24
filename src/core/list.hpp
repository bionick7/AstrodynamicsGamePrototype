#ifndef LIST_H
#define LIST_H

#include <cstring>
#include <stdlib.h>
#include "logging.hpp"

// Reinventing the wheel, since I find stdlib stuff virtually impossible to debug

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
        capacity = 5;
        size = 0;
        buffer = new T[capacity];
    }

    List(int initial_capacity){
        capacity = initial_capacity;
        size = 0;
        buffer = new T[capacity];
    }

    List(const List& other) {
        capacity = other.capacity;
        size = other.size;
        buffer = new T[capacity];
        for(int i=0; i < size; i++) {
            buffer[i] = other[i];
        }
    }

    ~List() {
        delete[] buffer;
    }

    void Resize(int new_capacity) {
        if (new_capacity == 0) {
            delete[] buffer;
            buffer = NULL;
        } else {
            T* buffer2 = new T[new_capacity];
            memcpy(buffer2, buffer, sizeof(T) * size);
            delete[] buffer;
            buffer = buffer2;
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
    
    int Find(T item) const {
        for (int i=0; i < size; i++) {
            if(buffer[i] == item) return i;
        }
        return -1;
    }

    int Count() const {
        return size;
    }

    void Clear() {
        capacity = 5;
        size = 0;
        Resize(5);
    }

    List& operator=(const List& other) {
        capacity = other.size;
        size = other.size;
        if (capacity == 0) {
            buffer = NULL;    
        } else {
            buffer = new T[capacity];
            for(int i=0; i < size; i++) {
                buffer[i] = other[i];
            }
        }
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

int ListTests();

#endif  // LIST_H