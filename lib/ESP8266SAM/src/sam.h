#ifndef SAM_H
#define SAM_H

#ifdef __cplusplus
extern "C" {
#endif

void SetInput(char *_input);
void SetSpeed(unsigned char _speed);
void SetPitch(unsigned char _pitch);
void SetMouth(unsigned char _mouth);
void SetThroat(unsigned char _throat);
void EnableSingmode(int x);

int SAMMain( void (*cb)(void *, unsigned char), void *cbdata );

int GetBufferLength();

void GetPhonemeCode(unsigned char index, char code[3]);

int SAMPrepare();



#ifdef __cplusplus
}
#endif


#endif
