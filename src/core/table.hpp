#ifndef TABLE_H
#define TABLE_H

#include "basic.hpp"

struct TableKey {
    StrHash h;
    const char* txt;  // not owning
    
    // Hashes at compile time
    constexpr TableKey(const char* text) : 
        h(HashKey(text)), txt(text) {  }
};

template<typename T>
struct Table { 
    StrHash* hashes = NULL;
    T* data = NULL;

    int size = 0;
    int capacity = 0;

    Table() {
        Init();
    }

    Table(const Table<T>& other) { 
        Init();
        CopyTable(other, this);
    }

    ~Table() { 
        Clear(); 
    }

    int Find(TableKey key) const {
        // TODO: Faster as binary search tree ig
        for (int i=0; i < size; i++) {
            if (key.h == hashes[i]) {
                return i;
            }
        }
        return -1;
    }

    void Remove(TableKey key) {
        // Doesn't recycle memory
        // If remove is used excessively, effectively a memory leak
        // Will still persist, if manually iterated over
        int find = Find(key);
        if (find < 0) 
            return;
        hashes[find] = 0;
    }

    void Resize(int new_capacity) {
        if (new_capacity == 0) {
            Clear();
        } else {
            hashes = (hashes == NULL) ? (uint64_t*) malloc(sizeof(uint64_t) * new_capacity)
                                      : (uint64_t*) realloc(hashes, sizeof(uint64_t) * new_capacity);
            data   = (data   == NULL) ? (T*) malloc(sizeof(T) * new_capacity)
                                      : (T*) realloc(data, sizeof(T) * new_capacity);
            capacity = new_capacity;
        }
    }

    int Insert(TableKey key, T value) {
        int index = AllocForInsertion(key);
        data[index] = value;
        return index;
    }

    int AllocForInsertion(TableKey key) {
        if (size + 1 > capacity) {
            Resize(capacity + 10);
        }
        hashes[size] = key.h;
        new (&data[size]) T();
        size++;
        return size - 1;
    }

    void Init() {
        size = 0;
        capacity = 0;
        hashes = NULL;
        data = NULL;
    }

    void Clear() {
        for(int i=0; i < size; i++) {
            data[i].~T();
        }
        free(hashes);
        free(data);
        hashes = NULL;
        data = NULL;
        size = 0;
        capacity = 0;
    }

    T& operator[](TableKey key) {
        int find = Find(key);
        if (find < 0) {
            return T();
        }
        return data[find];
    }

    T operator[](TableKey key) const {
        return (*this)[key];
    }

    Table& operator=(const Table<T>& other) {
        CopyTable(&other, this);
        return *this;
    }
};

template<typename T>
void CopyTable(const Table<T>* from, Table<T>* to) {
    to->Resize(from->size);
    to->size = from->size;
    for (int i=0; i < from->size; i++) {
        to->data[i] = from->data[i];
        to->hashes[i] = from->hashes[i];
    }
}

#endif  // TABLE_H 