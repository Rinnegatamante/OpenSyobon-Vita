#include "osvita.h"


#include <sys/stat.h>
#include "Roboto_Regular_ttf.h"


vita2d_font *font;
int sound;

SFX_s* currentMusic;
int musicPaused;

int osvita_Init()
{
	
	scePowerSetArmClockFrequency(444);
    vita2d_init();
	vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));
	vita2d_set_vblank_wait(0);
	
	// In case game dir doesn't exist
    sceIoMkdir("ux0:/data/OpenSyobon", 0777);

    font = vita2d_load_font_mem(Roboto_Regular_ttf, Roboto_Regular_ttf_size);

	initSound();	
	sound = true;
	currentMusic = NULL;
	musicPaused=0;
    
    return 0;
}

//Fonts
byte fontsize = 0;

//Strings
void SetFontSize(byte size)
{
    fontsize = size;
}

byte fontType = 0;
void ChangeFontType(byte type)
{
    fontType = type;
}

void DrawString(int a, int b, const char *x, int c)
{
	int shadow;
	
	shadow = (c==RGBA8(0x0, 0x0, 0x0, 0xff))?RGBA8(0xff, 0xff, 0xff, 0xff):RGBA8(0x0, 0x0, 0x0, 0xff);

	vita2d_font_draw_text(font, 192 + a/scale-1, b/scale-1+y_off, shadow, fontsize/scale, x);
	vita2d_font_draw_text(font, 192 + a/scale, b/scale-1+y_off, shadow, fontsize/scale, x);
	vita2d_font_draw_text(font, 192 + a/scale+1, b/scale-1+y_off, shadow, fontsize/scale, x);
	vita2d_font_draw_text(font, 192 + a/scale-1, b/scale+y_off, shadow, fontsize/scale, x);
	vita2d_font_draw_text(font, 192 + a/scale+1, b/scale+y_off, shadow, fontsize/scale, x);
	vita2d_font_draw_text(font, 192 + a/scale-1, b/scale+1+y_off, shadow, fontsize/scale, x);
	vita2d_font_draw_text(font, 192 + a/scale, b/scale+1+y_off, shadow, fontsize/scale, x);
	vita2d_font_draw_text(font, 192 + a/scale+1, b/scale+1+y_off, shadow, fontsize/scale, x);

	vita2d_font_draw_text(font, 192 + a/scale, b/scale+y_off, c, fontsize/scale, x);

}

void DrawFormatString(int a, int b, int color, const char *str, ...)
{
    va_list args;
    char *newstr = new char[strlen(str) + 16];
    va_start(args, str);
    vsprintf(newstr, str, args);
    va_end(args);
    DrawString(a, b, newstr, color);
    delete newstr;
}

bool ex = false;

SceCtrlData pad;
SceCtrlData oldpad;

void UpdateKeys()
{
	oldpad = pad;
	sceCtrlPeekBufferPositive(0, &pad, 1);
}

byte ProcessMessage()
{
    return 0;
}

byte CheckHitKey(int key)
{
    return ((oldpad.buttons&key) && (pad.buttons&key))?1:0;
}

byte CheckDownKey(int key)
{
    return ((!(oldpad.buttons&key)) && (pad.buttons&key))?1:0;
}

int WaitKey()
{
	SceCtrlData pad;
	pad.buttons = 0;
    while (!pad.buttons) sceCtrlPeekBufferPositive(0, &pad, 1);
	
	return pad.buttons; 	
}

void DrawBG(int a, int b, vita2d_texture * mx)
{
	if(mx)
    {
		vita2d_draw_texture_scale(mx, a,b, 2.4, 2.4);
	}
}

void DrawBG(int a, int b, t_graph * mx)
{
	if(mx)
    {
		vita2d_draw_texture_part_scale(mx->texture, a,b,mx->x, mx->y, mx->width, mx->height, 2.4, 2.4);
	}
}


void DrawGraphZ(int a, int b, vita2d_texture * mx)
{
	if(mx)
    {
		vita2d_draw_texture_part_scale_rotate(mx, 192 + (a+vita2d_texture_get_width(mx)/2)/scale, (b+vita2d_texture_get_height(mx)/2)/scale+y_off, 0, 0, vita2d_texture_get_width(mx), vita2d_texture_get_height(mx), 1.08/scale, 1.08/scale, 0);
	}
}
void DrawGraphZ(int a, int b, t_graph * mx)
{
	if(mx)
    {
		vita2d_draw_texture_part_scale_rotate(mx->texture, 192 + (a+mx->width/2)/scale, (b+mx->height/2)/scale+y_off, mx->x, mx->y, mx->width, mx->height, 1.08/scale, 1.08/scale, 0);
	}
}

void DrawTurnGraphZ(int a, int b, vita2d_texture * mx)
{
    if(mx)
    {

		vita2d_draw_texture_part_scale_rotate(mx, 192 + (a+vita2d_texture_get_width(mx)/2)/scale, (b+vita2d_texture_get_height(mx)/2)/scale+y_off, 0, 0, vita2d_texture_get_width(mx), vita2d_texture_get_height(mx), 1.08/scale, 1.08/scale, 0);
    }
}

void DrawTurnGraphZ(int a, int b, t_graph * mx)
{
    if(mx)
    {

		vita2d_draw_texture_part_scale_rotate(mx->texture, 192 + (a+mx->width/2)/scale, (b+mx->height/2)/scale+y_off, mx->x, mx->y, mx->width, mx->height, -1.08/scale, 1.08/scale, 0);
    }
}

void DrawVertTurnGraph(int a, int b, vita2d_texture * mx)
{
    if(mx)
    {
		vita2d_draw_texture_part_scale_rotate(mx, 192 + (a+vita2d_texture_get_width(mx)/2)/scale, (b+vita2d_texture_get_height(mx)/2)/scale+y_off, 0, 0, vita2d_texture_get_width(mx), vita2d_texture_get_height(mx), 1.08/scale, -1.08/scale, 0);
    }
}

void DrawVertTurnGraph(int a, int b, t_graph * mx)
{
    if(mx)
    {
		vita2d_draw_texture_part_scale_rotate(mx->texture, 192 + (a+mx->width/2)/scale, (b+mx->height/2)/scale+y_off, mx->x, mx->y, mx->width, mx->height, 1.08/scale, -1.08/scale, 0);
    }
}

t_graph *DerivationGraph(int srcx, int srcy, int width, int height,
			     vita2d_texture * src)
{

	if(!src) return NULL;
	t_graph * g;
	g = (t_graph*) malloc(sizeof(t_graph));
	g->texture=src;
	g->x=srcx;
	g->y=srcy;
	g->width=width;
	g->height=height;
	return g;
}


void PlaySoundMem(SFX_s* s, int l)
{
   if (s)
	if(sound && s->used) playSFX(s);
}

void Mix_PlayMusic(SFX_s* s, int l)
{
   if (s)
	if(sound && s->used) { 
		currentMusic = s;
		playMSC(s);
	}
}

void StopSoundMem(SFX_s* s)
{
	stopSFX();
}

void Mix_HaltMusic(void)
{
	stopMSC();
	currentMusic = NULL;
}

