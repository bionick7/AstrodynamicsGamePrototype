#include "datahandling.hpp"

const DataNode DataNode::Empty;


bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

DataNode::DataNode() {
    Fields = std::map<std::string, std::string>();
    Children = std::map<std::string, DataNode*>();
    FieldArrays = std::map<std::string, std::vector<std::string>>();
    ChildArrays = std::map<std::string, std::vector<DataNode>*>();
    IsReadOnly = false;
}

DataNode::DataNode(const DataNode& other) {
    CopyTo(other, this);
}

DataNode::~DataNode() {
    for (auto kv = Children.begin(); kv != Children.end(); kv++) {
        delete kv->second;
    }
    for (auto kv = ChildArrays.begin(); kv != ChildArrays.end(); kv++) {
        delete kv->second;
    }
}

void DataNode::CopyTo(const DataNode& from, DataNode* to) {
    for (auto kv = from.Fields.begin(); kv != from.Fields.end(); kv++) {
        to->Set(kv->first.c_str(), kv->second.c_str());
    }
    for (auto kv = from.Children.begin(); kv != from.Children.end(); kv++) {
        to->SetChild(kv->first.c_str(), *kv->second);
    }
    // These are very unoptimizer right now with individual lookups for each element insertion
    for (auto kv = from.FieldArrays.begin(); kv != from.FieldArrays.end(); kv++) {
        to->SetArray(kv->first.c_str(), kv->second.size());
        for (size_t i=0; i < kv->second.size(); i++) {
            to->SetArrayElem(kv->first.c_str(), i, kv->second[i].c_str());
        }
    }
    for (auto kv = from.ChildArrays.begin(); kv != from.ChildArrays.end(); kv++) {
        to->SetArrayChild(kv->first.c_str(), kv->second->size());
        for (size_t i=0; i < kv->second->size(); i++) {
            to->SetArrayElemChild(kv->first.c_str(), i, kv->second->at(i));
        }
    }
}

int _YamlParse(DataNode* node, const char* filepath) {
    yaml_parser_t parser;

    yaml_parser_initialize(&parser);
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        printf("No such file: %s\n", filepath);
        return 1;
    }
    yaml_parser_set_input_file(&parser, file);

    // consume until you reach map_start
    yaml_event_t event;
    while (true) {
        if (!yaml_parser_parse(&parser, &event)) break;
        if (event.type == YAML_MAPPING_START_EVENT) break;
    }
    yaml_event_delete(&event);

    int status = DataNode::FromYaml(node, &parser, false, 0);

    yaml_parser_delete(&parser);
    fclose(file);
    return status;
}

DataNode DataNode::FromFile(const char* filepath, FileFormat fmt, bool isReadonly) {
    switch (fmt) {
        //case FileFormat::Auto: {
        //    if (strcmp(filepath + strlen(filepath) - 5, ".json")) {
        //        fmt = FileFormat::JSON;
        //    } else if (strcmp(filepath + strlen(filepath) - 4, ".csv")) {
        //        fmt = FileFormat::CSV;
        //    } else if (strcmp(filepath + strlen(filepath) - 5, ".yaml")) {
        //        fmt = FileFormat::YAML;
        //    } else {
        //        return DataNode();
        //    }
        //    return FromFile(filepath, fmt, isReadonly);
        //}
        case FileFormat::YAML: {
            DataNode res = DataNode();
            _YamlParse(&res, filepath);
            return res;
        }
        case FileFormat::CSV:
        default: {
            return DataNode();
        }
    }
}

std::vector<DataNode> DataNode::ManyFromFile(const char* filepath, FileFormat fmt) {
    std::vector<DataNode> result;

    //if (fmt == FileFormat::Auto) {
    //    if (hasEnding(filepath, ".json")) {
    //        fmt = FileFormat::JSON;
    //    } else if (hasEnding(filepath, ".csv")) {
    //        fmt = FileFormat::CSV;
    //    } else {
    //        fmt = FileFormat::YAML;
    //    }
    //}

    switch (fmt) {
        case FileFormat::YAML: {
            break;
        }
        case FileFormat::CSV: {
            // Implementation for reading CSV files goes here
            break;
        }
        default:
            break;
    }

    return result;
}

