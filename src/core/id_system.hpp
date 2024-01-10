#ifndef ID_SYSTEM_H
#define ID_SYSTEM_H
#include "basic.hpp"

enum class EntityType: uint8_t {
    UNINITIALIZED = 0,
    PLANET,
    SHIP,
    SHIP_CLASS,
    MODULE_CLASS,
    QUEST,
    ACTIVE_QUEST,
    TASK,
    DIALOGUE,
    TEST = 0xFE,
    INVALID = 0xFF,
};

//typedef uint32_t RID;

struct RID {
    RID();
    explicit RID(uint32_t p_internal);
    RID(uint32_t index, EntityType type);

    inline int AsInt() const { return _internal; }
    inline bool operator== (RID other) const { return _internal == other._internal; }
    inline bool operator!= (RID other) const { return _internal != other._internal; }

    explicit operator uint32_t() const { return _internal; }  // Prevent implicit conversion
    explicit operator int() const { return _internal; }  // Prevent implicit conversion

private:
    uint32_t _internal;
};

//constexpr uint32_t GetInvalidId() { return UINT32_MAX; }
inline RID GetInvalidId() { return RID(UINT32_MAX); }
inline EntityType IdGetType(RID id) { return (EntityType) ((id.AsInt() >> 24) & 0xff); }
inline uint32_t IdGetIndex(RID id) {  return id.AsInt() & 0x00fffffful; }

bool IsIdValid(RID id);

struct IDList {
    int capacity;
    int size;

    RID* buffer;

    IDList();
    ~IDList();

    void Append(RID id);
    void EraseAt(int index);
    RID Get(int index) const;
    void Clear();

    RID operator [] (int index);
};

#endif  // ID_SYSTEM_H