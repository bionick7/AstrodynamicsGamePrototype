#include "time.hpp"
#include "logging.hpp"
#include "string_builder.hpp"
#include "assets.hpp"
#include "debug_console.hpp"
#include "datanode.hpp"

DataNode::FlexString::FlexString(const char *txt, DataNode *dn) {
    offset = dn->text_buffer.length;
    dn->text_buffer.Add(txt);
}

const char *DataNode::FlexString::GetChar(const DataNode *dn) {
    return dn->text_buffer.c_str + offset;
}

DataNode::DataNode() {
    is_read_only = false;
    text_buffer = StringBuffer();
    fields = Table<Field>();
    children = Table<Child>();
    field_arrays = Table<FieldArray>();
    child_arrays = Table<ChildArray>();
}

DataNode::DataNode(const DataNode& other) {
    text_buffer = StringBuffer();
    fields = Table<Field>();
    children = Table<Child>();
    field_arrays = Table<FieldArray>();
    child_arrays = Table<ChildArray>();
    CopyTo(&other, this);
}

DataNode::~DataNode() {
    if (children.size > 0 && children.data[0].value == NULL) {
        int a = 0;
    }
    for (int i=0; i < children.size; i++) {
        delete children.data[i].value;
    }
}

void DataNode::CopyTo(const DataNode* from, DataNode* to) {
    if (to == NULL) return;
    if (from == NULL) return;
    CopyTable(&from->fields, &to->fields);
    CopyTable(&from->children, &to->children);
    CopyTable(&from->field_arrays, &to->field_arrays);
    CopyTable(&from->child_arrays, &to->child_arrays);
    to->text_buffer = from->text_buffer.c_str;
    to->is_read_only = from->is_read_only;
}

int _YamlParseFromText(DataNode* node, const char* origin, const char* text, bool quiet) {
    yaml_parser_t parser;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_string(&parser, (const unsigned char*) text, strlen(text));

    // consume until you reach map_start
    int status = -1;
    yaml_event_t event;
    while (true) {
        if (!yaml_parser_parse(&parser, &event)) break;
        if (event.type == YAML_MAPPING_START_EVENT) break;
        if (event.type == YAML_STREAM_END_EVENT) goto EMPTY_STREAM;
    }

    status = DataNode::FromYaml(node, origin, &parser, false, 0);
    if (status != 0) {
        FAIL("Error when reading '%s' (parsed from text)", origin)
    }

    EMPTY_STREAM:
    yaml_event_delete(&event);
    yaml_parser_delete(&parser);
    return status;
}

int _YamlParse(DataNode* node, const char* filepath, bool quiet) {
    if (!assets::HasTextResource(filepath)) {
        if (!quiet && !GetSettingBool("suppress_datanode_warnings")) { ERROR("No such file: %s\n", filepath) }
        return 1;
    }

    char* text = assets::GetResourceText(filepath);
    int result = _YamlParseFromText(node, filepath, text, quiet);
    free(text);
    return result;
}

int DataNode::FromMemory(DataNode* out, const char* origin, const char* text,
                         file_format::T fmt, bool isReadonly, bool quiet) {
    switch (fmt) {
        case file_format::AUTO: {
            if (IsFileExtension(origin, ".json")) {
                fmt = file_format::JSON;
            } else if (IsFileExtension(origin, ".csv")) {
                fmt = file_format::CSV;
            } else if (IsFileExtension(origin, ".yaml")) {
                fmt = file_format::YAML;
            } else {
                return 1;
            }
            FromMemory(out, origin, text, fmt, isReadonly);
            out->is_read_only = true;
            return 0;
        }
        case file_format::YAML: {
            _YamlParseFromText(out, origin, text, quiet);
            out->is_read_only = true;
            return 0;
        }
        case file_format::JSON:
        case file_format::CSV:
        default: {
            return 1;
        }
    }
}

