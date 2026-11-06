#pragma once

#include <stddef.h>

struct Bank {
  const char *const *entries;
  size_t count;
  bool phonetic;
  const char *const *labels;
};

template <size_t N>
constexpr Bank bank(const char *const (&entries)[N], bool phonetic = false) {
  return {entries, N, phonetic, nullptr};
}

template <size_t N>
constexpr Bank bank(const char *const (&entries)[N], const char *const (&labels)[N]) {
  return {entries, N, true, labels};
}

constexpr const char *numbers[] = {
    "ZERO", "ONE", "TWO", "THREE", "FOUR",
    "FIVE", "SIX", "SEVEN", "EIGHT", "NINE",
};

constexpr const char *alphabet[] = {
    "EY4", "BIY4", "SIY4", "DIY4", "IY4", "EH4F", "JIY4",
    "EY4CH", "AY4", "JEY4", "KEY4", "EH4L", "EH4M", "EH4N",
    "OW4", "PIY4", "KYUW4", "AA4R", "EH4S", "TIY4", "YUW4",
    "VIY4", "DAH4BUL YUW4", "EH4KS", "WAY4", "ZIY4",
};

constexpr const char *alphabetLabels[] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
    "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
};

constexpr const char *nato[] = {
    "ALFA", "BRAVO", "CHARLIE", "DELTA", "ECHO", "FOXTROT", "GOLF", "HOTEL",
    "INDIA", "JULIETT", "KILO", "LIMA", "MIKE", "NOVEMBER", "OSCAR", "PAPA",
    "QUEBEC", "ROMEO", "SIERRA", "TANGO", "UNIFORM", "VICTOR", "WHISKEY",
    "X RAY", "YANKEE", "ZULU",
};

constexpr const char *words[] = {
    "HELLO", "GOODBYE", "YES", "NO", "PLEASE", "THANKS", "ROBOT", "ECHO",
    "MACHINE", "CIRCUIT", "VOICE", "SPEECH", "MUSIC", "RHYTHM", "DREAM", "FUTURE",
};

constexpr const char *commands[] = {
    "START", "STOP", "WAIT", "GO", "TURN LEFT", "TURN RIGHT", "SAY IT",
    "TRY AGAIN", "LISTEN", "SPEAK", "REPEAT", "RESET",
};

constexpr const char *computer[] = {
    "SYSTEM READY", "ACCESS GRANTED", "ACCESS DENIED", "SYNTAX ERROR",
    "FILE NOT FOUND", "MEMORY FULL", "LOADING", "PLEASE WAIT",
    "INPUT REQUIRED", "PROCESS COMPLETE", "REBOOTING", "SYSTEM OFFLINE",
};

constexpr const char *radio[] = {
    "RADIO CHECK", "DO YOU COPY", "COPY THAT", "LOUD AND CLEAR", "STAND BY",
    "SAY AGAIN", "ROGER", "AFFIRMATIVE", "NEGATIVE", "TRANSMISSION LOST",
    "SIGNAL FOUND", "OVER AND OUT",
};

constexpr const char *vowels[] = {
    "IY4", "IH4", "EH4", "AE4", "AA4", "AH4", "AO4", "OH4", "UH4", "UX4",
    "ER4", "EY4", "AY4", "OY4", "AW4", "OW4", "UW4",
};

constexpr const char *vowelLabels[] = {
    "IY", "IH", "EH", "AE", "AA", "AH", "AO", "OH", "UH", "UX",
    "ER", "EY", "AY", "OY", "AW", "OW", "UW",
};

constexpr const char *consonants[] = {
    "S", "SH", "F", "TH", "/H", "/X", "Z", "ZH", "V", "DH",
    "M", "N", "NX", "T", "K", "P", "CH", "J",
};

constexpr const char *percussion[] = {
    "BUW4M", "TS", "KAA4", "PSH", "PUH4", "TK", "KSH",
    "DAH4M", "CH TS", "TAH4 KAA4", "PUH4 TS KAH4 TS", "BAE4P BAE4P",
};

constexpr const char *percussionLabels[] = {
    "BOOM", "TSS", "KA", "PSH", "PUH", "TK", "KSH", "DUM",
    "CH TSS", "TA KA", "PUH TSS KA TSS", "BAP BAP",
};

constexpr Bank banks[] = {
    bank(numbers), bank(alphabet, alphabetLabels), bank(nato), bank(words), bank(commands),
    bank(computer), bank(radio), bank(vowels, vowelLabels), bank(consonants, true),
    bank(percussion, percussionLabels),
};

constexpr size_t bankCount = sizeof(banks) / sizeof(banks[0]);
