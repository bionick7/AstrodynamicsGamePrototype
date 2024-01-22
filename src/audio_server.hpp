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
    int LoadSFX();
    void PlaySFX(SFX_ID id);
    void StartMusic();
    void Update(double delta_t);

    Music current_ambience;
    Sound sound_array[SFX_MAX];
};

void PlaySFX(SFX_ID id);

#endif  // AUDIO_SERVER_H