int DataNode::FromFile(DataNode* out, const char* filepath, file_format::T fmt, bool isReadonly, bool quiet) {
    switch (fmt) {
        case file_format::AUTO: {
            if (IsFileExtension(filepath, ".json")) {
                fmt = file_format::JSON;
            } else if (IsFileExtension(filepath, ".csv")) {
                fmt = file_format::CSV;
            } else if (IsFileExtension(filepath, ".yaml")) {
                fmt = file_format::YAML;
            } else {
                return 1;
            }
            FromFile(out, filepath, fmt, isReadonly);
            out->is_read_only = true;
            return 0;
        }
        case file_format::YAML: {
            _YamlParse(out, filepath, quiet);
            out->is_read_only = true;
            return 0;
        }
        case file_format::JSON:
        case file_format::CSV:
        default: {
            return 1;
        }
    }
}

enum DataNodeParseState {
    DN_PARSE_EXPECT_KEY,
    DN_PARSE_EXPECT_VALUE,
    DN_PARSE_EXPECT_ITEM,
};

const int MAX_INDENT = 40;
static const char INDENTS[MAX_INDENT+1] = "                                        ";

const char* _GetSpaces(int indent_level) {
    if (indent_level > MAX_INDENT) indent_level = MAX_INDENT;
    return &INDENTS[MAX_INDENT - indent_level];
}

int DataNode::FromYaml(DataNode* node, const char* filename, yaml_parser_t* parser, bool is_readonly, int recursion_depth) {
    node->is_read_only = false;
    yaml_event_t event;

    //const char* indent = _GetSpaces(recursion_depth*4);

    DataNodeParseState parse_state = DN_PARSE_EXPECT_KEY;
    char key_name[1024];  // Cannot be static, since it's recursive

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
                ERROR("Unexpected end of stream encountered\n");
                goto error;
            }
            case YAML_SCALAR_EVENT: {
                char* scalar_value = (char*) event.data.scalar.value;
                //INFO("%ss%d: %s", indent, parse_state, scalar_value)
                switch (parse_state) {
                case DN_PARSE_EXPECT_KEY:
                    strcpy(key_name, scalar_value);
                    parse_state = DN_PARSE_EXPECT_VALUE;
                    break;
                case DN_PARSE_EXPECT_VALUE:
                    node->Set(key_name, scalar_value);
                    parse_state = DN_PARSE_EXPECT_KEY;
                    break;
                case DN_PARSE_EXPECT_ITEM:
                    node->AppendToArray(key_name, scalar_value);
                    break;
                default:
                    FAIL("Should not reach")
                    break;
                }
                break;
            }
            case YAML_SEQUENCE_START_EVENT:{
                parse_state = DN_PARSE_EXPECT_ITEM;
                break;
            }
            case YAML_SEQUENCE_END_EVENT:{
                parse_state = DN_PARSE_EXPECT_KEY;
                break;
            }
            case YAML_MAPPING_START_EVENT:{
                // Recurse
                DataNode* child_node_ptr = NULL;
                // Consumes everything up to and inlcuding the corresponding end event
                switch (parse_state) {
                    case DN_PARSE_EXPECT_VALUE:
                        child_node_ptr = node->SetChild(key_name);
                        parse_state = DN_PARSE_EXPECT_KEY;
                        break;
                    case DN_PARSE_EXPECT_ITEM:
                        child_node_ptr = node->AppendToChildArray(key_name);
                        break;
                    case DN_PARSE_EXPECT_KEY:
                        ERROR("Expected key, got map\n");
                        goto error;
                }
                int status = DataNode::FromYaml(child_node_ptr, filename, parser, is_readonly, recursion_depth+1);
                //INFO("%sChild constructed\n", indent);
                if (status != 0) {
                    ERROR("Error occurred in child '%s'\n", key_name);
                    goto error;
                }
                break;
            }
            case YAML_MAPPING_END_EVENT:
                //INFO("%sMap end\n", indent);
                node->is_read_only = is_readonly;
                yaml_event_delete(&event);
                return 0;
        }
    }

    error:
    yaml_event_delete(&event);
    return 1;
}