int DataNode::FromYaml(DataNode* node, yaml_parser_t* parser, bool is_readonly, int recursion_depth) {
    node->IsReadOnly = is_readonly;
    yaml_event_t event;

    bool expects_key = true;
    char key_name[1024];

    bool done = false;
    while (!done) {

        // Get the next event.
        if (!yaml_parser_parse(parser, &event)){
            goto error;
        }

        switch(event.type) {
            default:
            case YAML_STREAM_START_EVENT:
            case YAML_DOCUMENT_START_EVENT:
            case YAML_DOCUMENT_END_EVENT:
                break;  // Ignore
            case YAML_STREAM_END_EVENT:{
                goto error;
            }
            case YAML_SCALAR_EVENT: {
                char* scalar_value = (char*) event.data.scalar.value;
                if (expects_key) {
                    strcpy(key_name, scalar_value);
                    expects_key = false;
                } else {
                    node->Set(key_name, scalar_value);
                    expects_key = true;
                }
                break;
            }
            case YAML_SEQUENCE_START_EVENT:{
                NOT_IMPLEMENTED
                break;
            }
            case YAML_SEQUENCE_END_EVENT:{
                NOT_IMPLEMENTED
                expects_key = true;
                break;
            }
            case YAML_MAPPING_START_EVENT:{
                // Recurse
                DataNode child_node = DataNode();
                int status = DataNode::FromYaml(&child_node, parser, is_readonly, recursion_depth+1);
                if (status != 0) {
                    goto error;
                }
                node->SetChild(key_name, child_node);
                expects_key = true;
                break;
            }
            case YAML_MAPPING_END_EVENT:
                yaml_event_delete(&event);
                return 0;
        }

        done = (event.type == YAML_STREAM_END_EVENT);
        yaml_event_delete(&event);
    }

    error:
    yaml_event_delete(&event);
    return 1;
}

// Helper function to write DataNode to JSON
void DataNode::WriteJSON(std::ostream& os, int indentLevel) const {
    std::string indent(indentLevel * 4, ' ');

    os << "{\n";
    for (auto it = Fields.begin(); it != Fields.end(); ++it) {
        os << indent << "  \"" << it->first << "\": \"" << it->second << "\"";
        if (std::next(it) != Fields.end() || !Children.empty() || !FieldArrays.empty() || !ChildArrays.empty()) {
            os << ",";
        }
        os << "\n";
    }

    for (auto it = Children.begin(); it != Children.end(); ++it) {
        os << indent << "  \"" << it->first << "\": ";
        it->second->WriteJSON(os, indentLevel + 1);
        if (std::next(it) != Children.end() || !FieldArrays.empty() || !ChildArrays.empty()) {
            os << ",";
        }
        os << "\n";
    }

    for (auto it = FieldArrays.begin(); it != FieldArrays.end(); ++it) {
        os << indent << "  \"" << it->first << "\": [\n";
        for (const auto& item : it->second) {
            os << indent << "    \"" << item << "\"";
            if (&item != &it->second.back()) {
                os << ",";
            }
            os << "\n";
        }
        os << indent << "  ]";
        if (std::next(it) != FieldArrays.end() || !ChildArrays.empty()) {
            os << ",";
        }
        os << "\n";
    }

    for (auto it = ChildArrays.begin(); it != ChildArrays.end(); ++it) {
        os << indent << "  \"" << it->first << "\": [\n";
        for (const auto& item : *it->second) {
            os << indent << "    ";
            item.WriteJSON(os, indentLevel + 1);
            if (&item != &it->second->back()) {
                os << ",";
            }
            os << "\n";
        }
        os << indent << "  ]";
        if (std::next(it) != ChildArrays.end()) {
            os << ",";
        }
        os << "\n";
    }

    os << indent << "}";
}

// Helper function to write DataNode to YAML
void DataNode::WriteYAML(std::ostream& os, int indentLevel) const {
    char* indent = (char*)malloc(sizeof(char) * indentLevel * 2 + 1);
    for (int i=0; i < indentLevel*2; i++) indent[i] = ' ';
    indent[indentLevel*2] = '\x00';

    //os << "Fields: " << Fields.size() << " -- Children: " << Children.size() << " -- Field Arrays " << FieldArrays.size() << std::endl;

    for (auto it = Fields.begin(); it != Fields.end(); ++it) {
        os << indent << it->first << ": " << it->second << "\n";
    }

    for (auto it = Children.begin(); it != Children.end(); ++it) {
        os << indent << it->first << ":\n";
        it->second->WriteYAML(os, indentLevel + 1);
    }

    for (auto it = FieldArrays.begin(); it != FieldArrays.end(); ++it) {
        os << indent << it->first << ":\n";
        for (const auto& item : it->second) {
            os << indent << "  - " << item << "\n";
        }
    }

    for (auto it = ChildArrays.begin(); it != ChildArrays.end(); ++it) {
        os << indent << it->first << ":\n";
        for (const auto& item : *it->second) {
            item.WriteYAML(os, indentLevel + 1);
        }
    }
    os << std::endl;

    free(indent);
}

