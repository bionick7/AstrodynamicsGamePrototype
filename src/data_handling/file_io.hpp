#ifndef FILE_IO
#define FILE_IO
#include <yaml.h>
#include "global_state.hpp"

void LoadPlanets(GlobalState* gs, const char* filepath, const char* root_key);

/*
uuid -> string  : value
uuid -> len  : list
uuid, int -> uuid  : list
uuid, key -> uuid  : dict
uuid -> type

map(uuid, str)
map(uuid, arr[uuid])
map(uuid, map[int -> uuid])

map(uuid -> datablock)
    map(uuid -> ptr)
    insert(uuid, ptr)
    deleta(uuid)
    dynamic list


struct ResourceKey {
	char type;
	uint64_t uuid;
}

struct DataBlock {
    
    int size;
}

int GetKeyType(DataMap* hm, int key);
const char* FetchStr(DataMap* hm, int key);
int FetchListLen(DataMap* hm, int key);
int FetchListItem(DataMap* hm, int key, int index);
int FetchMapItem(DataMap* hm, int key, const char* item_key);
int InsertMapItem(DataMap* hm, int key, const char* item_key);
*/
#endif // FILE_IO