// Helper function to write DataNode to JSON
/*void DataNode::WriteJSON(StringBuilder* sb, int indentLevel) const {
    const char* indent = _GetSpaces(indentLevel*2);

    sb->Add("{\n");
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        sb->AddFormat("%s   \"%s\": \"%s\"", indent, it->first.c_str(), it->second.c_str());
        if (std::next(it) != fields.end() || !children.empty() || !field_arrays.empty() || !child_arrays.empty()) {
            sb->Add(",");
        }
        sb->Add("\n");
    }

    for (auto it = children.begin(); it != children.end(); ++it) {
        sb->AddFormat("%s   \"%s\":", indent, it->first.c_str());
        it->second->WriteJSON(sb, indentLevel + 1);
        if (std::next(it) != children.end() || !field_arrays.empty() || !child_arrays.empty()) {
            sb->Add(",");
        }
        sb->Add("\n");
    }

    for (auto it = field_arrays.begin(); it != field_arrays.end(); ++it) {
        sb->AddFormat("%s   \"%s\": [\n", indent, it->first.c_str());
        for (const auto& item : it->second) {
            sb->AddFormat("%s     \"%s\"", indent, item);
            if (&item != &it->second.back()) {
                sb->Add(",");
            }
            sb->Add("\n");
        }
        sb->Add(indent).Add("  ]");
        if (std::next(it) != field_arrays.end() || !child_arrays.empty()) {
            sb->Add(",");
        }
        sb->Add("\n");
    }

    for (auto it = child_arrays.begin(); it != child_arrays.end(); ++it) {
        sb->AddFormat("%s   \"%s\": [\n", indent, it->first.c_str());
        for (const auto& item : *it->second) {
            sb->Add(indent).Add("    ");
            item.WriteJSON(sb, indentLevel + 1);
            if (&item != &it->second->back()) {
                sb->Add(",");
            }
            sb->Add("\n");
        }
        sb->Add(indent).Add("  ]");
        if (std::next(it) != child_arrays.end()) {
            sb->Add(",");
        }
        sb->Add("\n");
    }

    sb->Add(indent).Add("}");
}*/

// Helper function to write DataNode to YAML
void DataNode::WriteYAML(StringBuilder* sb, int indentLevel, bool ignore_first_indent) const {
    const char* indent = _GetSpaces(indentLevel*2);
    int indent_nr = 0;
#define INDENT ((indent_nr++==0 && ignore_first_indent) ? "" : indent)  // All this to make lists of children look nicer

    //os << "fields: " << fields.size() << " -- children: " << children.size() << " -- Field Arrays " << field_arrays.size() << std::endl;

    for (int i=0; i < fields.size; i++) {
        sb->AddFormat("%s%s: %s\n", INDENT, fields.data[i].key.GetChar(this), fields.data[i].value.GetChar(this));
    }

    for (int i=0; i < children.size; i++) {
        sb->AddFormat("%s%s:\n", INDENT, children.data[i].key.GetChar(this));
        children.data[i].value->WriteYAML(sb, indentLevel + 1, false);
    }

    for (int i=0; i < field_arrays.size; i++) {
        const char* key = field_arrays.data[i].key.GetChar(this);
        if (field_arrays.data[i].strings.size == 0) {
            sb->AddFormat("%s%s: []\n", INDENT, key);
        } else {
            sb->AddFormat("%s%s:\n", INDENT, key);
        }
        for (int j=0; j < field_arrays.data[i].strings.size; j++) {
            sb->AddFormat("%s- %s\n", INDENT, field_arrays.data[i].strings[i].GetChar(this) );
        }
    }

    for (int i=0; i < child_arrays.size; i++) {
        const char* key = child_arrays.data[i].key.GetChar(this);
        if (child_arrays.data[i].nodes.size == 0) {
            sb->AddFormat("%s%s: []\n", INDENT, key);
        } else {
            sb->AddFormat("%s%s:\n", INDENT, key);
        }
        for (int j=0; j < child_arrays.data[i].nodes.size; j++) {
            sb->Add(INDENT).Add("- ");
            child_arrays.data[i].nodes[j].WriteYAML(sb, indentLevel + 1, true);
        }
    }
    sb->Add("\n");
#undef INDENT
}

void DataNode::WriteToFile(const char* filepath, file_format::T fmt) const {
    StringBuilder sb;
    switch (fmt) {
    //case file_format::JSON:
    //    WriteJSON(&sb, 0);
    //    break;
    case file_format::YAML:
        WriteYAML(&sb, 0);
        break;
    default:
        WARNING("Unsupported safe format %d", fmt)
        break;
    }
    sb.WriteToFile(filepath);
}

/*******************************
 * GETTERS
 * ***************************/