/*******************************
 * GETTERS
 * ***************************/


const char* DataNode::Get(const char* key, const char* def, bool quiet) const {
    auto it = Fields.find(key);
    if (it != Fields.end()) {
        return it->second.c_str();
    }
    if (!quiet) {
        std::cerr << "Key " << key << " not found" << std::endl;
    }
    return def;
}

int DataNode::GetI(const char* key, int def, bool quiet) const {
    char *p; 
    int res = (int) strtol(Get(key, "", quiet), &p, 10);
    return (p - key) == 0 ? def : res;
}

double DataNode::GetF(const char* key, double def, bool quiet) const {
    char *p; 
    double res = strtod(Get(key, "", quiet), &p);
    return (p - key) == 0 ? def : res;
}

DataNode* DataNode::GetChild(const char* key, bool quiet) const {
    auto it = Children.find(key);
    if (it != Children.end()) {
        return it->second;
    }
    if (!quiet) {
        std::cerr << "Key " << key << " not found" << std::endl;
    }
    return NULL;
}

const char* DataNode::GetArray(const char* key, int index, const char* def, bool quiet) const {
    auto it = FieldArrays.find(key);
    if (it != FieldArrays.end()) {
        if (index < it->second.size()) {
            return it->second[index].c_str();
        }
    }
    if (!quiet) {
        std::cerr << "Key " << key << " not found" << std::endl;
    }
    return def;
}

int DataNode::GetArrayI(const char* key, int index, int def, bool quiet) const {
    char *p; 
    int res = (int) strtol(GetArray(key, index, "", quiet), &p, 10);
    return *p ? res : def;
}

double DataNode::GetArrayF(const char* key, int index, double def, bool quiet) const {
    char *p; 
    int res = (int) strtod(GetArray(key, index, "", quiet), &p);
    return *p ? res : def;
}

DataNode* DataNode::GetArrayChild(const char* key, int index, bool quiet) const {
    auto it = ChildArrays.find(key);
    if (it != ChildArrays.end()) {
        if (index < it->second->size()) {
            return &it->second->at(index);
        }
    }
    if (!quiet) {
        std::cerr << "Key " << key << " not found" << std::endl;
    }
    return NULL;
}


size_t DataNode::GetChildCount() {
    return Children.size();
}

size_t DataNode::GetChildArrayCount() {
    return ChildArrays.size();
}

size_t DataNode::GetArrayLen(const char* key, bool quiet) const {
    auto it = FieldArrays.find(key);
    if (it != FieldArrays.end()) {
        return it->second.size();
    } else {
        return 0;
    }
    if (!quiet) {
        std::cerr << "Key " << key << " not found" << std::endl;
    }
}

const char* DataNode::GetChildKey(int index) {
    if (index < 0 || index >= Children.size()) return "[Out Of Bounds]";
    auto it = Children.begin();
    for (int i=0; i < index; i++) it++;
    return it->first.c_str();
}

const char* DataNode::GetChildArrayKey(int index) {
    if (index < 0 || index >= ChildArrays.size()) return "[Out Of Bounds]";
    auto it = ChildArrays.begin();
    for (int i=0; i < index; i++) it++;
    return it->first.c_str();
}

/***********************************
 * SETTERS
 * **********************************/

void DataNode::Set(const char* key, const char* val) {
    if (IsReadOnly) {
        std::cerr << "Trying to set data on a readonly datanode" << std::endl;
        return;
    }
    Fields.insert_or_assign(std::string(key), std::string(val));
}

void DataNode::SetI(const char* key, int value) {
    char buffer[100];
    sprintf(buffer, "%d", value);
    Set(key, buffer);
}

void DataNode::SetF(const char* key, double value) {
    char buffer[100];
    sprintf(buffer, "%f", value);
    Set(key, buffer);
}

