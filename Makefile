CC = g++
CFLAGS = -g -Wall -Wextra -Wno-narrowing -std=c++11
INCLUDES = -I /usr/local/Cellar/sdl2/2.0.16/include \
 -I /usr/local/Cellar/sdl2_image/2.0.5/include
LINKS = -L /usr/local/Cellar/sdl2/2.0.16/lib -L /usr/local/Cellar/sdl2_image/2.0.5/lib
LINK_FLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf
ARGS =

MAINFILE = main.cpp
APP = main
SRC_FILES = $(MAINFILE)
OBJ_FILES = NC/NC.a main.o SDL2_gfx/SDL2_gfx.a SDL_FontCache_Fork/SDL_FontCache.o
EMSC_OBJ_FILES = NC/NC.emsc.a SDL2_gfx/SDL2_gfx.emsc.a SDL_FontCache_Fork/SDL_FontCache.emsc.o

.c.o: $(SRC_FILES)
	$(CC) $(CFLAGS) $(INCLUDES) -c $^
all:
	$(CC) $(CFLAGS) -o $(APP) ${LINKS} $(OBJ_FILES) $(LINK_FLAGS)
clean:
	rm -rf $(OBJ_FILES) $(APP)
	rm build/game.html
	rm build/game.js
	rm build/game.wasm
deploy: 
	make .c.o && make all && ./$(APP) $(ARGS)
$(APP):
	make deploy

buildNC:
	(cd NC && make all)
  
asm:
	$(CC) -g -Wall -Wextra -S -Wa,-alh new.cpp > new.txt

sourceEMCC:
	cd ~/emsdk && ./emsdk activate
web:
	em++ $(CFLAGS) -DBUILD_EMSCRIPTEN main.cpp $(EMSC_OBJ_FILES)  \
	-o build/game.html \
	$(INCLUDES) \
 	-I /usr/local/Cellar/emscripten/include \
	-s LLD_REPORT_UNDEFINED $(LINK_FLAGS) \
	--preload-file assets \
	-Wall -g -lm -s \
	USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s SDL2_IMAGE_FORMATS='["jpg","png"]' \
	--use-preload-plugins