const char *DataNode::Get(TableKey key, const char *def, bool quiet) const {
    int find = fields.Find(key);
    if (find >= 0) {
        return fields.data[find].value.GetChar(this);
    }
    if (!quiet && !GetSettingBool("suppress_datanode_warnings")) {
        WARNING("Key '%s' not found", key)
    }
    return def;
}

long DataNode::GetI(TableKey key, long def, bool quiet) const {
    const char* str = Get(key, "needs non-empty string", quiet);
    char *p;    long res = strtol(str, &p, 10);
    if (p == str) {  // intentionally comparing pointers because of how strtoX works
        if (!quiet && !GetSettingBool("suppress_datanode_warnings")) WARNING("Could not convert '%s' to int", str)
        return def;
    }
    return res;
}

double DataNode::GetF(TableKey key, double def, bool quiet) const {
    const char* str = Get(key, "needs non-empty string", quiet);
    char *p;    double res = (double) strtod(str, &p);
    if (p == str) {  // intentionally comparing pointers because of how strtoX works
        if (!quiet && !GetSettingBool("suppress_datanode_warnings")) WARNING("Could not convert '%s' to double", str)
        return def;
    }
    return res;
}

timemath::Time DataNode::GetDate(TableKey key, timemath::Time def, bool quiet) const {
    def.Deserialize(this, key);
    return def;
}

DataNode* DataNode::GetChild(TableKey key, bool quiet) const {
    int it = children.Find(key);
    if (it >= 0) {
        return children.data[it].value;
    }
    if (!quiet && !GetSettingBool("suppress_datanode_warnings")) {
        WARNING("Key %s not found\n", key)
    }
    return NULL;
}

const char* DataNode::GetArrayElem(TableKey key, int index, const char* def, bool quiet) const {
    auto it = field_arrays.Find(key);
    if (it >= 0) {
        if (index < field_arrays.data[it].strings.size) {
            return field_arrays.data[it].strings[index].GetChar(this);
        }
    }
    if (!quiet && !GetSettingBool("suppress_datanode_warnings")) {
        WARNING("Key %s not found\n", key)
    }
    return def;
}

long DataNode::GetArrayElemI(TableKey key, int index, long def, bool quiet) const {
    const char *str = GetArrayElem(key, index, "needs non-empty string", quiet);    char *p;    long res = strtol(str, &p, 10);
    return p == str ? def : res;
}

double DataNode::GetArrayElemF(TableKey key, int index, double def, bool quiet) const {
    const char *str = GetArrayElem(key, index, "needs non-empty string", quiet);    char *p;    double res = strtod(str, &p);
    return p == str ? def : res;
}

DataNode* DataNode::GetChildArrayElem(TableKey key, int index, bool quiet) const {
    int it = child_arrays.Find(key);
    if (it >= 0) {
        if (index < child_arrays.data[it].nodes.size) {
            return &child_arrays.data[it].nodes[index];
        }
    }
    if (!quiet && !GetSettingBool("suppress_datanode_warnings")) {
        WARNING("Key %s not found\n", key)
    }
    return NULL;
}

size_t DataNode::GetArrayLen(TableKey key, bool quiet) const {
    int it = field_arrays.Find(key);
    if (it >= 0) {
        return field_arrays.data[it].strings.size;
    } else {
        if (!quiet && !GetSettingBool("suppress_datanode_warnings")) {
            WARNING("Key %s not found\n", key)
        }
        return 0;
    }
}

size_t DataNode::GetChildArrayLen(TableKey key, bool quiet) const {
    int it = child_arrays.Find(key);
    if (it >= 0) {
        return child_arrays.data[it].nodes.size;
    } else {
        if (!quiet && !GetSettingBool("suppress_datanode_warnings")) {
            WARNING("Key '%s' not found", key)
        }
        return 0;
    }
}

size_t DataNode::GetFieldCount() const {
    return fields.size;
}

size_t DataNode::GetChildCount() const {
    return children.size;
}

size_t DataNode::GetArrayCount() const {
    return field_arrays.size;
}

size_t DataNode::GetChildArrayCount() const {
    return child_arrays.size;
}

