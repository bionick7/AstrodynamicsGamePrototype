#ifndef DATA_NODE_H
#define DATA_NODE_H

#include "time.hpp"
#include "table.hpp"
#include "string_builder.hpp"
#include "id_allocator.hpp"
#include "list.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#include <yaml.h>

namespace file_format {
    enum T { JSON, YAML, CSV, AUTO };
}

struct StringBuilder;

struct DataNode {
    struct FlexString { 
        int offset = 0;

        FlexString() = default;
        FlexString(const char* txt, DataNode* dn);
        const char* GetChar(const DataNode* dn);
    };

    int __destructor_calls = 0;

    struct Field      { FlexString key; FlexString value; };
    struct Child      { FlexString key; DataNode* value; };
    struct FieldArray { FlexString key; List<FlexString> strings; };
    struct ChildArray { FlexString key; List<DataNode> nodes; };

    bool is_read_only;
    StringBuffer text_buffer;

    Table<Field> fields;  // String offsets
    Table<Child> children;   // Indices into static arra
    Table<FieldArray> field_arrays;  // String offsets
    Table<ChildArray> child_arrays;   // Slices in static array

    DataNode();
    ~DataNode();
    DataNode(const DataNode& other);
    DataNode(DataNode&& other) = default;

    static void CopyTo(const DataNode* from, DataNode* to);

    static int FromMemory(DataNode* out, const char* origin, const char* text,
                          file_format::T fmt = file_format::AUTO, bool isReadonly = false, bool quiet = false);
    static int FromFile(DataNode* out, const char* filepath, 
                        file_format::T fmt = file_format::AUTO, bool isReadonly = false, bool quiet = false);
    static int FromYaml(DataNode* dn, const char* filepath, yaml_parser_t* yaml, bool isReadonly=false, int recursion_depth=0);
    //void WriteJSON(StringBuilder* os, int indentLevel=0) const;
    void WriteYAML(StringBuilder* os, int indentLevel=0, bool ignore_first_indent=false) const;
    void WriteToFile(const char* file, file_format::T fmt) const;

    bool Has(TableKey key) const;
    bool HasChild(TableKey key) const;
    bool HasArray(TableKey key) const;
    bool HasChildArray(TableKey key) const;
    void Remove(TableKey key);
    void RemoveAt(TableKey key, int index);

    void Set(TableKey key, const char* value);
    void SetI(TableKey key, int value);
    void SetF(TableKey key, double value);
    void SetDate(TableKey key, timemath::Time value);
    DataNode* SetChild(TableKey key);

    void CreateArray(TableKey key, size_t size);
    void CreatChildArray(TableKey key, size_t size);

    void InsertIntoArray(TableKey key, int index, const char* value);
    void InsertIntoArrayI(TableKey key, int index, int value);
    void InsertIntoArrayF(TableKey key, int index, double value);
    DataNode* InsertIntoChildArray(TableKey key, int index);

    void AppendToArray(TableKey key, const char* value);
    DataNode* AppendToChildArray(TableKey key);

    const char* Get(TableKey key, const char* def="", bool quiet=false) const;
    long GetI(TableKey key, long def=0, bool quiet=false) const;
    double GetF(TableKey key, double def=0, bool quiet=false) const;
    timemath::Time GetDate(TableKey key, timemath::Time def=0, bool quiet=false) const;
    DataNode* GetChild(TableKey key, bool quiet=false) const;

    size_t GetArrayLen(TableKey key, bool quiet=false) const;
    size_t GetChildArrayLen(TableKey key, bool quiet=false) const;
    const char* GetArrayElem(TableKey key, int index, const char* def = "", bool quiet=false) const;
    long GetArrayElemI(TableKey key, int index, long def=0, bool quiet=false) const;
    double GetArrayElemF(TableKey key, int index, double def=0, bool quiet=false) const;
    DataNode* GetChildArrayElem(TableKey key, int index, bool quiet=false) const;

    size_t GetFieldCount() const;
    size_t GetChildCount() const;
    size_t GetArrayCount() const;
    size_t GetChildArrayCount() const;

    const char* GetKey(int index) const;
    const char* GetChildKey(int index) const;
    const char* GetArrayKey(int index) const;
    const char* GetChildArrayKey(int index) const;

    void SerializeBuffer(TableKey key, const int buffer[], const char* names[], int buffer_size, bool skip_zeros=true);
    void DeserializeBuffer(TableKey key, int buffer[], const char* names[], int buffer_size) const;
    template<typename T, EntityType E>
    void SerializeAllocList(IDAllocatorList<T,E>* list, TableKey key, void fn(DataNode*, const T*)) {
        CreatChildArray(key, list->alloc_count);
        for(auto it = list->GetIter(); it; it++) {
            DataNode* child = InsertIntoChildArray(key, it.counter);
            child->SetI("id", it.GetId().AsInt());
            fn(child, list->Get(it));
        }
    }

    template<typename T, EntityType E>
    void DeserializeAllocList(IDAllocatorList<T,E>* list, TableKey key, void fn(const DataNode*, T*)) const {
        list->Clear();
        for(int i=0; i < GetChildArrayLen(key); i++) {
            DataNode* child = GetChildArrayElem(key, i);
            T* d;
            list->AllocateAtID(RID(child->GetI("id")), &d);
            fn(child, d);
        }
    }
    
    //static bool FieldEquals(std::string lhs, std::string rhs);

    void Inspect() const;
};

int DataNodeTests();

#endif   // DATA_NODE_H
