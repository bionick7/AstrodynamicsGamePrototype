#include "audio_server.hpp"
#include "global_state.hpp"
#include "logging.hpp"
#include "assets.hpp"


void AudioServer::StartMusic() {
    current_ambience = LoadMusicStream("resources/sound/ambient/Electricity, Buzz & Hum, Motor, Electric, Neon, Loop SND5749.wav");
    SetMusicVolume(current_ambience, 0.2);
    PlayMusicStream(current_ambience);
}

void AudioServer::Update(double dt) {
    UpdateMusicStream(current_ambience);
}

void AudioServer::PlaySFX(SFX_ID id) {
    if (id < 0 || id >= SFX_MAX) {
        ERROR("Invalid SFX id: %d (max is %d)", id, SFX_MAX - 1)
    }
    SetSoundVolume(assets::GetSound(sound_paths[SFX_CLICK_SHORT]), 0.2);
    PlaySound(assets::GetSound(sound_paths[id]));
}

void PlaySFX(SFX_ID id) { return GetAudioServer()->PlaySFX(id); }