void DataNode::SerializeBuffer(TableKey key, const int buffer[], const char* names[], int buffer_size, bool skip_zeros) {
    DataNode* child_data = SetChild(key);
    for(int i=0; i < buffer_size; i++) {
        if (buffer[i] > 0 || !skip_zeros) {
            child_data->SetI(names[i], buffer[i]);
        }
    }
}

void DataNode::DeserializeBuffer(TableKey key, int buffer[], const char* names[], int buffer_size) const {
    if (!HasChild(key)) {
        for(int i=0; i < buffer_size; i++) buffer[i] = 0;
        return;
    }
    const DataNode* child = GetChild(key);
    for(int i=0; i < buffer_size; i++) {
        if (child->Has(names[i])) {
            buffer[i] = child->GetI(names[i]);
        } else {
            buffer[i] = 0;
        }
    }
}

const char* DataNode::GetKey(int index) const {
    if (index < 0 || index >= fields.size) return "[Out Of Bounds]";
    return fields.data[index].key.GetChar(this);
}

const char* DataNode::GetChildKey(int index) const {
    if (index < 0 || index >= children.size) return "[Out Of Bounds]";
    return children.data[index].key.GetChar(this);
}

const char* DataNode::GetArrayKey(int index) const {
    if (index < 0 || index >= field_arrays.size) return "[Out Of Bounds]";
    return field_arrays.data[index].key.GetChar(this);
}

const char* DataNode::GetChildArrayKey(int index) const {
    if (index < 0 || index >= child_arrays.size) return "[Out Of Bounds]";
    return child_arrays.data[index].key.GetChar(this);
}

void DataNode::Inspect() const {
    StringBuilder sb;
    WriteYAML(&sb);
    INFO(sb.c_str);
}

/***********************************
 * SETTERS
 * **********************************/

void DataNode::Set(TableKey key, const char* value) {
    if (is_read_only) {
        WARNING("Trying to set data on a readonly datanode at '%s' with '%s'\n", key, value);
        return;
    }
    int find = fields.Find(key);
    if (find >= 0) {
        fields.data[find].value = FlexString(value, this);
    } else {
        int index = fields.AllocForInsertion(key);
        fields.data[index].key = FlexString(key.txt, this);
        fields.data[index].value = FlexString(value, this);
    }
}

void DataNode::SetI(TableKey key, int value) {
    static char buffer[20];
    sprintf(buffer, "%d", value);
    Set(key, buffer);
}

void DataNode::SetF(TableKey key, double value) {
    static char buffer[30];
    sprintf(buffer, "%f", value);
    Set(key, buffer);
}

void DataNode::SetDate(TableKey key, timemath::Time value) {
    value.Serialize(this, key);
}

DataNode* DataNode::SetChild(TableKey key) {
    // Adds a copy to the DataNode to the map and returns pointer to it
    //CopyTo(val, child);
    if (is_read_only) {
        WARNING("Trying to set child on a readonly datanode at '%s'\n", key);
        return NULL;
    }

    int index = children.AllocForInsertion(key);
    children.data[index].key = FlexString(key.txt, this);
    children.data[index].value = new DataNode();
    return children.data[index].value;
}

void DataNode::CreateArray(TableKey key, size_t size) {
    if (is_read_only) {
        WARNING("Trying to set array on a readonly datanode at '%s'\n", key);
        return;
    }
    int find = field_arrays.Find(key);
    if (find < 0) {
        find = field_arrays.AllocForInsertion(key);
        field_arrays.data[find].key = FlexString(key.txt, this);
    }
    field_arrays.data[find].strings.Resize(size);  // Set Capacity
    field_arrays.data[find].strings.size = size;   // Set Size
}

void DataNode::InsertIntoArray(TableKey key, int index, const char* value) {
    if (is_read_only) {
        WARNING("Trying to set array element on a readonly datanode at '%s' [%d] with '%s'\n", key, index, value);
        return;
    }
    int find = field_arrays.Find(key);
    if (find < 0) {
        WARNING("No such array '%s'\n", key);
        return;
    }
    if (index < 0 || index >= field_arrays.data[find].strings.size) {
        WARNING("Invalid index at '%s' (%d >= %lld)\n", key, index, field_arrays.data[find].strings.size);
        return;
    }
    field_arrays.data[find].strings[index] = FlexString(value, this);
}

