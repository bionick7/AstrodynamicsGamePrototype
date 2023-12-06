#include "datanode.hpp"
#include "time.hpp"
#include "logging.hpp"

const DataNode DataNode::Empty = DataNode();

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
    Fields = std::map<std::string, std::string>();
    Children = std::map<std::string, DataNode*>();
    FieldArrays = std::map<std::string, std::vector<std::string>>();
    ChildArrays = std::map<std::string, std::vector<DataNode>*>();
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
        to->Fields.insert_or_assign(kv->first.c_str(), kv->second.c_str());
    }
    for (auto kv = from.Children.begin(); kv != from.Children.end(); kv++) {
        DataNode* child = new DataNode(*kv->second);
        to->Children.insert_or_assign(kv->first.c_str(), child);
    }
    for (auto kv = from.FieldArrays.begin(); kv != from.FieldArrays.end(); kv++) {
        std::vector<std::string> array = std::vector<std::string>(kv->second.size());
        for (size_t i=0; i < kv->second.size(); i++) {
            array[i] = kv->second[i];
        }
        to->FieldArrays.insert_or_assign(kv->first.c_str(), std::vector<std::string>(kv->second));
    }
    for (auto kv = from.ChildArrays.begin(); kv != from.ChildArrays.end(); kv++) {
        std::vector<DataNode>* array = new std::vector<DataNode>(kv->second->size());
        for (size_t i=0; i < kv->second->size(); i++) {
            (*array)[i] = DataNode();
            CopyTo(kv->second->at(i), &(*array)[i]);
        }
        to->ChildArrays.insert_or_assign(kv->first.c_str(), array);
    }
    to->IsReadOnly = from.IsReadOnly;
}

int _YamlParse(DataNode* node, const char* filepath, bool quiet) {
    yaml_parser_t parser;

    yaml_parser_initialize(&parser);
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        if (!quiet) { ERROR("No such file: %s\n", filepath) }
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
    int status = DataNode::FromYaml(node, filepath, &parser, false, 0);
    if (status != 0) {
        FAIL("Error when reading '%s'", filepath)
    }

    yaml_parser_delete(&parser);
    fclose(file);
    return status;
}

int DataNode::FromFile(DataNode* out, const char* filepath, FileFormat fmt, bool isReadonly, bool quiet) {
    switch (fmt) {
        case FileFormat::Auto: {
            if (IsFileExtension(filepath, ".json")) {
                fmt = FileFormat::JSON;
            } else if (IsFileExtension(filepath, ".csv")) {
                fmt = FileFormat::CSV;
            } else if (IsFileExtension(filepath, ".yaml")) {
                fmt = FileFormat::YAML;
            } else {
                return 1;
            }
            FromFile(out, filepath, fmt, isReadonly);
            out->IsReadOnly = true;
            return 0;
        }
        case FileFormat::YAML: {
            _YamlParse(out, filepath, quiet);
            out->IsReadOnly = true;
            return 0;
        }
        case FileFormat::JSON:
        case FileFormat::CSV:
        default: {
            return 1;
        }
    }
}

/*std::vector<DataNode> DataNode::ManyFromFile(const char* filepath, FileFormat fmt) {
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
}*/

enum DataNodeParseState {
    DN_PARESE_EXPECT_KEY,
    DN_PARESE_EXPECT_VALUE,
    DN_PARESE_EXPECT_ITEM,
};

const int MAX_INDENT = 40;
static const char INDENTS[MAX_INDENT+1] = "                                        ";

const char* _GetSpaces(int indent_level) {
    if (indent_level > MAX_INDENT) indent_level = MAX_INDENT;
    return &INDENTS[MAX_INDENT - indent_level];
}

