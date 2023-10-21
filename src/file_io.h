#ifndef FILE_IO
#define FILE_IO
#include <yaml.h>
#include "global_state.h"

void LoadPlanets(GlobalState* gs, const char* filepath, const char* root_key);

#endif // FILE_IO