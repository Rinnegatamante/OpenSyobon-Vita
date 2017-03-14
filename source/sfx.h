#ifndef SFX_H
#define SFX_H

#define NUMSFX (32)

typedef struct
{
	char filename[128];
	bool used;
}SFX_s;

void initSound(void);
void exitSound(void);
void loadSFX(SFX_s* s, const char* filename);
SFX_s* createSFX(const char* filename);
void playSFX(SFX_s* s);
void playMSC(SFX_s* s);
void stopSFX(void);
void stopMSC(void);

#endif