int DataNode::FromYaml(DataNode* node, const char* filename, yaml_parser_t* parser, bool is_readonly, int recursion_depth) {
    node->IsReadOnly = false;
    yaml_event_t event;

    //const char* indent = _GetSpaces(recursion_depth*4);

    DataNodeParseState parse_state = DN_PARESE_EXPECT_KEY;
    char key_name[1024];
    int array_index = 0;

    while (true) {

        // Get the next event.
        if (!yaml_parser_parse(parser, &event)){
            ERROR("Parsing error at %s:%d : '%s %s'", filename, parser->problem_mark.line, parser->problem, parser->context);
            goto error;
        }

        //INFO("%sgot %d\n", indent, event.type);
        switch(event.type) {
            default:
            case YAML_STREAM_START_EVENT:
            case YAML_DOCUMENT_START_EVENT:
            case YAML_DOCUMENT_END_EVENT:
                break;  // Ignore
            case YAML_STREAM_END_EVENT:{
                ERROR("Unexpected end  of stream encountered\n");
                goto error;
            }
            case YAML_SCALAR_EVENT: {
                char* scalar_value = (char*) event.data.scalar.value;
                //INFO("%ss%d: %s", indent, parse_state, scalar_value)
                switch (parse_state) {
                case DN_PARESE_EXPECT_KEY:
                    strcpy(key_name, scalar_value);
                    parse_state = DN_PARESE_EXPECT_VALUE;
                    break;
                case DN_PARESE_EXPECT_VALUE:
                    node->Set(key_name, scalar_value);
                    parse_state = DN_PARESE_EXPECT_KEY;
                    break;
                case DN_PARESE_EXPECT_ITEM:
                    node->AddArrayElem(key_name, scalar_value);
                    array_index++;
                    break;
                default:
                    FAIL("Should not reach")
                    break;
                }
                break;
            }
            case YAML_SEQUENCE_START_EVENT:{
                parse_state = DN_PARESE_EXPECT_ITEM;
                array_index = 0;
                break;
            }
            case YAML_SEQUENCE_END_EVENT:{
                parse_state = DN_PARESE_EXPECT_KEY;
                break;
            }
            case YAML_MAPPING_START_EVENT:{
                // Recurse
                DataNode child_node = DataNode();
                // Consumes everything up to and inlcuding the corresponding end event
                int status = DataNode::FromYaml(&child_node, filename, parser, is_readonly, recursion_depth+1);
                //INFO("%sChild constructed\n", indent);
                if (status != 0) {
                    ERROR("Error occured in child '%s'\n", key_name);
                    goto error;
                }
                switch (parse_state) {
                    case DN_PARESE_EXPECT_VALUE:
                        node->SetChild(key_name, child_node);
                        parse_state = DN_PARESE_EXPECT_KEY;
                        break;
                    case DN_PARESE_EXPECT_ITEM:
                        node->AddArrayElemChild(key_name, child_node);
                        break;
                    case DN_PARESE_EXPECT_KEY:
                        ERROR("Expected key, got map\n");
                        goto error;
                }
                break;
            }
            case YAML_MAPPING_END_EVENT:
                //INFO("%sMap end\n", indent);
                node->IsReadOnly = is_readonly;
                yaml_event_delete(&event);
                return 0;
        }
    }

    error:
    yaml_event_delete(&event);
    return 1;
}

