
#ifndef DATAMAP_H
#define DATAMAP_H
#include "basic.hpp"
#include <map>
#include <vector>
#include <string>

using namespace std;

typedef uint64_t data_node_t;

ENUM_DECL(DataNodeType) {
    DNTYPE_INVALID = -1,
    DNTYPE_STRING_NODE = 0,
    DNTYPE_DATA_NODE = 1,
    DNTYPE_LIST_NODE = 2,
    DNTYPE_DATA_LIST_NODE = 3,
};

static inline bool DataNodeTypeIsArray(DataNodeType t) { return t >> 1; }

class DataNode {
public:
    data_node_t _GetChildIndex(string key);
    data_node_t index;

    bool is_valid() { return index == 0; }
    DataNodeType GetType(string key);
    DataNode(data_node_t p_index);
    DataNode GetChild(string key, DataNode def=DataNode(0));
    string GetString(string key, string def="");
    size_t GetArrayLen(string key);
    string GetArrayItem(string key, size_t index, string def="");
    DataNode GetArrayDataNodeItem(string key, size_t index, DataNode def=DataNode(0));
};

#endif // DATAMAP_H