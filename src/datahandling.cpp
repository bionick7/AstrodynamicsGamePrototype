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

DataNode::~DataNode() {
    for (auto kv = Children.begin(); kv != Children.end(); kv++) {
        free(kv->second);
    }
    for (auto kv = ChildArrays.begin(); kv != ChildArrays.end(); kv++) {
        free(kv->second);
    }
}

DataNode::DataNode(const DataNode& other) {
    for (auto kv = Fields.begin(); kv != Fields.end(); kv++) {
        Set(kv->first, kv->second);
    }
    for (auto kv = Children.begin(); kv != Children.end(); kv++) {
        SetChild(kv->first, *kv->second);
    }
    // These are very unoptimizer right now with individual lookups for each element insertion
    for (auto kv = FieldArrays.begin(); kv != FieldArrays.end(); kv++) {
        SetArray(kv->first, kv->second.size());
        for (size_t i=0; i < kv->second.size(); i++) {
            SetArrayElem(kv->first, i, kv->second[i]);
        }
    }
    for (auto kv = ChildArrays.begin(); kv != ChildArrays.end(); kv++) {
        SetArrayChild(kv->first, kv->second->size());
        for (size_t i=0; i < kv->second->size(); i++) {
            SetArrayElemChild(kv->first, i, kv->second->at(i));
        }
    }
}