// Helper function to write DataNode to JSON
void DataNode::WriteJSON(std::ostream& os, int indentLevel) const {
    const char* indent = _GetSpaces(indentLevel*2);

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
void DataNode::WriteYAML(std::ostream& os, int indentLevel, bool ignore_first_indent) const {
    const char* indent = _GetSpaces(indentLevel*2);
    int indent_nr = 0;
#define INDENT ((indent_nr++==0 && ignore_first_indent) ? "" : indent)  // All this to make lists of children look nicer

    //os << "Fields: " << Fields.size() << " -- Children: " << Children.size() << " -- Field Arrays " << FieldArrays.size() << std::endl;

    for (auto it = Fields.begin(); it != Fields.end(); ++it) {
        os << INDENT << it->first << ": " << it->second << "\n";
    }

    for (auto it = Children.begin(); it != Children.end(); ++it) {
        os << INDENT << it->first << ":\n";
        it->second->WriteYAML(os, indentLevel + 1, false);
    }

    for (auto it = FieldArrays.begin(); it != FieldArrays.end(); ++it) {
        if (it->second.empty()) {
            os << INDENT << it->first << ": []\n";
        } else {
            os << INDENT << it->first << ":\n";
        }
        for (const auto& item : it->second) {
            os << INDENT << "- " << item << "\n";
        }
    }

    for (auto it = ChildArrays.begin(); it != ChildArrays.end(); ++it) {
        if (it->second->empty()) {
            os << INDENT << it->first << ": []\n";
        } else {
            os << INDENT << it->first << ":\n";
        }
        for (const auto& item : *it->second) {
            os << INDENT << "- ";
            item.WriteYAML(os, indentLevel + 1, true);
        }
    }
    os << std::endl;
#undef INDENT
}

void DataNode::WriteToFile(const char* filepath, FileFormat fmt) const {
    /*if (!FileExists(filepath)) {
        ERROR("Could not find file '%s'", filepath)
        return;
    }*/
    std::ofstream file;
    file.open(filepath);
    switch (fmt) {
    case FileFormat::JSON:
        WriteJSON(file, 0);
        break;
    case FileFormat::YAML:
        WriteYAML(file, 0);
        break;
    default:
        WARNING("Unsupported safe format %d", fmt)
        break;
    }
    file.close();
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
        WARNING("Key %s not found", key)
    }
    return def;
}

int DataNode::GetI(const char* key, int def, bool quiet) const {
    const char* str = Get(key, "needs non-empty string", quiet);
    char *p; 
    int res = (int) strtol(str, &p, 10);
    if (p == str) {  // intentionally comparing pointers because of how strtoX works
        if (!quiet) WARNING("Could not convert '%s' to int", str)
        return def;
    }
    return res;
}

double DataNode::GetF(const char* key, double def, bool quiet) const {
    const char* str = Get(key, "needs non-empty string", quiet);
    char *p; 
    double res = (double) strtod(str, &p);
    if (p == str) {  // intentionally comparing pointers because of how strtoX works
        if (!quiet) WARNING("Could not convert '%s' to double", str)
        return def;
    }
    return res;
}

timemath::Time DataNode::GetDate(const char *key, timemath::Time def, bool quiet) const {
    timemath::TimeDeserialize(&def, GetChild(key, quiet));
    return def;
}

DataNode* DataNode::GetChild(const char* key, bool quiet) const {
    auto it = Children.find(key);
    if (it != Children.end()) {
        return it->second;
    }
    if (!quiet) {
        WARNING("Key %s not found\n", key)
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
        WARNING("Key %s not found\n", key)
    }
    return def;
}

int DataNode::GetArrayI(const char* key, int index, int def, bool quiet) const {
    const char *str = GetArray(key, index, "needs non-empty string", quiet); 
    char *p; 
    int res = (int) strtol(str, &p, 10);
    return p == str ? def : res;
}

double DataNode::GetArrayF(const char* key, int index, double def, bool quiet) const {
    const char *str = GetArray(key, index, "needs non-empty string", quiet); 
    char *p; 
    double res = strtod(str, &p);
    return p == str ? def : res;
}

DataNode* DataNode::GetArrayChild(const char* key, int index, bool quiet) const {
    auto it = ChildArrays.find(key);
    if (it != ChildArrays.end()) {
        if (index < it->second->size()) {
            return &it->second->at(index);
        }
    }
    if (!quiet) {
        WARNING("Key %s not found\n", key)
    }
    return NULL;
}

size_t DataNode::GetArrayLen(const char* key, bool quiet) const {
    auto it = FieldArrays.find(key);
    if (it != FieldArrays.end()) {
        return it->second.size();
    } else {
        if (!quiet) {
            WARNING("Key %s not found\n", key)
        }
        return 0;
    }
}

size_t DataNode::GetArrayChildLen(const char* key, bool quiet) const {
    auto it = ChildArrays.find(key);
    if (it != ChildArrays.end()) {
        return it->second->size();
    } else {
        if (!quiet) {
            WARNING("Key '%s' not found", key)
        }
        return 0;
    }
}

