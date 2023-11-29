#ifndef AUDIO_SERVER_H
#define AUDIO_SERVER_H
#include "basic.hpp"

enum SFX_ID {
    SFX_INVALID = -1,

    SFX_CLICK_BUTTON = 0,
    SFX_ERROR,
    SFX_CLICK_SHORT,
    SFX_CANCEL,

    SFX_MAX
};

struct AudioServer {
    int LoadSFX(const char* directory_name);
    void PlaySFX(SFX_ID id);

    Sound sound_array[SFX_MAX];
};

AudioServer* GetAudioServer();
void PlaySFX(SFX_ID id);

#endif  // AUDIO_SERVER_H