int _YamlParse(DataNode* node, const char* filepath) {
    yaml_parser_t parser;

    yaml_parser_initialize(&parser);
    FILE *input = fopen(filepath, "rb");
    yaml_parser_set_input_file(&parser, input);
    DataNode datanode = DataNode();

    int status = DataNode::FromYaml(&datanode, &parser, false, 0);

    yaml_parser_delete(&parser);
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
                if (recursion_depth > 0) {
                    goto error;
                }
                yaml_event_delete(&event);
                return 0;
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

                break;
            }
            case YAML_SEQUENCE_END_EVENT:{
                

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
// Utility function to serialize DataNode to JSON or YAML
std::string DataNode::Serialize(FileFormat format) const {
    switch (format) {
        case FileFormat::JSON:
            return WriteJSON();
        case FileFormat::YAML:
            return WriteYAML();
        default:
            std::cout << "Unsupported file format" << std::endl;
            return "";
    }
}

std::string DataNode::WriteJSON() const {
    std::stringstream ss;
    WriteJSONInternal(ss, this, 0);
    return ss.str();
}

std::string DataNode::WriteYAML() const {
    std::stringstream ss;
    WriteYAMLInternal(ss, this, 0);
    return ss.str();
}


// Helper function to write DataNode to JSON
void DataNode::WriteJSONInternal(std::ostream& os, const DataNode* node, int indentLevel) {
    std::string indent(indentLevel * 4, ' ');

    os << "{\n";
    for (auto it = node->Fields.begin(); it != node->Fields.end(); ++it) {
        os << indent << "  \"" << it->first << "\": \"" << it->second << "\"";
        if (std::next(it) != node->Fields.end() || !node->Children.empty() || !node->FieldArrays.empty() || !node->ChildArrays.empty()) {
            os << ",";
        }
        os << "\n";
    }

    for (auto it = node->Children.begin(); it != node->Children.end(); ++it) {
        os << indent << "  \"" << it->first << "\": ";
        WriteJSONInternal(os, it->second, indentLevel + 1);
        if (std::next(it) != node->Children.end() || !node->FieldArrays.empty() || !node->ChildArrays.empty()) {
            os << ",";
        }
        os << "\n";
    }

    for (auto it = node->FieldArrays.begin(); it != node->FieldArrays.end(); ++it) {
        os << indent << "  \"" << it->first << "\": [\n";
        for (const auto& item : it->second) {
            os << indent << "    \"" << item << "\"";
            if (&item != &it->second.back()) {
                os << ",";
            }
            os << "\n";
        }
        os << indent << "  ]";
        if (std::next(it) != node->FieldArrays.end() || !node->ChildArrays.empty()) {
            os << ",";
        }
        os << "\n";
    }

    for (auto it = node->ChildArrays.begin(); it != node->ChildArrays.end(); ++it) {
        os << indent << "  \"" << it->first << "\": [\n";
        for (const auto& item : *it->second) {
            os << indent << "    ";
            WriteJSONInternal(os, &item, indentLevel + 1);
            if (&item != &it->second->back()) {
                os << ",";
            }
            os << "\n";
        }
        os << indent << "  ]";
        if (std::next(it) != node->ChildArrays.end()) {
            os << ",";
        }
        os << "\n";
    }

    os << indent << "}";
}

// Helper function to write DataNode to YAML
void DataNode::WriteYAMLInternal(std::ostream& os, const DataNode* node, int indentLevel) {
    std::string indent(indentLevel * 2, ' ');

    for (auto it = node->Fields.begin(); it != node->Fields.end(); ++it) {
        os << indent << it->first << ": " << it->second << "\n";
    }

    for (auto it = node->Children.begin(); it != node->Children.end(); ++it) {
        os << indent << it->first << ":\n";
        WriteYAMLInternal(os, it->second, indentLevel + 1);
    }

    for (auto it = node->FieldArrays.begin(); it != node->FieldArrays.end(); ++it) {
        os << indent << it->first << ":\n";
        for (const auto& item : it->second) {
            os << indent << "  - " << item << "\n";
        }
    }

    for (auto it = node->ChildArrays.begin(); it != node->ChildArrays.end(); ++it) {
        os << indent << it->first << ":\n";
        for (const auto& item : *it->second) {
            WriteYAMLInternal(os, &item, indentLevel + 1);
        }
    }
}

/*******************************
 * GETTERS
 * ***************************/


std::string DataNode::Get(std::string key, std::string def, bool quiet) const {
    auto it = Fields.find(key);
    if (it != Fields.end()) {
        return it->second;
    }
    if (!quiet) {
        std::cerr << "Key " << key << " not found" << std::endl;
    }
    return def;
}

int DataNode::GetI(std::string key, int def, bool quiet) const {
    char *p; 
    int res = (int) strtol(Get(key, "", quiet).c_str(), &p, 10);
    return *p ? res : def;
}

double DataNode::GetF(std::string key, double def, bool quiet) const {
    char *p; 
    int res = (int) strtod(Get(key, "", quiet).c_str(), &p);
    return *p ? res : def;
}

DataNode* DataNode::GetChild(std::string key, bool quiet) const {
    auto it = Children.find(key);
    if (it != Children.end()) {
        return it->second;
    }
    if (!quiet) {
        std::cerr << "Key " << key << " not found" << std::endl;
    }
    return NULL;
}

std::string DataNode::GetArray(std::string key, int index, std::string def, bool quiet) const {
    auto it = FieldArrays.find(key);
    if (it != FieldArrays.end()) {
        if (index < it->second.size()) {
            return it->second[index];
        }
    }
    if (!quiet) {
        std::cerr << "Key " << key << " not found" << std::endl;
    }
    return def;
}

int DataNode::GetArrayI(std::string key, int index, int def, bool quiet) const {
    char *p; 
    int res = (int) strtol(GetArray(key, index, "", quiet).c_str(), &p, 10);
    return *p ? res : def;
}

double DataNode::GetArrayF(std::string key, int index, double def, bool quiet) const {
    char *p; 
    int res = (int) strtod(GetArray(key, index, "", quiet).c_str(), &p);
    return *p ? res : def;
}

DataNode* DataNode::GetArrayChild(std::string key, int index, bool quiet) const {
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

size_t DataNode::GetArrayLen(std::string key, bool quiet) const {
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

std::string DataNode::GetChildKey(int index) {
    if (index < 0 || index >= Children.size()) return "[Out Of Bounds]";
    NOT_IMPLEMENTED
    //return (Children.begin() + index).;
}

std::string DataNode::GetChildArrayKey(int index) {
    if (index < 0 || index >= Children.size()) return "[Out Of Bounds]";
    NOT_IMPLEMENTED
    //return ChildArray[index];
}

/***********************************
 * SETTERS
 * **********************************/

void DataNode::Set(std::string key, std::string val) {
    if (IsReadOnly) {
        std::cerr << "Trying to set data on a readonly datanode" << std::endl;
        return;
    }
    Fields.insert_or_assign(key, val);
}

void DataNode::SetI(std::string key, int value) {
    Set(key, std::to_string(value));
}

void DataNode::SetF(std::string key, double value) {
    Set(key, std::to_string(value));
}

void DataNode::SetChild(std::string key, const DataNode& val) {
    // Adds a copy to the DataNode to the map
    DataNode* child = (DataNode*) malloc(sizeof(DataNode));
    if (IsReadOnly) {
        std::cerr << "Trying to set data on a readonly datanode" << std::endl;
        return;
    }
    
    Children.insert_or_assign(key, child);
}

void DataNode::SetArray(std::string key, size_t size) {
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

void DataNode::SetArrayElem(std::string key, int index, std::string value) {
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

void DataNode::SetArrayChild(std::string key, size_t size) {
    if (IsReadOnly) {
        std::cerr << "Trying to set data on a readonly datanode" << std::endl;
        return;
    }
    auto find = ChildArrays.find(key);
    if (find == ChildArrays.end()) {
        std::vector<DataNode>* arr = (std::vector<DataNode>*) malloc(sizeof(std::vector<DataNode>));
        find->second = arr;
    }
    find->second->resize(size);
}

void DataNode::SetArrayElemI(std::string key, int index, int value) {
    SetArrayElem(key, index, std::to_string(value));
}

void DataNode::SetArrayElemF(std::string key, int index, double value) {
    SetArrayElem(key, index, std::to_string(value));
}

void DataNode::SetArrayElemChild(std::string key, int index, const DataNode& value) {
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

bool DataNode::Has(std::string key) const {
    return Fields.count(key) > 0 || Children.count(key) > 0 || FieldArrays.count(key) > 0 || ChildArrays.count(key) > 0;
}

void DataNode::Remove(std::string key) {
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


bool DataNode::FieldEquals(std::string lhs, std::string rhs) {
    if (lhs == rhs)
        return true;
    //if (double.TryParse(lhs, out double lhsDouble) && double.TryParse(rhs, out double rhsDouble))
    //    return lhsDouble == rhsDouble;
    return false;
}

//bool DataNode::operator==(const DataNode& other) const {
//    NOT_IMPLEMENTED;
//}
//
//bool DataNode::operator!=(const DataNode& other) const {
//    return !(*this == other);
//}


int DataNodeTests() {
    DataNode node = DataNode();
    //_YamlParse(&node, "resources/data/test_data/inexistiant_file.yaml");
    //_YamlParse(&node, "resources/data/test_data/readonly_file.yaml");
    _YamlParse(&node, "resources/data/test_data/test_data.yaml");

    printf("%s\n", node.Get("key1").c_str());

    return 0;
}