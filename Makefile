# Define the applications properties here:

APP_NAME = delta_teleprompter
DATA_DIR = /usr/share/$(APP_NAME)

# Define the compiler settings here:

CPP       = g++
CC        = gcc
LD        = g++
REAL_LD   = ld
OBJCOPY   = objcopy

SOURCE    = .

INCLUDE   = -I. -I/usr/include/SDL

W_OPTS    = -Wall -Wextra #-finline-functions -fomit-frame-pointer -fno-builtin -fno-exceptions
CC_OPTS   = -O0 $(INCLUDE) $(W_OPTS) -D_DEBUG -DDATA_DIR=\"$(DATA_DIR)\" -c -ggdb3 -fPIC
CPP_OPTS  = $(CC_OPTS)
CC_OPTS_A = $(CC_OPTS) -D_ASSEMBLER_

LIBS      = -lc -lm -lSDL -lSDL_gfx -lSDL_image -lSDL_ttf

LD_OPTS   = $(LIBS) -o $(APP_NAME)



# Find all source files

SRC_CPP = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.cpp))
SRC_C   = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.c))
SRC_S   = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.S))
OBJ_CPP = $(patsubst %.cpp, %.o, $(SRC_CPP))
OBJ_C   = $(patsubst %.c, %.o, $(SRC_C))
OBJ_S   = $(patsubst %.S, %.o, $(SRC_S))
DEP     = $(patsubst %.o, %.d, $(OBJ))
BMP     = $(foreach dir, $(SOURCE), $(wildcard $(dir)/gfx/*.bmp))
TGA     = $(patsubst %.bmp, %.tga, $(BMP))
TTF     = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.ttf))
OBJ_TTF = $(patsubst %.ttf, %.o, $(TTF))
OBJ     = $(OBJ_CPP) $(OBJ_C) $(OBJ_S) $(OBJ_TTF)

# Compile rules.

.PHONY : all

all : $(APP_NAME) $(TGA)

$(APP_NAME) : $(OBJ)
	$(LD) $(OBJ) $(LD_OPTS)

$(OBJ_CPP) : %.o : %.cpp
	$(CPP) $(CPP_OPTS) -o $@ $<
	@$(CPP) -MM $(CPP_OPTS) $*.cpp > $*.d

$(OBJ_C) : %.o : %.c
	$(CC) $(CC_OPTS) -o $@ $<
	@$(CC) -MM $(CC_OPTS) $*.c > $*.d

$(OBJ_S) : %.o : %.S
	$(CC) $(CC_OPTS_A) -o $@ $<
	@$(CC) -MM $(CC_OPTS_A) $*.S > $*.d

$(TGA) : %.tga : %.bmp
	convert $< $@

$(OBJ_TTF) : %.o : %.ttf
	# Convert ttf files directly to object
	$(REAL_LD) -r -b binary -o $@ $<
#	$(OBJCOPY) --rename-section .data=.rodata,alloc,load,readonly,data,contents $@ $@

-include $(DEP)

# Clean rules

.PHONY : clean

clean :
	rm -f $(OBJ) *.d $(APP_NAME)

INSTALL_DIR = delta_teleprompter_v101
INSTALL_FILES = README.md LICENSE $(APP_NAME) $(TGA)

.PHONY: install
install: $(APP_NAME)
	install -D -m755 delta_teleprompter /usr/bin
	install -m755 -d /usr/share/delta_teleprompter/gfx
#	install -m644 gfx/bg.png /usr/share/delta_teleprompter/gfx
#	install -m644 gfx/block?.png /usr/share/delta_teleprompter/gfx

.PHONY: tags
tags:
	ctags -R . 

DIST_NAME = delta_teleprompter_v111
%.tar.gz:
	tar -cvzf $@ $^

%.zip:
	zip -r $@ $^