void DataNode::CreatChildArray(TableKey key, size_t size) {
    if (is_read_only) {
        WARNING("Trying to set child array element on a readonly datanode at '%s'\n", key);
        return;
    }
    int find = child_arrays.Find(key);
    if (find < 0) {
        find = child_arrays.AllocForInsertion(key);
        child_arrays.data[find].key = FlexString(key.txt, this);
    } 
    child_arrays.data[find].nodes.Resize(size);  // Set Capacity
    child_arrays.data[find].nodes.size = size;   // Set Size
}

void DataNode::InsertIntoArrayI(TableKey key, int index, int value) {
    static char buffer[100];
    sprintf(buffer, "%d", value);
    InsertIntoArray(key, index, buffer);
}

void DataNode::InsertIntoArrayF(TableKey key, int index, double value) {
    static char buffer[100];
    sprintf(buffer, "%f", value);
    InsertIntoArray(key, index, buffer);
}

DataNode* DataNode::InsertIntoChildArray(TableKey key, int index) {
    if (is_read_only) {
        WARNING("Trying to set array element on a readonly datanode at '%s' [%d]\n", key, index);
        return NULL;
    }
    int find = child_arrays.Find(key);
    if (find < 0) {
        WARNING("No such child array '%s'\n", key);
        return NULL;
    }
    if (index < 0 || index >= child_arrays.data[find].nodes.size) {
        WARNING("Invalid index at '%s' (%d >= %lld)\n", key, index, child_arrays.data[find].nodes.size);
        return NULL;
    }
    return &child_arrays.data[find].nodes[index];
}

void DataNode::AppendToArray(TableKey key, const char* value) {
    if (is_read_only) {
        WARNING("Trying to set child array element on a readonly datanode at '%s'\n", key);
        return;
    }
    int find = field_arrays.Find(key);
    if (find < 0) {
        find = field_arrays.AllocForInsertion(key);
        field_arrays.data[find].key = FlexString(key.txt, this);
    }
    field_arrays.data[find].strings.Append(FlexString(value, this));
}

DataNode* DataNode::AppendToChildArray(TableKey key) {
    if (is_read_only) {
        WARNING("Trying to set array element on a readonly datanode at '%s'\n", key);
        return NULL;
    }
    int find = child_arrays.Find(key);
    if (find < 0) {
        find = child_arrays.AllocForInsertion(key);
        child_arrays.data[find].key = FlexString(key.txt, this);
    }
    int index = child_arrays.data[find].nodes.AllocForAppend();
    return &child_arrays.data[find].nodes[index];
}


bool DataNode::Has(TableKey key) const {
    return fields.Find(key) >= 0;
}

bool DataNode::HasChild(TableKey key) const {
    return children.Find(key) >= 0;
}

bool DataNode::HasArray(TableKey key) const {
    return field_arrays.Find(key) >= 0;
}

bool DataNode::HasChildArray(TableKey key) const {
    return fields.Find(key) >= 0;
}

void DataNode::Remove(TableKey key) {
    if (is_read_only) {
        WARNING("Trying to remove %s from read-only datanode", key);
        return;
    }

    fields.Remove(key);
    field_arrays.Remove(key);
    children.Remove(key);
    child_arrays.Remove(key);
}

void RemoveAt(TableKey key, int index) {
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

#define DN_TEST_FAIL(msg, exit_code) {ERROR("DataNodeTest failed with: %s\n", msg); return exit_code;}
#define DN_TEST_ASSERTKV(node, key, value) if(strcmp(node.Get(key), value) != 0){ \
    ERROR("DataNodeTest failed with: %s != %s\n", key, value); \
    return 1; \
}
#define DN_TEST_ASSERTKV_PTR(node, key, value) if(strcmp(node->Get(key), value) != 0){ \
    ERROR("DataNodeTest failed with: %s != %s\n", key, value); \
    return 1; \
}
#define DN_TEST_ASSERTNOKV_PTR(node, key, value) if(strcmp(node->Get(key, "__INVALID__", true), value) == 0){ \
    ERROR("DataNodeTest failed with: %s != %s\n", key, value); \
    return 1; \
}