size_t DataNode::GetFieldCount() const {
    return Fields.size();
}

size_t DataNode::GetChildCount() const {
    return Children.size();
}

size_t DataNode::GetArrayCount() const {
    return FieldArrays.size();
}

size_t DataNode::GetChildArrayCount() const {
    return ChildArrays.size();
}

const char* DataNode::GetKey(int index) const {
    if (index < 0 || index >= Fields.size()) return "[Out Of Bounds]";
    auto it = Fields.begin();
    for (int i=0; i < index; i++) it++;
    return it->first.c_str();
}

const char* DataNode::GetChildKey(int index) const {
    if (index < 0 || index >= Children.size()) return "[Out Of Bounds]";
    auto it = Children.begin();
    for (int i=0; i < index; i++) it++;
    return it->first.c_str();
}

const char* DataNode::GetArrayKey(int index) const {
    if (index < 0 || index >= FieldArrays.size()) return "[Out Of Bounds]";
    auto it = FieldArrays.begin();
    for (int i=0; i < index; i++) it++;
    return it->first.c_str();
}

const char* DataNode::GetChildArrayKey(int index) const {
    if (index < 0 || index >= ChildArrays.size()) return "[Out Of Bounds]";
    auto it = ChildArrays.begin();
    for (int i=0; i < index; i++) it++;
    return it->first.c_str();
}


/***********************************
 * SETTERS
 * **********************************/

void DataNode::Set(const char* key, const char* value) {
    if (IsReadOnly) {
        WARNING("Trying to set data on a readonly datanode at '%s' with '%s'\n", key, value);
        return;
    }
    Fields.insert_or_assign(std::string(key), std::string(value));
}

void DataNode::SetI(const char* key, int value) {
    char buffer[20];
    sprintf(buffer, "%d", value);
    Set(key, buffer);
}

void DataNode::SetF(const char* key, double value) {
    char buffer[30];
    sprintf(buffer, "%f", value);
    Set(key, buffer);
}

void DataNode::SetDate(const char* key, timemath::Time value) {
    timemath::TimeSerialize(value, SetChild(key, DataNode()));
}

DataNode* DataNode::SetChild(const char* key, const DataNode& val) {
    // Adds a copy to the DataNode to the map and returns pointer to it
    //DataNode* child = (DataNode*) malloc(sizeof(DataNode));
    //CopyTo(val, child);
    DataNode* child = new DataNode(val);
    if (IsReadOnly) {
        WARNING("Trying to set child on a readonly datanode at '%s'\n", key);
        return NULL;
    }
    
    Children.insert_or_assign(std::string(key), child);
    return child;
}

void DataNode::SetArray(const char* key, size_t size) {
    if (IsReadOnly) {
        WARNING("Trying to set array on a readonly datanode at '%s'\n", key);
        return;
    }
    auto find = FieldArrays.find(key);
    if (find == FieldArrays.end()) {
        std::vector<std::string> arr = std::vector<std::string>();
        arr.resize(size);
        FieldArrays.insert({std::string(key), arr});
    } else {
        find->second.resize(size);
    }
}

void DataNode::SetArrayElem(const char* key, int index, const char* value) {
    if (IsReadOnly) {
        WARNING("Trying to set array element on a readonly datanode at '%s' [%d] with '%s'\n", key, index, value);
        return;
    }
    auto find = FieldArrays.find(key);
    if (find == FieldArrays.end()) {
        WARNING("No such array '%s'\n", key);
        return;
    }
    if (index < 0 || index >= find->second.size()) {
        WARNING("Invalid index at '%s' (%d >= %lld)\n", key, index, find->second.size());
        return;
    }
    find->second[index] = value;
}

