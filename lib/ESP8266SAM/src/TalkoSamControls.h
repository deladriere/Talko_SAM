#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int TalkoSamCancelled(void);
void TalkoSamGrowl(uint8_t *samples, uint8_t count, uint8_t frame);
uint16_t TalkoSamSpeed(unsigned char baseSpeed);
uint16_t TalkoSamPitchPeriod(unsigned char framePeriod);

#ifdef __cplusplus
}
#endif