int DataNodeTests() {
    DataNode node = DataNode();
    
    //INFO("Current directiory: %s\n", GetWorkingDirectory());

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
    if (strcmp(node.GetArrayElem("array1", 0), "item2") != 0) DN_TEST_FAIL("array1 error[0]", 1);
    if (strcmp(node.GetArrayElem("array1", 1), "item1") != 0) DN_TEST_FAIL("array1 error[1]", 1);
    if (node.GetArrayElemF("array1", 2) != +5.67e-8) DN_TEST_FAIL("array1 error[2]", 1);
    if (node.GetArrayElemI("array1", 3) != -589) DN_TEST_FAIL("array1 error[3]", 1);

    if (node.GetChildArrayLen("child_array1") != 2) DN_TEST_FAIL("child_array1 wrong size", 1);
    if (strcmp(node.GetChildArrayElem("child_array1", 0)->Get("child_item2"), "child_value2-1") != 0) DN_TEST_FAIL("child_array1 error[0]", 1);
    if (strcmp(node.GetChildArrayElem("child_array1", 1)->Get("child_item2"), "child_value2-2") != 0) DN_TEST_FAIL("child_array1 error[1]", 1);

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
    node.CreateArray("array2", 3);
    if(node.GetArrayLen("array2") != 3) DN_TEST_FAIL("Array not properly initialized", 1);
    node.InsertIntoArray("array2", 0, "a");
    node.InsertIntoArrayI("array2", 1, 1);
    node.InsertIntoArrayF("array2", 2, -0.5);
    if(strcmp(node.GetArrayElem("array2", 0), "a") != 0) DN_TEST_FAIL("Array element 0 not properly set", 1);
    if(node.GetArrayElemI("array2", 1) != 1) DN_TEST_FAIL("Array element 0 not properly set", 1);
    if(node.GetArrayElemF("array2", 2) != -0.5) DN_TEST_FAIL("Array element 0 not properly set", 1);

    node.AppendToArray("array3", "a");
    node.AppendToArray("array3", "b");
    if(node.GetArrayLen("array3") != 2) DN_TEST_FAIL("Array not properly initialized for appending", 1);
    if(strcmp(node.GetArrayElem("array3", 0), "a") != 0) DN_TEST_FAIL("Array element 0 not properly appended", 1);
    if(strcmp(node.GetArrayElem("array3", 0), "b") != 0) DN_TEST_FAIL("Array element 1 not properly appended", 1);


    // Test Child Setters
    DataNode* child2 = node.SetChild("child2");
    child2->Set("x", "y");
    node.CreatChildArray("child_arr", 2);
    DataNode* arr0_child = node.InsertIntoChildArray("child_arr", 0);
    DataNode::CopyTo(child2, arr0_child);
    child2->Set("x2", "y2");
    arr0_child->Set("x2", "z2");
    DataNode* child2_tst = node.GetChild("child2");
    DataNode* child_arr0_tst = node.GetChildArrayElem("child_arr", 0);
    DataNode* child_arr1_tst = node.GetChildArrayElem("child_arr", 1);
    DN_TEST_ASSERTKV_PTR(child2_tst, "x", "y")
    DN_TEST_ASSERTKV_PTR(child2_tst, "x2", "y2")
    DN_TEST_ASSERTKV_PTR(child2_tst, "x", "y")
    DN_TEST_ASSERTKV_PTR(child_arr0_tst, "x2", "z2")
    DN_TEST_ASSERTNOKV_PTR(child_arr0_tst, "x2", "y2")
    DN_TEST_ASSERTNOKV_PTR(child_arr1_tst, "x", "y")

    node.CreatChildArray("child_arr", 0);

    // Test reallocations
    for (int i=0; i < 20; i++) {
        DataNode* arr_child = node.AppendToChildArray("child_arr");
        DataNode* arr_child_child = arr_child->SetChild("c");
        arr_child_child->Set("index", TextFormat("%d", i));
    }

    // Test others
    if (!node.Has("new_key")) DN_TEST_FAIL("'Has' error", 1);
    node.Remove("new_key");
    if(strcmp(node.Get("new_key", "", true), "") != 0) DN_TEST_FAIL("Sould not have found 'new_key'", 1);
    if (node.Has("new_key")) DN_TEST_FAIL("'Has' error", 1);

    // Untested methods


    // RemoveAt(const char* key, int index);

    return 0;
}