void DataNode::SetArrayChild(const char* key, size_t size) {
    if (IsReadOnly) {
        WARNING("Trying to set child array element on a readonly datanode at '%s'\n", key);
        return;
    }
    auto find = ChildArrays.find(key);
    if (find == ChildArrays.end()) {
        std::vector<DataNode>* arr = new std::vector<DataNode>();
        arr->resize(size);
        ChildArrays.insert({key, arr});
    } else {
        find->second->resize(size);
    }
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

DataNode* DataNode::SetArrayElemChild(const char* key, int index, const DataNode& value) {
    if (IsReadOnly) {
        WARNING("Trying to set array element on a readonly datanode at '%s' [%d]\n", key, index);
        return NULL;
    }
    auto find = ChildArrays.find(key);
    if (find == ChildArrays.end()) {
        WARNING("No such child array '%s'\n", key);
        return NULL;
    }
    if (index < 0 || index >= find->second->size()) {
        WARNING("Invalid index at '%s' (%d >= %lld)\n", key, index, find->second->size());
        return NULL;
    }
    find->second->at(index) = DataNode(value);
    return &find->second->at(index);
}

void DataNode::AddArrayElem(const char* key, const char* value) {
    if (IsReadOnly) {
        WARNING("Trying to set child array element on a readonly datanode at '%s'\n", key);
        return;
    }
    auto find = FieldArrays.find(key);
    if (find == FieldArrays.end()) {
        std::vector<std::string> arr = std::vector<std::string>();
        arr.push_back(value);
        FieldArrays.insert({key, arr});
    } else {
        find->second.push_back(value);
    }
}

DataNode* DataNode::AddArrayElemChild(const char* key, const DataNode& value) {
    if (IsReadOnly) {
        WARNING("Trying to set array element on a readonly datanode at '%s'\n", key);
        return NULL;
    }
    auto find = ChildArrays.find(key);  // This crashes quietly without error smh
    if (find == ChildArrays.end()) {
        std::vector<DataNode>* arr = new std::vector<DataNode>();
        arr->push_back(value);
        ChildArrays.insert({key, arr});
        return &arr->back();
    } else {
        find->second->push_back(value);
        return &find->second->back();
    }
}


bool DataNode::Has(const char* key) const {
    return Fields.count(key) > 0 || Children.count(key) > 0 || FieldArrays.count(key) > 0 || ChildArrays.count(key) > 0;
}

void DataNode::Remove(const char* key) {
    if (IsReadOnly) {
        WARNING("Trying to remove %s from invalid datanode", key);
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
#define DN_TEST_ASSERTKV_PTR(node, key, value) if(strcmp(node->Get(key), value) != 0){ \
    printf("DataNodeTest failed with: %s != %s\n", key, value); \
    return 1; \
}
#define DN_TEST_ASSERTNOKV_PTR(node, key, value) if(strcmp(node->Get(key, "__INVALID__", true), value) == 0){ \
    printf("DataNodeTest failed with: %s != %s\n", key, value); \
    return 1; \
}

int DataNodeTests() {
    DataNode node = DataNode();
    
    //printf("Current directiory: %s\n", GetWorkingDirectory());

    int status;
    status = _YamlParse(&node, "resources/data/test_data/inexistiant_file.yaml", true);
    if (status != 1) DN_TEST_FAIL("inexistant file did not fail", 1);
    //_YamlParse(&node, "resources/data/test_data/readonly_file.yaml");
    //if (status != 1) return 1;
    status = _YamlParse(&node, "resources/data/test_data/test_data.yaml", false);
    if (status != 0) DN_TEST_FAIL("test_data could not be loaded", 1);

    //node.Inspect();

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
    if (strcmp(node.GetArray("array1", 0), "item2") != 0) DN_TEST_FAIL("array1 error[0]", 1);
    if (strcmp(node.GetArray("array1", 1), "item1") != 0) DN_TEST_FAIL("array1 error[1]", 1);
    if (node.GetArrayF("array1", 2) != +5.67e-8) DN_TEST_FAIL("array1 error[2]", 1);
    if (node.GetArrayI("array1", 3) != -589) DN_TEST_FAIL("array1 error[3]", 1);

    if (node.GetArrayChildLen("child_array1") != 2) DN_TEST_FAIL("child_array1 wrong size", 1);
    if (strcmp(node.GetArrayChild("child_array1", 0)->Get("child_item2"), "child_value2-1") != 0) DN_TEST_FAIL("child_array1 error[0]", 1);
    if (strcmp(node.GetArrayChild("child_array1", 1)->Get("child_item2"), "child_value2-2") != 0) DN_TEST_FAIL("child_array1 error[1]", 1);

    if (node.GetChildCount() != 1) DN_TEST_FAIL("wrong child count", 1);
    if (node.GetChildArrayCount() != 2) DN_TEST_FAIL("wrong child array count", 1);
    if (strcmp(node.GetChildKey(0), "child1") != 0) DN_TEST_FAIL("wrong child indexing", 1);
    if (strcmp(node.GetChildArrayKey(0), "child_array1") != 0) DN_TEST_FAIL("wrong child array indexing", 1);

    if (strcmp(node.Get("UNPRESENT_KEY", "default", true), "default") != 0) DN_TEST_FAIL("default getter error", 1);
    if (node.GetArrayLen("UNPRESENT_KEY", true) != 0) DN_TEST_FAIL("invalid array should not be registered", 1);
    if (node.GetF("UNPRESENT_KEY", 123.4, true) != 123.4) DN_TEST_FAIL("default getter error (float)", 1);
    if (node.GetF("UNPRESENT_KEY", 123, true) != 123) DN_TEST_FAIL("default getter error (int)", 1);

    // Test Setters
    node.Set("key1", "5");
    DN_TEST_ASSERTKV(node, "key1", "5");
    node.SetI("new_key", 15);
    if (node.GetI("new_key") != 15) DN_TEST_FAIL("Did not set new_key properly", 1);
    node.SetF("new_key", 3.5);
    if (node.GetF("new_key") != 3.5) DN_TEST_FAIL("Did not reset new_key properly", 1);

    // Test Array Setters
    node.SetArray("array2", 3);
    if(node.GetArrayLen("array2") != 3) DN_TEST_FAIL("Array not properly initialized", 1);
    node.SetArrayElem("array2", 0, "a");
    node.SetArrayElemI("array2", 1, 1);
    node.SetArrayElemF("array2", 2, -0.5);
    if(strcmp(node.GetArray("array2", 0), "a") != 0) DN_TEST_FAIL("Array element 0 not properly set", 1);
    if(node.GetArrayI("array2", 1) != 1) DN_TEST_FAIL("Array element 0 not properly set", 1);
    if(node.GetArrayF("array2", 2) != -0.5) DN_TEST_FAIL("Array element 0 not properly set", 1);


    // Test Child Setters
    DataNode child_prefab = DataNode();
    child_prefab.Set("x", "y");
    DataNode* child2 = node.SetChild("child2", child_prefab);
    node.SetArrayChild("child_arr", 2);
    DataNode* arr0_child = node.SetArrayElemChild("child_arr", 0, child_prefab);
    child2->Set("x2", "y2");
    arr0_child->Set("x2", "z2");
    DataNode* child2_tst = node.GetChild("child2");
    DataNode* child_arr0_tst = node.GetArrayChild("child_arr", 0);
    DataNode* child_arr1_tst = node.GetArrayChild("child_arr", 1);
    DN_TEST_ASSERTKV_PTR(child2_tst, "x", "y")
    DN_TEST_ASSERTKV_PTR(child2_tst, "x2", "y2")
    DN_TEST_ASSERTKV_PTR(child2_tst, "x", "y")
    DN_TEST_ASSERTKV_PTR(child_arr0_tst, "x2", "z2")
    DN_TEST_ASSERTNOKV_PTR(child_arr0_tst, "x2", "y2")
    DN_TEST_ASSERTNOKV_PTR(child_arr1_tst, "x", "y")

    // Test others
    if (!node.Has("new_key")) DN_TEST_FAIL("'Has' error", 1);
    node.Remove("new_key");
    if(strcmp(node.Get("new_key", "", true), "") != 0) DN_TEST_FAIL("Sould not have found 'new_key'", 1);
    if (node.Has("new_key")) DN_TEST_FAIL("'Has' error", 1);

    // Untested methods


    // RemoveAt(const char* key, int index);

    return 0;
}