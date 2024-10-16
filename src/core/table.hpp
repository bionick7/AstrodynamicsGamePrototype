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

    Table() { Init(); }
    Table(const Table<T>& other) { 
        CopyTable(other, this);
    }
    ~Table() { Clear(); }

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
        uint64_t* new_hashes = new uint64_t[new_capacity];
        T* new_data = new T[new_capacity];
        for(int i=0; i < size; i++) {
            new_hashes[i] = hashes[i];
            new_data[i] = data[i];
        }
        delete[] hashes;
        delete[] data;
        hashes = new_hashes;
        data = new_data;
        capacity = new_capacity;
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
        size++;
        return size - 1;
    }

    void Clear() {
        delete[] hashes;
        delete[] data;
        hashes = NULL;
        data = NULL;
        size = 0;
        capacity = 0;
    }

    void Init() {
        size = 0;
        capacity = 0;
        hashes = NULL;
        data = NULL;
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