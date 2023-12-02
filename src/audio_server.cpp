#include "audio_server.hpp"
#include "logging.hpp"

AudioServer audio_server;

int AudioServer::LoadSFX(const char* directory_name) {
    sound_array[SFX_CLICK_BUTTON] = LoadSound("resources/sound/SFX/Click_Standard_00.wav");
    sound_array[SFX_ERROR]        = LoadSound("resources/sound/SFX/Click_Electronic_01.wav");
    sound_array[SFX_CLICK_SHORT]  = LoadSound("resources/sound/SFX/ClickMechanic1.wav");
    sound_array[SFX_CANCEL]       = LoadSound("resources/sound/SFX/Click_Electronic_14.wav");
    SetSoundVolume(sound_array[SFX_CLICK_SHORT], 0.3);
}

void AudioServer::StartMusic() {
    current_ambience = LoadMusicStream("resources/sound/ambient/Electricity, Buzz & Hum, Motor, Electric, Neon, Loop SND5749.wav");
    SetMusicVolume(current_ambience, 0.2);
    //PlayMusicStream(current_ambience);   
}

void AudioServer::Update(double dt) {
    UpdateMusicStream(current_ambience);
}

void AudioServer::PlaySFX(SFX_ID id) {
    if (id < 0 || id >= SFX_MAX) {
        ERROR("Invalid SFX id: %d (max is %d)", id, SFX_MAX - 1)
    }
    PlaySound(sound_array[id]);
}

AudioServer* GetAudioServer() {
    return &audio_server;
}

void PlaySFX(SFX_ID id) { return audio_server.PlaySFX(id); }