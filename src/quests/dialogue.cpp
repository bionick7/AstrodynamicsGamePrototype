#include "dialogue.hpp"
#include "utils.hpp"
#include "tests.hpp"

Dialogue::~Dialogue() {
    delete[] text;
}

/*
    speaker_name\0text ... blablalb \0 opt1 \0 opt2 \0\0\0\0\0
    ^             ^                    ^                    ^
    GetSpeaker    GetBody()            GetReply(0)          GetReply(5)
*/

void Dialogue::Setup(const char* speaker, const char* body, const char* replies[], int p_reply_count) {
    reply_count = p_reply_count;
    if (reply_count > DIALOGUE_MAX_REPLIES) {
        reply_count = DIALOGUE_MAX_REPLIES;
    }
    // Define sizes
    body_offs = strlen(speaker) + 1;
    replies_offs[0] = body_offs + strlen(body) + 1;

    int last_reply_iter = MinInt(reply_count, DIALOGUE_MAX_REPLIES-1);
    for(int i=0; i < last_reply_iter; i++){
        replies_offs[i+1] = replies_offs[i] + strlen(replies[i]) + 1;
    }
    for(int i=last_reply_iter; i < DIALOGUE_MAX_REPLIES-1; i++){
        replies_offs[i+1] = replies_offs[i] + 1;
    }
    size_t buffer_size = replies_offs[DIALOGUE_MAX_REPLIES-1] + 1;
    if (reply_count == 5) buffer_size += strlen(replies[DIALOGUE_MAX_REPLIES-1]);
    text = new char[buffer_size];

    // Copy strings
    strcpy(&text[0], speaker);
    text[body_offs-1] = '\0';
    strcpy(&text[body_offs], body);
    for(int i=0; i < reply_count; i++){
        strcpy(&text[replies_offs[i]], replies[i]);
        text[replies_offs[i]-1] = '\0';
    }
    for(int i=reply_count; i < DIALOGUE_MAX_REPLIES; i++){
        text[replies_offs[i]-1] = '\0';
    }
    text[buffer_size-1] = '\0';
}

void Dialogue::Serialize(DataNode* dn) const {
    dn->Set("speaker", GetSpeaker());
    dn->Set("body", GetBody());
    dn->SetArray("replies", reply_count);
    for(int i=0; i < reply_count; i++) {
        dn->SetArrayElem("replies", i, GetReply(i));
    }
    dn->SetI("selected", reply);
}

void Dialogue::Deserialize(const DataNode* dn) {
    reply_count = dn->GetArrayLen("replies");
    const char** replies = new const char*[reply_count];
    for(int i=0; i < reply_count; i++) {
        replies[i] = dn->GetArray("replies", i);
    }
    Setup(
        dn->Get("speaker"),
        dn->Get("body"),
        replies,
        reply_count
    );
    reply = dn->GetI("selected", reply);
}

const char* Dialogue::GetSpeaker() const {
    return text;
}

const char* Dialogue::GetBody() const {
    return &text[body_offs];
}

const char* Dialogue::GetReply(int i) const {
    if (i >= DIALOGUE_MAX_REPLIES) ERROR("reply index (%d) must be less than (%d)", i, DIALOGUE_MAX_REPLIES)
    return &text[replies_offs[i]];
}

void Dialogue::DrawToUIContext() const {

}

int DialogueTests() {   
    const char* replies[] = {"reply1", "reply2", "reply3"};
    Dialogue dialogue3 = Dialogue("speaker", "body\n\tfurther body", replies, 3);
    Dialogue dialogue0 = Dialogue("speaker", "body\n\tfurther body", NULL, 0);
    TEST_ASSERT_STREQUAL(dialogue3.GetSpeaker(), "speaker")
    TEST_ASSERT_STREQUAL(dialogue3.GetBody(), "body\n\tfurther body")
    for(int i=0; i < 3; i++) {
        TEST_ASSERT_STREQUAL(dialogue3.GetReply(i), replies[i])
    }
    TEST_ASSERT_STREQUAL(dialogue3.GetReply(3), "")
    TEST_ASSERT_STREQUAL(dialogue3.GetReply(4), "")

    DataNode dn;
    dialogue0.Serialize(&dn);
    dialogue0 = Dialogue();
    dialogue0.Deserialize(&dn);

    TEST_ASSERT_STREQUAL(dialogue0.GetSpeaker(), "speaker")
    TEST_ASSERT_STREQUAL(dialogue0.GetBody(), "body\n\tfurther body")
    for(int i=0; i < 5; i++) {
        TEST_ASSERT_STREQUAL(dialogue0.GetReply(i), "")
    }
    return 0;
}