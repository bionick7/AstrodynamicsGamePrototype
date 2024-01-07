#ifndef DAILOGUE_H
#define DAILOGUE_H
#include "basic.hpp"
#include "datanode.hpp"

#define DIALOGUE_MAX_REPLIES 5

struct Dialogue {
    char* text = NULL;
    size_t body_offs = 0;
    size_t replies_offs[DIALOGUE_MAX_REPLIES];
    int reply_count;

    int reply = -1;

    Dialogue() = default;
    Dialogue(const char* speaker, const char* body, const char* replies[], int p_reply_count) {
        Setup(speaker, body, replies, p_reply_count);
    }
    ~Dialogue();

    void Serialize(DataNode* dn) const;
    void Deserialize(const DataNode* dn);

    void Setup(const char* speaker, const char* body, const char* replies[], int p_reply_count);
    const char* GetSpeaker() const;
    const char* GetBody() const;
    const char* GetReply(int i) const;

    void DrawToUIContext();
};

int DialogueTests();

#endif  // DAILOGUE_H