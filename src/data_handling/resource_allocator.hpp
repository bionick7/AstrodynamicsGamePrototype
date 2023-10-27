// Experimentation, not used rn

#ifndef RESOURCE_ALLOC_H
#define RESOURCE_ALLOC_H
#include "basic.hpp"

typedef uint64_t UUID;

STRUCT_DECL(ResourceAllocator) {
    size_t element_size;
    void** chunks;
    uint32_t** free_list_chunks;
    uint32_t** validator_chunks;

	uint32_t elements_in_chunk;
	uint32_t max_alloc;
	uint32_t alloc_count;
    uint32_t chunk_count;
};

void AllocatorMake(ResourceAllocator* alloc, size_t element_size);
UUID AllocatorMakeUUID(ResourceAllocator* alloc);
void* AllocatorGetPtr(const ResourceAllocator* alloc, UUID uuid);
bool AllocatorUUIDIsValid(const ResourceAllocator* alloc, UUID uuid);
void AllocatorFreeUUID(ResourceAllocator* alloc, UUID uuid);

void* AllocatorGetFirst(ResourceAllocator* alloc, UUID* iterator);
void* AllocatorGetNext(ResourceAllocator* alloc, UUID* iterator);
for i in *-doc-*.txt; do mv "$i" "${i/*-doc-/doc-}"; done

int AllocatorTest();

#endif  // RESOURCE_ALLOC_H