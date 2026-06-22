#include "sound.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <iostream>

ma_engine engine;
bool sound_initialized = false;

static ma_sound_group omnitrix_group;
static ma_sound_group jump_group;
static ma_sound_group detransform_group;

ma_sound ice_breath_sound;
bool ice_breath_initialized = false;

ma_sound oops_sound;
bool oops_initialized = false;

ma_sound bgm_sound;
bool bgm_initialized = false;

std::string next_music_file;
bool is_transitioning = false;
ma_sound transition_sound;
bool transition_initialized = false;

void InitSoundSystem() {
    ma_result result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize sound engine." << std::endl;
        return;
    }

    ma_engine_set_volume(&engine, 0.5f);

    ma_sound_group_init(&engine, 0, NULL, &omnitrix_group);
    ma_sound_group_set_volume(&omnitrix_group, 0.3f); // Decrease omnitrix volume

    ma_sound_group_init(&engine, 0, NULL, &jump_group);
    ma_sound_group_set_volume(&jump_group, 0.2f); // Decrease jump volume

    ma_sound_group_init(&engine, 0, NULL, &detransform_group);
    ma_sound_group_set_volume(&detransform_group, 1.5f); // Increase detransform volume

    if (ma_sound_init_from_file(&engine, "../../data/sounds/ice_breath.mp3", MA_SOUND_FLAG_DECODE, NULL, NULL, &ice_breath_sound) == MA_SUCCESS) {
        ma_sound_set_looping(&ice_breath_sound, MA_TRUE);
        ice_breath_initialized = true;
    }

    if (ma_sound_init_from_file(&engine, "../../data/sounds/omnitrix_oops.wav", MA_SOUND_FLAG_DECODE, NULL, NULL, &oops_sound) == MA_SUCCESS) {
        ma_sound_set_volume(&oops_sound, 0.4f);
        oops_initialized = true;
    }

    sound_initialized = true;
    std::cout << "Sound system initialized successfully." << std::endl;
}

void CleanupSoundSystem() {
    if (sound_initialized) {
        if (ice_breath_initialized) ma_sound_uninit(&ice_breath_sound);
        if (oops_initialized) ma_sound_uninit(&oops_sound);
        if (bgm_initialized) ma_sound_uninit(&bgm_sound);
        if (transition_initialized) ma_sound_uninit(&transition_sound);
        ma_sound_group_uninit(&omnitrix_group);
        ma_sound_group_uninit(&jump_group);
        ma_sound_group_uninit(&detransform_group);
        ma_engine_uninit(&engine);
        sound_initialized = false;
    }
}

void PlaySoundEffect(const std::string& filepath) {
    if (!sound_initialized) return;
    ma_engine_play_sound(&engine, filepath.c_str(), NULL);
}

void PlayOmnitrixSound(const std::string& filepath) {
    if (!sound_initialized) return;
    ma_engine_play_sound(&engine, filepath.c_str(), &omnitrix_group);
}

void PlayJumpSound(const std::string& filepath) {
    if (!sound_initialized) return;
    ma_engine_play_sound(&engine, filepath.c_str(), &jump_group);
}

void PlayDetransformSound(const std::string& filepath) {
    if (!sound_initialized) return;
    ma_engine_play_sound(&engine, filepath.c_str(), &detransform_group);
}

void StartIceBreath() {
    if (!ice_breath_initialized) return;
    if (!ma_sound_is_playing(&ice_breath_sound)) {
        ma_sound_start(&ice_breath_sound);
    }
}

void StopIceBreath() {
    if (!ice_breath_initialized) return;
    if (ma_sound_is_playing(&ice_breath_sound)) {
        ma_sound_stop(&ice_breath_sound);
        ma_sound_seek_to_pcm_frame(&ice_breath_sound, 0);
    }
}

void PlayOopsSound() {
    if (!oops_initialized) return;
    ma_sound_seek_to_pcm_frame(&oops_sound, 0);
    ma_sound_start(&oops_sound);
}

bool IsOopsSoundFinished() {
    if (!oops_initialized) return true;
    return ma_sound_at_end(&oops_sound);
}

void PlayMusic(const std::string& filepath, bool loop) {
    if (!sound_initialized) return;
    
    is_transitioning = false;
    if (bgm_initialized) {
        ma_sound_uninit(&bgm_sound);
        bgm_initialized = false;
    }
    
    if (ma_sound_init_from_file(&engine, filepath.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &bgm_sound) == MA_SUCCESS) {
        ma_sound_set_looping(&bgm_sound, loop ? MA_TRUE : MA_FALSE);
        ma_sound_set_volume(&bgm_sound, 0.4f); 
        ma_sound_start(&bgm_sound);
        bgm_initialized = true;
    }
}

void PlayTransition(const std::string& transition_file, const std::string& loop_file) {
    if (!sound_initialized) return;
    StopMusic();
    next_music_file = loop_file;
    
    if (transition_initialized) {
        ma_sound_uninit(&transition_sound);
        transition_initialized = false;
    }
    if (ma_sound_init_from_file(&engine, transition_file.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &transition_sound) == MA_SUCCESS) {
        ma_sound_set_volume(&transition_sound, 0.4f);
        ma_sound_start(&transition_sound);
        transition_initialized = true;
        is_transitioning = true;
    }
}

void UpdateSoundSystem() {
    if (is_transitioning && transition_initialized) {
        if (ma_sound_at_end(&transition_sound)) {
            is_transitioning = false;
            PlayMusic(next_music_file, true);
        }
    }
}

void StopMusic() {
    if (bgm_initialized && ma_sound_is_playing(&bgm_sound)) {
        ma_sound_stop(&bgm_sound);
    }
    if (transition_initialized && ma_sound_is_playing(&transition_sound)) {
        ma_sound_stop(&transition_sound);
    }
    is_transitioning = false;
}