void DataNode::SetChild(const char* key, const DataNode& val) {
    // Adds a copy to the DataNode to the map
    //DataNode* child = (DataNode*) malloc(sizeof(DataNode));
    //CopyTo(val, child);
    DataNode* child = new DataNode(val);
    if (IsReadOnly) {
        std::cerr << "Trying to set data on a readonly datanode" << std::endl;
        return;
    }
    
    Children.insert_or_assign(std::string(key), child);
}

void DataNode::SetArray(const char* key, size_t size) {
    if (IsReadOnly) {
        std::cerr << "Trying to set data on a readonly datanode" << std::endl;
        return;
    }
    auto find = FieldArrays.find(key);
    if (find == FieldArrays.end()) {
        find->second = std::vector<std::string>();
    }
    find->second.resize(size);
}

void DataNode::SetArrayElem(const char* key, int index, const char* value) {
    if (IsReadOnly) {
        std::cerr << "Trying to set data on a readonly datanode" << std::endl;
        return;
    }
    auto find = FieldArrays.find(key);
    if (find == FieldArrays.end()) {
        std::cerr << "No such key " << key << std::endl;
        return;
    }
    if (index < 0 || index >= find->second.size()) {
        std::cerr << "Invalid index " << index << std::endl;
        return;
    }
    find->second[index] = value;
}

void DataNode::SetArrayChild(const char* key, size_t size) {
    if (IsReadOnly) {
        std::cerr << "Trying to set data on a readonly datanode" << std::endl;
        return;
    }
    auto find = ChildArrays.find(key);
    if (find == ChildArrays.end()) {
        std::vector<DataNode>* arr = new std::vector<DataNode>();
        find->second = arr;
    }
    find->second->resize(size);
}

void DataNode::SetArrayElemI(const char* key, int index, int value) {
    char buffer[100];
    sprintf(buffer, "%d", value);
    SetArrayElem(key, index, buffer);
}

void DataNode::SetArrayElemF(const char* key, int index, double value) {
    char buffer[100];
    sprintf(buffer, "%f", value);
    SetArrayElem(key, index, buffer);
}

void DataNode::SetArrayElemChild(const char* key, int index, const DataNode& value) {
    if (IsReadOnly) {
        std::cerr << "Trying to set data on a readonly datanode" << std::endl;
        return;
    }
    auto find = ChildArrays.find(key);
    if (find == ChildArrays.end()) {
        std::cerr << "No such key " << key << std::endl;
        return;
    }
    if (index < 0 || index >= find->second->size()) {
        std::cerr << "Invalid index " << index << std::endl;
        return;
    }
    find->second->at(index) = DataNode(value);
}

bool DataNode::Has(const char* key) const {
    return Fields.count(key) > 0 || Children.count(key) > 0 || FieldArrays.count(key) > 0 || ChildArrays.count(key) > 0;
}

void DataNode::Remove(const char* key) {
    if (IsReadOnly) {
        std::cout << "Cannot modify read-only DataNode" << std::endl;
        return;
    }

    Fields.erase(key);
    FieldArrays.erase(key);
    auto it_children = Children.find(key);
    if (it_children != Children.end()) {
        free(it_children->second);
        Children.erase(it_children);
    }
    Children.erase(key);
    auto it_children_arr = ChildArrays.find(key);
    if (it_children_arr != ChildArrays.end()) {
        free(it_children_arr->second);
        ChildArrays.erase(it_children_arr);
    }
    ChildArrays.erase(key);
}

void RemoveAt(const char* key, int index) {
    NOT_IMPLEMENTED
}

//bool DataNode::FieldEquals(std::string lhs, std::string rhs) {
//    if (lhs == rhs)
//        return true;
//    //if (double.TryParse(lhs, out double lhsDouble) && double.TryParse(rhs, out double rhsDouble))
//    //    return lhsDouble == rhsDouble;
//    return false;
//}

//bool DataNode::operator==(const DataNode& other) const {
//    NOT_IMPLEMENTED;
//}
//
//bool DataNode::operator!=(const DataNode& other) const {
//    return !(*this == other);
//}

#define DN_TEST_FAIL(msg, exit_code) {printf("DataNodeTest failed with: %s\n", msg); return exit_code;}
#define DN_TEST_ASSERTKV(node, key, value) if(strcmp(node.Get(key), value) != 0){ \
    printf("DataNodeTest failed with: %s != %s\n", key, value); \
    return 1; \
}

