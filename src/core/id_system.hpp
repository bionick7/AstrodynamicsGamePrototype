#ifndef ID_SYSTEM_H
#define ID_SYSTEM_H
#include "basic.hpp"

enum class EntityType: uint8_t {
    UNINITIALIZED = 0,
    PLANET,
    SHIP,
    MODULE,
    QUEST,
    ACTIVE_QUEST,
    TASK,
    TEST = 0xFE,
    INVALID = 0xFF,
};

//typedef uint32_t RID;

struct RID {
    uint32_t _internal;

    RID();
    explicit RID(uint32_t p_internal);
    RID(uint32_t index, EntityType type);

    inline int AsInt() const { return _internal; }
    inline bool operator== (RID other) const { return _internal == other._internal; }
    inline bool operator!= (RID other) const { return _internal != other._internal; }

    explicit operator uint32_t() const { return _internal; }  // Prevent implicit conversion
    explicit operator int() const { return _internal; }  // Prevent implicit conversion
};

//constexpr uint32_t GetInvalidId() { return UINT32_MAX; }
inline RID GetInvalidId() { return RID(UINT32_MAX); }

inline EntityType IdGetType(RID id) {
    return (EntityType) ((id._internal >> 24) & 0xff);
}

inline uint32_t IdGetIndex(RID id) { 
    return id._internal & 0x00fffffful; 
}

bool IsIdValid(RID id);

#endif  // ID_SYSTEM_H