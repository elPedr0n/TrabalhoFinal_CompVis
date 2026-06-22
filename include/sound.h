#ifndef SOUND_H
#define SOUND_H

#include <string>

void InitSoundSystem();
void PlaySoundEffect(const std::string& filepath);
void PlayOmnitrixSound(const std::string& filepath);
void PlayJumpSound(const std::string& filepath);
void PlayDetransformSound(const std::string& filepath);

void StartIceBreath();
void StopIceBreath();

void PlayOopsSound();
bool IsOopsSoundFinished();

void PlayMusic(const std::string& filepath, bool loop = true);
void PlayTransition(const std::string& transition_file, const std::string& loop_file);
void StopMusic();

void UpdateSoundSystem();
void CleanupSoundSystem();

#endif // SOUND_H