int DataNodeTests() {
    DataNode node = DataNode();
    int status;
    status = _YamlParse(&node, "resources/data/test_data/inexistiant_file.yaml");
    if (status != 1) DN_TEST_FAIL("inexistant file did not fail", 1);
    //_YamlParse(&node, "resources/data/test_data/readonly_file.yaml");
    //if (status != 1) return 1;
    status = _YamlParse(&node, "resources/data/test_data/test_data.yaml");
    if (status != 0) DN_TEST_FAIL("test_data could not be loaded", 1);

    node.Inspect();

    // Test Getters

    DN_TEST_ASSERTKV(node, "key1", "value1");
    DN_TEST_ASSERTKV(node, "key  2", "value2");
    DN_TEST_ASSERTKV(node, "key3", "value3");
    if (node.GetF("float_val") != 5.67e-8) DN_TEST_FAIL("Did not find float_val", 1);
    if (node.GetI("int_val") != -589) DN_TEST_FAIL("Did not find int_val", 1);

    DataNode* child = node.GetChild("child1");
    if (strcmp(child->Get("key1"), "child_value1") != 0) DN_TEST_FAIL("Did not find key1 in child", 1);
    if (strcmp(child->Get("key  2"), "child value 2\n") != 0) DN_TEST_FAIL("Did not find key  2 in child", 1);

    if (node.GetArrayLen("array1") != 4) DN_TEST_FAIL("array1 wrong size", 1);
    if (strcmp(node.GetArray("array1", 0), "item2") != 0) DN_TEST_FAIL("array1 error", 1);
    if (strcmp(node.GetArray("array1", 1), "item1") != 0) DN_TEST_FAIL("array1 error", 1);
    if (node.GetArrayF("array1", 2) != +5.67e-8) DN_TEST_FAIL("array1 error", 1);
    if (node.GetArrayI("array1", 3) != -589) DN_TEST_FAIL("array1 error", 1);

    if (node.GetArrayLen("child_array1") != 2) DN_TEST_FAIL("array1 wrong size", 1);
    if (strcmp(node.GetArrayChild("child_array1", 0)->Get("child_item2"), "child_value2-1") != 0) DN_TEST_FAIL("child_array1 error", 1);
    if (strcmp(node.GetArrayChild("child_array1", 1)->Get("child_item2"), "child_value2-2") != 0) DN_TEST_FAIL("child_array1 error", 1);

    if (node.GetChildCount() != 1) DN_TEST_FAIL("wrong child count", 1);
    if (node.GetChildArrayCount() != 1) DN_TEST_FAIL("wrong child array count", 1);
    if (strcmp(node.GetChildKey(0), "array1") != 0) DN_TEST_FAIL("wrong child count", 1);
    if (strcmp(node.GetChildArrayKey(0), "child_array1") != 0) DN_TEST_FAIL("wrong child array count", 1);

    if (strcmp(node.Get("UNPRESENT_KEY", "default"), "default") != 0) DN_TEST_FAIL("default getter error", 1);
    if (node.GetArrayLen("invalid_array") != 0) DN_TEST_FAIL("invalid array should not be registered", 1);

    // Test Setters
    node.Set("key1", "5");
    node.SetI("new_key", 15);
    node.SetF("new_key", 3.5);
    DN_TEST_ASSERTKV(node, "key1", "5");
    if (node.GetI("new_key") != 15) DN_TEST_FAIL("Did not reset new_key properly", 1);
    if (node.GetF("new_key") != 3.5) DN_TEST_FAIL("Did not set new_key properly", 1);

    // Test others
    if (!node.Has("new_key")) DN_TEST_FAIL("'Has' error", 1);
    node.Remove("new_key");
    DN_TEST_ASSERTKV(node, "new_key", "");
    if (node.Has("new_key")) DN_TEST_FAIL("'Has' error", 1);

    // Untested methods

    // SetChild(const char* key, const DataNode& child);
    // SetArray(const char* key, size_t size);
    // SetArrayChild(const char* key, size_t size);
    // SetArrayElem(const char* key, int index, const char* value);
    // SetArrayElemI(const char* key, int index, int value);
    // SetArrayElemF(const char* key, int index, double value);
    // SetArrayElemChild(const char* key, int index, const DataNode& value);

    // RemoveAt(const char* key, int index);

    return 0;
}