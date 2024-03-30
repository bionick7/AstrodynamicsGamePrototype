#ifndef DATA_NODE_H
#define DATA_NODE_H

#include "basic.hpp"
#include "time.hpp"

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <sstream>

#include <yaml.h>

enum class FileFormat {
    JSON,
    YAML,
    CSV,
    Auto
};

struct StringBuilder;

struct DataNode {
    static const DataNode Empty;
    bool IsReadOnly;

    DataNode();
    ~DataNode();
    DataNode(const DataNode& other);

    static void CopyTo(const DataNode* from, DataNode* to);

    static int FromMemory(DataNode* out, const char* origin, const char* text,
                          FileFormat fmt = FileFormat::Auto, bool isReadonly = false, bool quiet = false);
    static int FromFile(DataNode* out, const char* filepath, 
                        FileFormat fmt = FileFormat::Auto, bool isReadonly = false, bool quiet = false);
    //static std::vector<DataNode> ManyFromFile(const char* filepath, FileFormat fmt = FileFormat::Auto);
    //void ToFile(const char* filepath, FileFormat format);
    static int FromYaml(DataNode* dn, const char* filepath, yaml_parser_t* yaml, bool isReadonly=false, int recursion_depth=0);
    void WriteJSON(StringBuilder* os, int indentLevel=0) const;
    void WriteYAML(StringBuilder* os, int indentLevel=0, bool ignore_first_indent=false) const;
    void WriteToFile(const char* file, FileFormat fmt) const;

    bool Has(const char* key) const;
    bool HasChild(const char* key) const;
    bool HasArray(const char* key) const;
    bool HasChildArray(const char* key) const;
    void Remove(const char* key);
    void RemoveAt(const char* key, int index);

    void Set(const char* key, const char* value);
    void SetI(const char* key, int value);
    void SetF(const char* key, double value);
    void SetDate(const char* key, timemath::Time value);
    DataNode* SetChild(const char* key);

    void CreateArray(const char* key, size_t size);
    void CreatChildArray(const char* key, size_t size);

    void InsertIntoArray(const char* key, int index, const char* value);
    void InsertIntoArrayI(const char* key, int index, int value);
    void InsertIntoArrayF(const char* key, int index, double value);
    DataNode* InsertIntoChildArray(const char* key, int index);

    void AppendToArray(const char* key, const char* value);
    DataNode* AppendToChildArray(const char* key);

    const char* Get(const char* key, const char* def="", bool quiet=false) const;
    long GetI(const char* key, long def=0, bool quiet=false) const;
    double GetF(const char* key, double def=0, bool quiet=false) const;
    timemath::Time GetDate(const char* key, timemath::Time def=0, bool quiet=false) const;
    DataNode* GetChild(const char* key, bool quiet=false) const;

    size_t GetArrayLen(const char* key, bool quiet=false) const;
    size_t GetChildArrayLen(const char* key, bool quiet=false) const;
    const char* GetArrayElem(const char* key, int index, const char* def = "", bool quiet=false) const;
    long GetArrayElemI(const char* key, int index, long def=0, bool quiet=false) const;
    double GetArrayElemF(const char* key, int index, double def=0, bool quiet=false) const;
    DataNode* GetChildArrayElem(const char* key, int index, bool quiet=false) const;

    size_t GetFieldCount() const;
    size_t GetChildCount() const;
    size_t GetArrayCount() const;
    size_t GetChildArrayCount() const;

    void SerializeBuffer(const char* name, const int buffer[], const char* names[], int buffer_size, bool skip_zeros=true);
    void DeserializeBuffer(const char* name, int buffer[], const char* names[], int buffer_size) const;

    const char* GetKey(int index) const;
    const char* GetChildKey(int index) const;
    const char* GetArrayKey(int index) const;
    const char* GetChildArrayKey(int index) const;

    //static bool FieldEquals(std::string lhs, std::string rhs);

    void Inspect() const;

private:
    std::map<std::string, std::string> Fields;
    std::map<std::string, DataNode*> Children;
    std::map<std::string, std::vector<std::string>> FieldArrays;
    std::map<std::string, std::vector<DataNode>*> ChildArrays;
};

int DataNodeTests();

#endif   // DATA_NODE_H
