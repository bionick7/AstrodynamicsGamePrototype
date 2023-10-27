
#include "resource_allocator.hpp"
#include <memory.h>

uint32_t global_counter = 0;

uint32_t _GenID() {
    global_counter++;
    if (global_counter == UINT32_MAX) {
        global_counter = 0;
    }
    return global_counter;
}

struct TestStruct {
    int32_t a;
    bool b;
    char c[50];
};

void AllocatorMake(ResourceAllocator* alloc, size_t element_size) {
    alloc->element_size = element_size;
    alloc->chunks = NULL;
    alloc->free_list_chunks = NULL;
    alloc->validator_chunks = NULL;
	alloc->elements_in_chunk = 10;
	alloc->max_alloc = 0;
	alloc->alloc_count = 0;
}

UUID AllocatorMakeUUID(ResourceAllocator* alloc) {
    if (alloc->alloc_count == alloc->max_alloc) {
        //allocate a new chunk
        uint32_t chunk_count = alloc->alloc_count == 0 ? 0 : (alloc->max_alloc / alloc->elements_in_chunk);

        //grow chunks
        alloc->chunks = realloc(alloc->chunks, sizeof(void*) * (chunk_count + 1));
        alloc->chunks[chunk_count] = malloc(alloc->element_size * alloc->elements_in_chunk); //but don't initialize

        //grow validators
        alloc->validator_chunks = (uint32_t **)realloc(alloc->validator_chunks, sizeof(uint32_t *) * (chunk_count + 1));
        alloc->validator_chunks[chunk_count] = (uint32_t *)malloc(sizeof(uint32_t) * alloc->elements_in_chunk);
        //grow free lists
        alloc->free_list_chunks = (uint32_t **)realloc(alloc->free_list_chunks, sizeof(uint32_t *) * (chunk_count + 1));
        alloc->free_list_chunks[chunk_count] = (uint32_t *)malloc(sizeof(uint32_t) * alloc->elements_in_chunk);

        //initialize
        for (uint32_t i = 0; i < alloc->elements_in_chunk; i++) {
            // Don't initialize chunk.
            alloc->validator_chunks[chunk_count][i] = 0xFFFFFFFF;
            alloc->free_list_chunks[chunk_count][i] = alloc->alloc_count + i;
        }

        alloc->max_alloc += alloc->elements_in_chunk;
    }

    uint32_t free_index = alloc->free_list_chunks[alloc->alloc_count / alloc->elements_in_chunk][alloc->alloc_count % alloc->elements_in_chunk];

    uint32_t free_chunk = free_index / alloc->elements_in_chunk;
    uint32_t free_element = free_index % alloc->elements_in_chunk;

    uint32_t validator = (uint32_t)(_GenID() & 0x7FFFFFFF);
    uint64_t id = validator;
    id <<= 32;
    id |= free_index;

    alloc->validator_chunks[free_chunk][free_element] = validator;
    alloc->validator_chunks[free_chunk][free_element] |= 0x80000000; //mark uninitialized bit

    alloc->alloc_count++;

    // Initialize immediatly
    alloc->validator_chunks[free_chunk][free_element] &= 0x7FFFFFFF;

    return id;
}

void* AllocatorGetPtr(const ResourceAllocator* alloc, UUID uuid) {
    uint32_t idx = (uint32_t)(uuid & 0xFFFFFFFF);
    if (idx >= alloc->max_alloc) {
        return NULL;
    }

    uint32_t idx_chunk = idx / alloc->elements_in_chunk;
    uint32_t idx_element = idx % alloc->elements_in_chunk;

    uint32_t validator = (uint32_t)(uuid >> 32);

    void* ptr = alloc->chunks[idx_chunk] + idx_element * alloc->element_size;

    return ptr;
}

bool AllocatorUUIDIsValid(const ResourceAllocator* alloc, UUID uuid) {
    uint32_t idx = (uint32_t)(uuid & 0xFFFFFFFF);
    if (idx >= alloc->max_alloc) {
        return false;
    }

    uint32_t idx_chunk = idx / alloc->elements_in_chunk;
    uint32_t idx_element = idx % alloc->elements_in_chunk;
    uint32_t validator = (uint32_t)(uuid >> 32);

    bool owned = (alloc->validator_chunks[idx_chunk][idx_element] & 0x7FFFFFFF) == validator;
    return owned;
}

void AllocatorFreeUUID(ResourceAllocator* alloc, UUID uuid) {
    uint32_t idx = (uint32_t)(uuid & 0xFFFFFFFF);
    if (idx >= alloc->max_alloc) {
        FAIL("INVALID UUID");
    }

    uint32_t idx_chunk = idx / alloc->elements_in_chunk;
    uint32_t idx_element = idx % alloc->elements_in_chunk;

    uint32_t validator = (uint32_t)(uuid >> 32);
    if (alloc->validator_chunks[idx_chunk][idx_element] & 0x80000000) {
        FAIL("Attempted to free an uninitialized or invalid UUID");
    } else if (alloc->validator_chunks[idx_chunk][idx_element] != validator) {
        FAIL("INVALID UUID");
    }

    //alloc->chunks[idx_chunk][idx_element].~T();
    alloc->validator_chunks[idx_chunk][idx_element] = 0xFFFFFFFF; // go invalid

    alloc->alloc_count--;
    alloc->free_list_chunks[alloc->alloc_count / alloc->elements_in_chunk][alloc->alloc_count % alloc->elements_in_chunk] = idx;
}

int AllocatorTest() {
    ResourceAllocator alloc;
    AllocatorMake(&alloc, sizeof(struct TestStruct));
    UUID uuid_list[20];
    for(int i=0; i < 20; i++) {
        UUID uuid = AllocatorMakeUUID(&alloc);
        uuid_list[i] = uuid;
        struct TestStruct* ptr = AllocatorGetPtr(&alloc, uuid);
        ptr->a = i;
        strcpy(ptr->c, "Hello");
    }
    AllocatorFreeUUID(&alloc, uuid_list[5]);
    AllocatorFreeUUID(&alloc, uuid_list[19]);
    AllocatorFreeUUID(&alloc, uuid_list[13]);
    for(int i=19; i >= 0; i--) {
        UUID uuid = uuid_list[i];
        if (AllocatorUUIDIsValid(&alloc, uuid)) {
            struct TestStruct* ptr = AllocatorGetPtr(&alloc, uuid);
            //printf("%d: uuid %ld => a = %d, c = %s\n", i, uuid, ptr->a, ptr->c);
            if (ptr->a != i) return 1;
            if (strcmp(ptr->c, "Hello")) return 1;
            if (i == 5 || i == 19 || i == 13) {
                return 1;
            }
        } else {
            //printf("%d: invalid uuid\n", i);
            if (i != 5 && i != 19 && i != 13) {
                return 1;
            }
        }
    }
    return 0;
}
