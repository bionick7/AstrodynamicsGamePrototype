#ifndef DATA_NODE_H
#define DATA_NODE_H

#include "basic.hpp"

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

struct DataNode {
    static const DataNode Empty;
    bool IsReadOnly;

    DataNode();
    ~DataNode();
    DataNode(const DataNode& other);

    static void CopyTo(const DataNode& from, DataNode* to);

    static int FromFile(DataNode* out, const char* filepath, FileFormat fmt = FileFormat::Auto, bool isReadonly = false);
    //static std::vector<DataNode> ManyFromFile(const char* filepath, FileFormat fmt = FileFormat::Auto);
    //void ToFile(const char* filepath, FileFormat format);
    static int FromYaml(DataNode* dn, yaml_parser_t* yaml, bool isReadonly=false, int recursion_depth=0);
    void WriteJSON(std::ostream& os, int indentLevel=0) const;
    void WriteYAML(std::ostream& os, int indentLevel=0) const;

    bool Has(const char* key) const;
    void Remove(const char* key);
    void RemoveAt(const char* key, int index);

    void Set(const char* key, const char* value);
    void SetI(const char* key, int value);
    void SetF(const char* key, double value);
    void SetChild(const char* key, const DataNode& child);

    void SetArray(const char* key, size_t size);
    void SetArrayChild(const char* key, size_t size);

    void SetArrayElem(const char* key, int index, const char* value);
    void SetArrayElemI(const char* key, int index, int value);
    void SetArrayElemF(const char* key, int index, double value);
    void SetArrayElemChild(const char* key, int index, const DataNode& value);

    void AddArrayElem(const char* key, const char* value);
    void AddArrayElemChild(const char* key, const DataNode& value);

    const char* Get(const char* key, const char* def="", bool quiet=false) const;
    int GetI(const char* key, int def=0, bool quiet=false) const;
    double GetF(const char* key, double def=0, bool quiet=false) const;
    DataNode* GetChild(const char* key, bool quiet=false) const;

    size_t GetArrayLen(const char* key, bool quiet=false) const;
    size_t GetArrayChildLen(const char* key, bool quiet=false) const;
    const char* GetArray(const char* key, int index, const char* def = "", bool quiet=false) const;
    int GetArrayI(const char* key, int index, int def=0, bool quiet=false) const;
    double GetArrayF(const char* key, int index, double def=0, bool quiet=false) const;
    DataNode* GetArrayChild(const char* key, int index, bool quiet=false) const;

    size_t GetKeyCount() const;
    size_t GetChildCount() const;
    size_t GetArrayCount() const;
    size_t GetChildArrayCount() const;

    const char* GetKey(int index) const;
    const char* GetChildKey(int index) const;
    const char* GetArrayKey(int index) const;
    const char* GetChildArrayKey(int index) const;

    //static bool FieldEquals(std::string lhs, std::string rhs);

    void Inspect() {
        WriteYAML(std::cout);
    }

private:
    std::map<std::string, std::string> Fields;
    std::map<std::string, DataNode*> Children;
    std::map<std::string, std::vector<std::string>> FieldArrays;
    std::map<std::string, std::vector<DataNode>*> ChildArrays;
};

int DataNodeTests();

#endif   // DATA_NODE_H
