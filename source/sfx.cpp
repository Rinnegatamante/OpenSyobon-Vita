#include <stdlib.h>
#include <stdio.h>
#include <vitasdk.h>
#include "sfx.h"
#include "filesystem.h"
#include "audio_decoder.h"

#define AUDIO_BUFSIZE 8192 // Max dimension of audio buffer size
#define NSAMPLES 2048 // Number of samples for output
#define AUDIO_CHANNELS 7 // PSVITA has 8 available audio channels (1 BGM, 7 SFX)

SFX_s SFX[NUMSFX];
typedef struct Music{
	SFX_s* sfx;
	bool isPlaying;
	bool isNewTrack;
	uint8_t audiobuf1[AUDIO_BUFSIZE];
	uint8_t audiobuf2[AUDIO_BUFSIZE];
	uint8_t* cur_audiobuf;
}Music;

typedef struct Sound{
	SFX_s* sfx;
	bool isPlaying;
	bool isFinished;
	uint8_t audiobuf1[AUDIO_BUFSIZE];
	uint8_t audiobuf2[AUDIO_BUFSIZE];
	uint8_t* cur_audiobuf;
	std::unique_ptr<AudioDecoder> decoder;
}Sound;

SceUID BGM_Mutex, BGM_Thread, SFX_Mutex, SFX_Mutex_ID, sfx_threads[AUDIO_CHANNELS];
int bgm_chn = 0xDEAD;
bool soundEnabled;
Music* BGM;
bool termStream = false;
std::array<Sound, AUDIO_CHANNELS> sfx_sounds;
std::unique_ptr<AudioDecoder> bgm_decoder;

// BGM Thread
static int mainAudioThread(unsigned int args, void* arg){

	int samplerate, chns;
	FILE* file_hdl = NULL;
	AudioDecoder::Format format;
	
	for(;;) {

		// A pretty bad way to close thread
		if(termStream){
			termStream = false;
			if (file_hdl != NULL) bgm_decoder.reset();
			if (bgm_chn != 0xDEAD){
				sceAudioOutReleasePort(bgm_chn);
				bgm_chn = 0xDEAD;
			}
			sceKernelExitDeleteThread(0);
		}

		sceKernelWaitSema(BGM_Mutex, 1, NULL);
		if (BGM == NULL || (!BGM->isPlaying)){
			sceKernelSignalSema(BGM_Mutex, 1);
			continue;
		}

		if (BGM->isNewTrack){
			if (file_hdl != NULL) bgm_decoder.reset();
			file_hdl = fopen(BGM->sfx->filename, "rb");

			// Initializing audio decoder
			bgm_decoder = AudioDecoder::Create(file_hdl, "Track");
			bgm_decoder->Open(file_hdl);
			bgm_decoder->SetLooping(true);
			
			samplerate = 48000;
			chns = 2;
			if (!bgm_decoder->SetFormat(48000, AudioDecoder::Format::S16, 2)) bgm_decoder->GetFormat(samplerate, format, chns);		
			uint8_t audio_mode = chns > 1 ? SCE_AUDIO_OUT_MODE_STEREO : SCE_AUDIO_OUT_MODE_MONO;
			int nsamples = AUDIO_BUFSIZE / ((audio_mode+1)<<1);
			if (bgm_chn == 0xDEAD) bgm_chn = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, nsamples, samplerate, audio_mode);
			sceAudioOutSetConfig(bgm_chn, nsamples, samplerate, audio_mode);
			int vol_stereo[] = {32767, 32767};
			sceAudioOutSetVolume(bgm_chn, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vol_stereo);
			BGM->isNewTrack = false;
			memset(BGM->audiobuf1, 0, AUDIO_BUFSIZE);
			memset(BGM->audiobuf2, 0, AUDIO_BUFSIZE);
			BGM->cur_audiobuf = BGM->audiobuf1;
		}

		// Audio streaming feature
		if (BGM->sfx != NULL){
			if (BGM->cur_audiobuf == BGM->audiobuf1) BGM->cur_audiobuf = BGM->audiobuf2;
			else BGM->cur_audiobuf = BGM->audiobuf1;
			bgm_decoder->Decode(BGM->cur_audiobuf, AUDIO_BUFSIZE);	
			sceAudioOutOutput(bgm_chn, BGM->cur_audiobuf);
		}

		sceKernelSignalSema(BGM_Mutex, 1);

	}
}

