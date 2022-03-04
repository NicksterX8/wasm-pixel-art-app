FILES = SDL2_framerate SDL2_gfxPrimitives SDL2_rotozoom SDL2_imageFilter
EMSC_CFLAGS =
CFLAGS = -g -Wall -Werror

SRC_FILES = $(FILES:=.c)
OBJ_FILES = $(FILES:=.o)
EMSC_OBJ_FILES = $(FILES:=.emsc.o)

LIB = SDL2_gfx.a
EMSC_LIB = SDL2_gfx.emsc.a

all:
	make full
	make full_emsc

full:
	rm -f $(LIB)
	make compile && make $(LIB)
full_emsc:
	rm -f $(EMSC_LIB)
	make compile_emsc && make $(EMSC_LIB)

$(LIB):
	ar cr $@ $(OBJ_FILES)
$(EMSC_LIB):
	emar rcs $@ $(EMSC_OBJ_FILES)

compile: $(OBJ_FILES)
compile_all: $(FILES)
compile_emsc: $(EMSC_OBJ_FILES)

%: %.c %.o %.emsc.o
	make $@.o && make $@.emsc.o

%.emsc.o : %.c
	emcc $(EMSC_CFLAGS) -c $< -o $@
%.o : %.c
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILES)
	rm -f $(EMSC_OBJ_FILES)
	rm -f $(LIB)
	rm -f $(EMSC_LIB)

compile_emsc_files:
	em++ $(CFLAGS) -c $(SRC_FILES)

static_lib:
	ar cr SDL2_gfx.a $(OBJ_FILES)

static_lib_emsc:
	emar rcs SDL2_gfx.emsc.a $(EMSC_OBJ_FILES)

compile_emsc: $(FILES)

