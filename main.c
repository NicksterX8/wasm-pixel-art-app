#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "SDL2_gfx/SDL2_gfxPrimitives.h"
#include "SDL_FontCache/SDL_FontCache.h"

#include "NC/SDLContext.h"
#include "NC/meta.h"
#include "NC/SDLBuild.h"

#define WINDOW_HIGH_DPI

FC_Font *FreeSans;
SDL_Texture* tex;

struct Context {
    SDLContext *sdlCtx;
    Uint8 *keyboard;
    struct MetaData *metaData;
};

void render(SDL_Renderer *ren) {
    // get window size in pixels
    int renWidth;
    int renHeight;
    SDL_GetRendererOutputSize(ren, &renWidth, &renHeight);

    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderClear(ren);

    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
    SDL_RenderDrawLine(ren, 100, 200, 400, 300);

    circleRGBA(ren, 402, 202, 30, 255, 0, 255, 255);

    SDL_Rect dst = {
        300,
        450,
        128,
        128
    };
    SDL_RenderCopy(ren, tex, NULL, &dst);

    FC_Draw(FreeSans, ren, renWidth/2, renHeight/2, "Hello World!");

    SDL_RenderPresent(ren);
}

// Main game loop
int update(SDLContext *sdlCtx, Uint8 *keyboard, struct MetaData *metaData) {
    updateMetaData(metaData);

    bool quit = false;

    // handle events //
    // get user input state for this update
    int mouseX,mouseY;
    Uint32 mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);

    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        switch(e.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    SDL_Log("window resized.");
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                SDL_Log("Mouse clicked at %d,%d", mouseX, mouseY);
                break;
            default:
                break;
        }
    }

    render(sdlCtx->ren);

    if (quit) {
        SDL_Log("update func sending quit");
        return 1;
    }
    return 0;
}

// update wrapper function to unwrap the void pointer main loop parameter into its properties
int updateWrapper(void *param) {
    struct Context *ctx = (struct Context*)param;
    return update(ctx->sdlCtx, ctx->keyboard, ctx->metaData);
}

// load assets and stuff
int load(SDL_Renderer *ren, float renderScale) {
    tex = IMG_LoadTexture(ren, "assets/pixelart/WatermelonSlice.png");
    FC_NewFont(&FreeSans, ren, "assets/fonts/FreeSans.ttf", 24*renderScale, FC_MakeColor(0,0,0,255), 0);
    
    return 0;
}

// unload assets and stuff
void unload() {
    SDL_DestroyTexture(tex);
    FC_FreeFont(FreeSans);
}

int main() {
    SDLSettings settings = DefaultSDLSettings;
    settings.windowTitle = "Hello There!";
    settings.windowIconPath = "assets/pixelart/gold-bullet.png";
#ifdef WINDOW_HIGH_DPI
    settings.allowHighDPI = true;
#endif
    SDLContext sdlCtx = initSDLAndContext(&settings); // SDL Context
    struct MetaData metaData = initMetaData();
    
    struct Context context;
    context.sdlCtx = &sdlCtx;
    context.metaData = &metaData;
    
    load(context.sdlCtx->ren, sdlCtx.scale);

    
    // NC_SetMainLoop(updateWrapper, &context);

    unload();

    quitSDLContext(sdlCtx);

    return 0;
}