#ifndef DATA_HANDLING_H
#define DATA_HANDLING_H

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
/*
template<typename T>
static bool TypeSupported(T s) {
    return (std::is_same<T, bool>::value
        || std::is_same<T, std::string>::value
        || std::is_integral<T>::value
        || std::is_floating_point<T>::value
    );
}

template<typename T>
static bool CanConvertTo(std::string s) {
    if (!TypeSupported<T>()) {
        return false;
    }
    if constexpr (std::is_same<T, bool>::value) {
        return ...;
    }
    if constexpr (std::is_same<T, std::string>::value) {
        return s;
    }
    if constexpr (std::is_integral<T>::value) {
        return ...;
    }
    if constexpr (std::is_floating_point<T>::value) {
        return ...;
    }
    return true;
}


template<typename T>
static T TryConvertTo(std::string s, T* def, bool* sucess) {
    if (!TypeSupported<T>()) {
        sucess = false;
        return def;
    }
    sucess = true;
    if constexpr (std::is_same<T, bool>::value) {
        return ...;
    }
    if constexpr (std::is_same<T, std::string>::value) {
        return s;
    }
    if constexpr (std::is_integral<T>::value) {
        return ...;
    }
    if constexpr (std::is_floating_point<T>::value) {
        return ...;
    }

    sucess = false;
    return def;
}

template<typename T>
static std::string ConvertFrom(T s) {
    if (!TypeSupported(typeid(T))) return "";
    if constexpr (std::is_same<T, bool>::value) return s ? "yes" : "no";
    if constexpr (std::is_same<T, std::string>::value) return s;
    if constexpr (std::is_integral<T>::value) NOT_IMPLEMENTED();
    if constexpr (std::is_floating_point<T>::value) NOT_IMPLEMENTED();
    return s.ToString();
}
*/


class DataNode {
public:
    static const DataNode Empty;
    bool IsReadOnly;

    DataNode();
    ~DataNode();
    DataNode(const DataNode& other);

    static DataNode FromFile(const char* filepath, FileFormat fmt = FileFormat::Auto, bool isReadonly = false);
    static std::vector<DataNode> ManyFromFile(const char* filepath, FileFormat fmt = FileFormat::Auto);
    //void ToFile(std::string filepath);
    std::string Serialize(FileFormat format) const;
    std::string WriteJSON() const;
    std::string WriteYAML() const;
    static int FromYaml(DataNode* dn, yaml_parser_t* yaml, bool isReadonly=false, int recursion_depth=0);
    //static DataNode FromYamlString(std::string s, bool isReadonly = false);
    //static DataNode FromJson(const JsonObject& node, bool isReadonly = false);

    bool Has(const std::string key) const;
    void Remove(const std::string key);

    void Set(std::string key, std::string value);
    void SetI(std::string key, int value);
    void SetF(std::string key, double value);
    void SetChild(std::string key, const DataNode& child);

    void SetArray(std::string key, size_t size);
    void SetArrayChild(std::string key, size_t size);

    void SetArrayElem(std::string key, int index, std::string value);
    void SetArrayElemI(std::string key, int index, int value);
    void SetArrayElemF(std::string key, int index, double value);
    void SetArrayElemChild(std::string key, int index, const DataNode& value);

    std::string Get(std::string key, std::string def = "", bool quiet = false) const;
    int GetI(std::string key, int def = 0, bool quiet = false) const;
    double GetF(std::string key, double def = 0, bool quiet = false) const;
    DataNode* GetChild(std::string key, bool quiet = false) const;

    size_t GetArrayLen(std::string key, bool quiet) const;
    std::string GetArray(std::string key, int index, std::string def = "", bool quiet = false) const;
    int GetArrayI(std::string key, int index, int def = 0, bool quiet = false) const;
    double GetArrayF(std::string key, int index, double def = 0, bool quiet = false) const;
    DataNode* GetArrayChild(std::string key, int index, bool quiet = false) const;

    size_t GetChildCount();
    size_t GetChildArrayCount();

    std::string GetChildKey(int index);
    std::string GetChildArrayKey(int index);

    static bool FieldEquals(std::string lhs, std::string rhs);

    // static DataNode GetDifferenceDict(const DataNode& base, const DataNode& target, bool recursive = true, bool strict = true);
    // static DataNode ApplyDifferenceDict(const DataNode& base, const DataNode& diff, bool recursive = true);
    // DataNode FindChildRecursively(std::function<bool(const DataNode&)> check);
    //bool operator==(const DataNode& other) const;
    //bool operator!=(const DataNode& other) const;
    //friend std::ostream& operator<<(std::ostream& os, const DataNode& dt) {
    //    os << dt.WriteYAML();
    //    return os;
    //}

private:
    std::map<std::string, std::string> Fields;
    std::map<std::string, DataNode*> Children;
    std::map<std::string, std::vector<std::string>> FieldArrays;
    std::map<std::string, std::vector<DataNode>*> ChildArrays;

    static void WriteJSONInternal(std::ostream& os, const DataNode* node, int indentLevel);
    static void WriteYAMLInternal(std::ostream& os, const DataNode* node, int indentLevel);
};

int DataNodeTests();

#endif   // DATA_HANDLING_H