// SFX Thread
volatile bool mustExit = false;
volatile uint8_t sfx_exited = 0;
static int sfxThread(unsigned int args, void* arg){
	int ch = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, 64, 48000, SCE_AUDIO_OUT_MODE_STEREO);
	if (ch < 0) sceKernelExitDeleteThread(0);
	
	int samplerate, chns;
	FILE* file_hdl = NULL;
	AudioDecoder::Format format;
	
	for (;;){
		sceKernelWaitSema(SFX_Mutex, 1, NULL);

		// Check if the thread must be closed
		if (mustExit){
			sfx_exited++;
			if (sfx_exited < AUDIO_CHANNELS) sceKernelSignalSema(SFX_Mutex, 1);
			else mustExit = false;
			sceAudioOutReleasePort(ch);
			sceKernelExitDeleteThread(0);
		}

		// Pick the next SE that wants to be played
		sceKernelWaitSema(SFX_Mutex_ID, 1, NULL);
		int idx = -1;
		for (int i = 0; i < AUDIO_CHANNELS; ++i) {
			if (!sfx_sounds[i].isPlaying && !sfx_sounds[i].isFinished) {
				idx = i;
				break;
			}
		}

		if (idx == -1) {
			sceKernelSignalSema(SFX_Mutex_ID, 1);
			continue;
		} else {
			sfx_sounds[idx].isPlaying = true;
			sceKernelSignalSema(SFX_Mutex_ID, 1);
		}

		Sound& sfx = sfx_sounds[idx];
		
		// Initializing audio decoder
		if (file_hdl != NULL) sfx.decoder.reset();
		file_hdl = fopen(sfx.sfx->filename, "rb");
		if (file_hdl == NULL) continue;
		sfx.decoder = AudioDecoder::Create(file_hdl, "Track");
		sfx.decoder->Open(file_hdl);
		sfx.decoder->SetLooping(false);
		samplerate = 48000;
		chns = 2;
		if (!sfx.decoder->SetFormat(48000, AudioDecoder::Format::S16, 2)) sfx.decoder->GetFormat(samplerate, format, chns);		
			
		// Preparing audio port
		uint8_t audio_mode = chns > 2 ? SCE_AUDIO_OUT_MODE_STEREO : SCE_AUDIO_OUT_MODE_MONO;
		int nsamples = AUDIO_BUFSIZE / ((audio_mode+1)<<1);
		sceAudioOutSetConfig(ch, nsamples, samplerate, audio_mode);
		int vol_stereo[] = {32767, 32767};
		sceAudioOutSetVolume(ch, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vol_stereo);

		// Playing sound
		while (!sfx.isFinished){
			if (sfx.cur_audiobuf == sfx.audiobuf1) sfx.cur_audiobuf = sfx.audiobuf2;
			else sfx.cur_audiobuf = sfx.audiobuf1;
			sfx.decoder->Decode(sfx.cur_audiobuf, AUDIO_BUFSIZE);
			sceAudioOutOutput(ch, sfx.cur_audiobuf);
			if (sfx.decoder->IsFinished()) { // EoF
				// SE slot is free again
				sfx.isFinished = true;
				break;
			}
		}
	}

}

void initSound()
{
	int i;
	for(i=0;i<NUMSFX;i++)
	{
		SFX[i].used=false;
	}
	
	// Creating mutexs
	BGM_Mutex = sceKernelCreateSema("BGM Mutex", 0, 1, 1, NULL);
	SFX_Mutex = sceKernelCreateSema("SFX Mutex", 0, 0, 1, NULL);
	SFX_Mutex_ID = sceKernelCreateSema("SFX Mutex for ID read", 0, 1, 1, NULL);
	
	BGM = (Music*)malloc(sizeof(Music));
	BGM->sfx = NULL;
	BGM->isPlaying = false;
	
	// Starting audio threads for SFX
	for (int i=0;i < AUDIO_CHANNELS; i++){
		sfx_threads[i] = sceKernelCreateThread("SFX Thread", &sfxThread, 0x10000100, 0x10000, 0, 0, NULL);
		int res = sceKernelStartThread(sfx_threads[i], sizeof(sfx_threads[i]), &sfx_threads[i]);
		sfx_sounds[i].isPlaying = true;
		sfx_sounds[i].isFinished = true;
	}
	
	// Starting audio thread for BGM
	BGM_Thread = sceKernelCreateThread("BGM Thread", &mainAudioThread, 0x10000100, 0x10000, 0, 0, NULL);
	int res = sceKernelStartThread(BGM_Thread, sizeof(BGM_Thread), &BGM_Thread);
	if (res != 0) soundEnabled=false;
	else soundEnabled=true;
	
}

void exitSound(void)
{
	int i;
	for(i=0;i<NUMSFX;i++){
		if(SFX[i].used) SFX[i].used=false;
	}
	if(soundEnabled){
		termStream = true;
		mustExit = true;
		sceKernelWaitThreadEnd(BGM_Thread, NULL, NULL);
		sceKernelSignalSema(SFX_Mutex, 1);
		for (int i=0;i<AUDIO_CHANNELS;i++){
			sceKernelWaitThreadEnd(sfx_threads[i], NULL, NULL);
		}
		sceKernelDeleteSema(BGM_Mutex);
		free(BGM);
	}
}

void loadSFX(SFX_s* s, const char* filename)
{
	if(!s)return;
	sprintf(s->filename, "%s", filename);
	s->used = true;
	
}

SFX_s* createSFX(const char* filename)
{
	int i;
	for(i=0;i<NUMSFX;i++)
	{
		if(!SFX[i].used)
		{
			loadSFX(&SFX[i], filename);
			return &SFX[i];
		}
	}
	return NULL;
}

void playSFX(SFX_s* s)
{
	if(!s || !s->used || !soundEnabled) return;
	
	// Pick the next free SE slot.
	// Does not need synchronisation
	int idx = -1;
	for (int i = 0; i < AUDIO_CHANNELS; ++i) {
		if (sfx_sounds[i].isPlaying && sfx_sounds[i].isFinished) {
			idx = i;
			break;
		}
	}
	if (idx == -1) return;
	
	// Wait and signal are required to prevent reordering
	sceKernelWaitSema(SFX_Mutex_ID, 1, NULL);
	sfx_sounds[idx].isPlaying = false;
	sfx_sounds[idx].isFinished = false;
	sfx_sounds[idx].sfx = s;
	sceKernelSignalSema(SFX_Mutex_ID, 1);

	// Start one SE thread
	sceKernelSignalSema(SFX_Mutex, 1);
}

void playMSC(SFX_s* s)
{
	if(!s || !s->used || !soundEnabled) return;
	sceKernelWaitSema(BGM_Mutex, 1, NULL);
	BGM->sfx = s;
	BGM->isNewTrack = true;
	BGM->isPlaying = true;
	sceKernelSignalSema(BGM_Mutex, 1);
}

void stopSFX(void)
{
}

void stopMSC(void)
{
	BGM->isPlaying = false;
}