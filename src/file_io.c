#include "file_io.h"


void LoadPlanets(GlobalState* gs, const char* filepath, const char* root_key) {
    yaml_parser_t parser;
    yaml_event_t event;

    yaml_parser_initialize(&parser);
    FILE *input = fopen(filepath, "rb");
    yaml_parser_set_input_file(&parser, input);

    int done = 0;
    while (!done) {
        if (!yaml_parser_parse(&parser, &event))
            goto error;
        
        switch (event.type) {
        case YAML_NO_EVENT:
        case YAML_STREAM_START_EVENT:
        case YAML_STREAM_END_EVENT:
        case YAML_DOCUMENT_START_EVENT: 
        case YAML_DOCUMENT_END_EVENT: 
        default:
            break;
        case YAML_ALIAS_EVENT: {

        break;}
        case YAML_SCALAR_EVENT: {

        break;}
        case YAML_SEQUENCE_START_EVENT: {

        break;}
        case YAML_SEQUENCE_END_EVENT: {

        break;}
        case YAML_MAPPING_START_EVENT: {
            
        break;}
        case YAML_MAPPING_END_EVENT: {

        break;}
        }

        done = (event.type == YAML_STREAM_END_EVENT);
        yaml_event_delete(&event);
    }

    error:

    yaml_parser_delete(&parser);
    fclose(input);
}