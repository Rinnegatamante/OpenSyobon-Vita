TARGET		:= OpenSyobon
TITLE		:= SYOBONCAT
SOURCES		:= source
INCLUDES	:= include

PKG_CONFIG = arm-vita-eabi-pkg-config
FREETYPE_LIBS := $(shell $(PKG_CONFIG) --static --libs freetype2)

LIBS = -lvorbisfile -lvorbis -logg -lspeexdsp -lvita2d \
	-lSceAppMgr_stub -lSceSysmodule_stub -lSceCtrl_stub \
	-lSceLibKernel_stub -lm -lSceAppUtil_stub -lScePgf_stub \
	-ljpeg $(FREETYPE_LIBS) -lc -lScePower_stub -lSceCommonDialog_stub -lpng16 -lz \
	-lSceAudio_stub -lSceGxm_stub -lSceDisplay_stub

CFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -fno-lto -g -Wl,-q -O3 -DHAVE_OGGVORBIS -DHAVE_LIBSPEEXDSP -DUSE_AUDIO_RESAMPLER
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11 -fpermissive
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

$(TARGET).vpk: $(TARGET).velf
	vita-make-fself -s $< builder/eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE) "$(TARGET)" builder/sce_sys/param.sfo
	cp -f builder/sce_sys/param.sfo param.sfo
	cp -f builder/eboot.bin eboot.bin
	vita-pack-vpk -s builder/sce_sys/param.sfo -b builder/eboot.bin \
		--add builder/sce_sys/icon0.png=sce_sys/icon0.png \
		--add builder/sce_sys/livearea=sce_sys/livearea \
		--add builder/splash.png=splash.png \
		--add builder/BGM=BGM --add builder/SE=SE --add builder/res=res \
		$@

%.velf: %.elf
	cp $< $<.unstripped.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS)
