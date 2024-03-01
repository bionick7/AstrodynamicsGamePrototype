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

static const char* sound_paths[] = {
    "resources/sound/SFX/Click_Standard_00.wav",    // SFX_CLICK_BUTTON
    "resources/sound/SFX/Click_Electronic_01.wav",  // SFX_ERROR
    "resources/sound/SFX/ClickMechanic1.wav",       // SFX_CLICK_SHORT
    "resources/sound/SFX/Click_Electronic_14.wav",  // SFX_CANCEL
};

struct AudioServer {
    void PlaySFX(SFX_ID id);
    void StartMusic();
    void Update(double delta_t);

    Music current_ambience;
};

void PlaySFX(SFX_ID id);

#endif  // AUDIO_SERVER_H