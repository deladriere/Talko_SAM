#pragma once

#include <stdint.h>

constexpr unsigned SpeechSampleRate = 22050;

struct Voice {
  uint8_t speed;
  uint8_t pitch;
  uint8_t mouth;
  uint8_t throat;
};

constexpr Voice voices[] = {
    {72, 64, 128, 128},  // S.A.M.
    {72, 64, 160, 110},  // Elf
    {92, 60, 190, 190},  // Robot
    {82, 72, 105, 110},  // Stuffy
    {82, 32, 145, 145},  // Little Old Lady
    {100, 64, 200, 150}, // Alien
};

constexpr int voiceCount = sizeof(voices) / sizeof(voices[0]);

void setupSpeech();
void speakStartup(const char *phrase);
void startSpeechWorker();
void triggerSpeech(const char *phrase, const char *label, bool phonetic, uint8_t voice);
void serviceSpeechConsole();
void stopSpeech();
void setSpeechControls(uint16_t speedScale, uint16_t pitchScale);
void setErrorHeld(bool held);
bool isSpeaking();
bool isStuttering();
