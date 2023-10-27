#include "data_map.hpp"

map<data_node_t, string> string_map;
map<data_node_t, vector<data_node_t>> array_map;
map<data_node_t, map<string, data_node_t>> map_map;

string _TryGetString(data_node_t index, string def) {
    auto string_find = string_map.find(index);
    if (string_find == string_map.end()) {
        return def;
    }
    return string_find->second;
}

vector<data_node_t> _TryGetArray(data_node_t index, bool* success) {
    auto array_find = array_map.find(index);
    *success = array_find == array_map.end();
    if (*success) {
        return vector<data_node_t>();
    }
    return array_find->second;
}

DataNode::DataNode(data_node_t p_index) {
    index = p_index;
    if (index != 0 && map_map.find(index) != map_map.end()) {
        index = 0;
    }
}

data_node_t DataNode::_GetChildIndex(string key) {
    auto map_find = map_map.find(index);
    if (map_find == map_map.end()) return 0;
    map<string, data_node_t> map = map_find->second;

    auto value_find = map.find(key);
    if (value_find == map.end()) return 0;
    return value_find->second;
}

DataNodeType DataNode::GetType(string key) {
    data_node_t child_index = _GetChildIndex(key);
    if (child_index == 0) return DNTYPE_INVALID;
    return (DataNodeType)(child_index >> 30);
}

DataNode DataNode::GetChild(string key, DataNode def) {
    data_node_t child_index = _GetChildIndex(key);
    if (!child_index) {
        return def;
    }
    return DataNode(child_index);
}

string DataNode::GetString(string key, string def) {
    data_node_t child_index = _GetChildIndex(key);
    if (!child_index) {
        return def;
    }
    return _TryGetString(child_index, def);
}

size_t DataNode::GetArrayLen(string key) {
    data_node_t child_index = _GetChildIndex(key);
    if (!child_index) {
        return 0;
    }

    bool success;
    vector<data_node_t> array = _TryGetArray(child_index, &success);
    if (!success) return 0;
    return array.size();
}

string DataNode::GetArrayItem(string key, size_t index, string def) {
    data_node_t child_index = _GetChildIndex(key);
    if (!child_index) {
        return def;
    }

    bool success;
    vector<data_node_t> array = _TryGetArray(child_index, &success);
    if (!success) return def;
    if (index >= array.size()) return def;
    return _TryGetString(array[index], def);
}

DataNode DataNode::GetArrayDataNodeItem(string key, size_t index, DataNode def) {
    data_node_t child_index = _GetChildIndex(key);
    if (!child_index) {
        return def;
    }

    bool success;
    vector<data_node_t> array = _TryGetArray(child_index, &success);
    if (!success) return def;
    if (index >= array.size()) return def;
    return DataNode(array[index]);
}