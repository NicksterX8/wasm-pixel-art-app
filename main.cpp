#include <stdlib.h>
#include <vector>
#include <functional>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "SDL2_gfx/SDL2_gfxPrimitives.h"
#include "SDL_FontCache_Fork/SDL_FontCache.h"

#include "NC/cpp-vectors.hpp"
#include "NC/SDLContext.h"
#include "NC/meta.h"
#include "NC/SDLBuild.h"
#include "NC/colors.h"

#define WINDOW_HIGH_DPI

FC_Font *FreeSans;
FC_Font *TitleFont;
FC_Font* InfoFont;
SDL_Texture* tex;

#define SDL_PIXELFORMAT SDL_PIXELFORMAT_RGBA8888 
struct Pixel {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
};

struct Canvas {
    Pixel* pixels; // pixel buffer
    int width;
    int height;
    int pixelSize; // pixel size before being scaled, usually just use scale instead of this.
    int scale; // actual screen pixel scale / size

    int translateX;
    int translateY;
    float zoom; // zoom in factor

    int windowOffsetX; // offset from window top left
    int windowOffsetY;

    SDL_Texture* texture;
};

struct Pen {
    int size;
    Pixel pixel;
};

struct BasicButton {
    SDL_Rect rect;
    std::function<void(void)> onClick;
};

class GUI {
    public:
    std::vector<BasicButton> buttons;

    void draw(SDL_Renderer* ren) {
        for (size_t i = 0; i < buttons.size(); i++) {
            SDL_SetRenderDrawColor(ren, 85, 85, 85, 255);
            SDL_RenderFillRect(ren, &buttons[i].rect);

            // draw texture...

            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderDrawRect(ren, &buttons[i].rect);
        }
    }

    void handleEvent(SDL_Event* event, int mouseX, int mouseY) {
        if (!event) {
            return;
        }
        if (event->type == SDL_MOUSEBUTTONDOWN) {
            SDL_Point mousePoint = {mouseX, mouseY};
            for (size_t i = 0; i < buttons.size(); i++) {
                if (SDL_PointInRect(&mousePoint, &buttons[i].rect)) {
                    // button was clicked
                    buttons[i].onClick();
                    // only allow one button to be clicked per click, so break
                    break;
                } 
            }
        }
    }
};

struct MouseState {
    int mouseX;
    int mouseY;
    Uint32 mouseButtons;
};

struct Context {
    SDLContext *sdlCtx;
    Uint8 *keyboard;
    struct MetaData *metaData;
    struct Canvas *canvas;
    Pen *pen;
    GUI *gui;
    struct MouseState *mouseStateHistory;
    int* mouseStateHistoryQueueIndex;
};

Pixel* createPixelBuffer(int width, int height) {
    // initialize all pixels to all zeros, transparent black
    Pixel* pixels = (Pixel*)calloc(width*height, sizeof(Pixel));
    return pixels;
}

void renderPenSelectionButton(SDL_Renderer* ren, SDL_Rect* dst, SDL_Texture* texture) {
    SDL_SetRenderDrawColor(ren, 85, 85, 85, 255);
    SDL_RenderFillRect(ren, dst);

    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderDrawRect(ren, dst);
}

/*
* Draw on the canvas with the pen at the given coordinates.
* @return The number of pixels drawn to the canvas
*/
int penDrawOnCanvas(Canvas* canvas, Pen* pen, int canvasX, int canvasY, SDL_Renderer* ren) {
    SDL_SetRenderTarget(ren, canvas->texture);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

    // go through pixels selected by pen based on pen size
    int pixelsDrawn = 0;
    for (int pixelX = canvasX; pixelX < canvasX+pen->size; pixelX++) {
        for (int pixelY = canvasY; pixelY < canvasY+pen->size; pixelY++) {
            // we only check for the bottom and right edges as the pen size expansion
            // only goes down and right, never up or left
            if (pixelX < 0 || pixelX >= canvas->width || pixelY < 0 || pixelY >= canvas->height) {
                // selected area went over the border, dont draw
                continue;
            }
            Pixel* selectedPixel = &canvas->pixels[pixelX + pixelY * canvas->height];
            // In the future, this might contain special tool stuff, for now just copy
            *selectedPixel = pen->pixel;
            Pixel pixel = *selectedPixel;

            SDL_SetRenderDrawColor(ren, pixel.r, pixel.g, pixel.b, pixel.a);
            SDL_RenderDrawPoint(ren, pixelX, pixelY);
            
            pixelsDrawn++;;
        }
    }

    SDL_SetRenderTarget(ren, NULL);

    return pixelsDrawn;
}

void renderEntireCanvas(SDL_Renderer* ren, Canvas* canvas) {
    SDL_SetRenderTarget(ren, canvas->texture);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 0);
    SDL_RenderClear(ren);
    for (int x = 0; x < canvas->width; x++) {
        for (int y = 0; y < canvas->height; y++) {
            Pixel pixel = canvas->pixels[x + y*canvas->width];

            SDL_SetRenderDrawColor(ren, pixel.r, pixel.g, pixel.b, pixel.a);
            SDL_RenderDrawPoint(ren, x, y);
        }
    }
    SDL_SetRenderTarget(ren, NULL);
}

void render(SDL_Renderer* ren, float scale, const Canvas* canvas, Pen *pen, GUI *gui, MetaData* metadata) {
    // get window size in pixels
    int renWidth;
    int renHeight;
    SDL_GetRendererOutputSize(ren, &renWidth, &renHeight);

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderClear(ren);

    SDL_Rect canvasRect = {
        canvas->windowOffsetX,
        canvas->windowOffsetY,
        canvas->width * canvas->scale,
        canvas->height * canvas->scale
    };
    SDL_SetRenderDrawColor(ren, 80, 80, 80, 255);
    SDL_RenderFillRect(ren, &canvasRect);


    int visibleCanvasWidth = (float)canvas->width / canvas->zoom;
    int visibleCanvasHeight = (float)canvas->height / canvas->zoom;

    SDL_Rect transformedCanvasRect = {
        canvas->translateX,
        canvas->translateY,
        visibleCanvasWidth,
        visibleCanvasHeight
    };

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(ren, canvas->texture, &transformedCanvasRect, &canvasRect);

    SDL_SetRenderDrawColor(ren, 255, 0, 255, 255);
    SDL_RenderDrawRect(ren, &canvasRect);

    /* Draw GUI */
    // gui->draw(ren);

    FC_Draw(FreeSans, ren, 5*scale, 5*scale, "FPS: %.2f", metadata->fps);

    FC_DrawAlign(TitleFont, ren, renWidth/2, 17*scale, FC_HALIGN_CENTER, "Pixel Art Maker");

    FC_DrawAny(InfoFont, ren, 5, renHeight - 5, FC_HALIGN_LEFT, FC_VALIGN_BOTTOM, FC_MakeScale(1, 1), FC_MakeColor(0,0,0,255),
    "Left mouse to draw, right mouse to erase.\n"
    "Scroll with mouse to zoom in/out, arrow keys to move around when zoomed.");

    SDL_RenderPresent(ren);
}

inline Vec2 screenToCanvasCoords(Canvas* canvas, int screenX, int screenY) {
    return (Vec2){screenX - canvas->windowOffsetX, screenY - canvas->windowOffsetY}
        .divided(canvas->scale * canvas->zoom) + (((Vec2){canvas->translateX, canvas->translateY}));
}

inline void translateCanvas(Canvas* canvas, int translationX, int translationY) {
    canvas->translateX += translationX;
    canvas->translateY += translationY;

    int zoomIncreasedWidth = ((canvas->width * canvas->zoom) - canvas->width) / canvas->zoom;
    int zoomIncreasedHeight = ((canvas->height * canvas->zoom) - canvas->height) / canvas->zoom;

    int translateX = canvas->translateX;
    if (translateX < 0) {
        translateX = 0;
    }
    // make sure not to translate too far, so it wouldn't push the canvas off the screen
    else if (translateX > zoomIncreasedWidth) {
        translateX = zoomIncreasedWidth;
    }
    int translateY = canvas->translateY;
    if (translateY < 0) {
        translateY = 0;
    }
    else if (translateY > zoomIncreasedHeight) {
        translateY = zoomIncreasedHeight;
    }

    canvas->translateX = translateX;
    canvas->translateY = translateY;
}

void resizeCanvas(Canvas* canvas, int windowWidth, int windowHeight, float renderScale) {\
    /* Design input values */
    int minCanvasMarginLeft = 10;
    int minCanvasMarginRight = 10;
    int minCanvasMarginBottom = 20;
    int minCanvasMarginTop = 75;

    /* Calculated values */
    int availableWidth = windowWidth - (minCanvasMarginLeft + minCanvasMarginRight) * renderScale;
    int availableHeight = windowHeight - (minCanvasMarginTop + minCanvasMarginBottom) * renderScale;

    int pixelSize = std::min(availableWidth / canvas->width, availableHeight / canvas->height);
    if (pixelSize < 1) {
        // force canvas to atleast exist
        pixelSize = 1;
    }

    int usedWidth = (pixelSize * canvas->width);
    int usedHeight = (pixelSize * canvas->height);

    int extraWidth = windowWidth - usedWidth;
    int extraHeight = windowHeight - usedHeight;

    canvas->windowOffsetX = (extraWidth/2) * renderScale;
    canvas->windowOffsetY = minCanvasMarginTop * renderScale;

    canvas->pixelSize = pixelSize;
    canvas->scale = pixelSize * renderScale;
}

/*
* Reload the canvas with a new texture, pixel buffer, etc.
* Does NOT change things like the pixel scale, window offset, etc. So it's most likely necessary to call resizeCanvas after calling this.
*/
void reloadCanvas(Canvas* canvas, int width, int height, SDL_Renderer* renderer) {
    canvas->width = width;
    canvas->height = height;
    if (canvas->pixels) {
        free(canvas->pixels);
    }
    canvas->pixels = createPixelBuffer(width, height);
    if (canvas->texture) {
        SDL_DestroyTexture(canvas->texture);
    }
    canvas->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, width, height);
    SDL_SetTextureBlendMode(canvas->texture, SDL_BLENDMODE_BLEND);
}

int writePixelBufferToFile(Pixel *buffer, int width, int height, const char* file) {
    if (!buffer) return -1;
    if (!file) return -1;

    SDL_Surface* bufferSurface = SDL_CreateRGBSurfaceWithFormatFrom(buffer, width, height, 64, width * 4, SDL_PIXELFORMAT_RGBA8888);
    if (!bufferSurface) {
        SDL_Log("Error: Failed to create surface to save to file.");
        return -1;
    }
    ((Uint32*)bufferSurface->pixels)[0] = 0xFF0000FF;
    IMG_SavePNG(bufferSurface, "ayooooo.png");
    if (IMG_SavePNG(bufferSurface, file)) {
        SDL_Log("Error saving pixel buffer to file, IMG_Error: %s", IMG_GetError());
        SDL_FreeSurface(bufferSurface);
        return -1;
    }

    SDL_FreeSurface(bufferSurface);

    return 0;
}

#define SAVE_FILE "art.png"

int saveCanvas(Canvas* canvas) {
    return writePixelBufferToFile(canvas->pixels, canvas->width, canvas->height, SAVE_FILE);
}

int loadCanvasFromSave(Canvas* canvas, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(SAVE_FILE);
    if (!surface) {
        SDL_Log("Error: Failed to load saved canvas from file.");
        SDL_FreeSurface(surface);
        return -1;
    }



    reloadCanvas(canvas, surface->w, surface->h, renderer);
    int surfaceSize = surface->w * surface->h;

    memcpy(canvas->pixels, surface->pixels, surfaceSize * sizeof(Pixel));
    

    SDL_FreeSurface(surface);
    return 0;
}

bool windowFocused = false;

// Main game loop
int update(Context ctx) {
    updateMetaData(ctx.metaData);

    bool quit = false;

    // handle events //
    // get user input state for this update
    int mouseX,mouseY;
    Uint32 mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
    // scale relative to actual pixels
    mouseX *= ctx.sdlCtx->scale;
    mouseY *= ctx.sdlCtx->scale;

    Canvas* canvas = ctx.canvas;

    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        ctx.gui->handleEvent(&e, mouseX, mouseY);

        switch(e.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    SDL_Log("window resized.");
                    resizeCanvas(canvas, e.window.data1, e.window.data2, ctx.sdlCtx->scale);
                } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                    windowFocused = true;
                } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    windowFocused = false;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    SDL_Log("Mouse clicked at %d,%d", mouseX, mouseY);
                } else if (e.button.button == SDL_BUTTON_RIGHT) {

                }
                break;
            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                    case SDLK_LEFT:
                        translateCanvas(canvas, -5, 0);
                        SDL_Log("translation: %d,%d", ctx.canvas->translateX, ctx.canvas->translateY);
                        break;
                    case SDLK_RIGHT:
                        translateCanvas(canvas, 5, 0);
                        break;
                    case SDLK_DOWN:
                        translateCanvas(canvas, 0, 5);
                        break;
                    case SDLK_UP:
                        translateCanvas(canvas, 0, -5);
                        break;
                    /*
                    // currently ultra broken, makes zero sense whatsoever, so just gonna give up
                    case SDLK_m:
                        // save canvas
                        saveCanvas(canvas);
                        renderEntireCanvas(ctx.sdlCtx->ren, canvas);
                        *?
                        break;
                    case SDLK_n:
                        // load canvas
                        loadCanvasFromSave(canvas, ctx.sdlCtx->ren);
                        renderEntireCanvas(ctx.sdlCtx->ren, canvas);
                        break;
                    */
                }
                break;
            case SDL_MOUSEWHEEL:
                canvas->zoom += e.wheel.y / 10.0f;
                if (canvas->zoom < 1.0f) canvas->zoom = 1.0f;
                translateCanvas(canvas, 0, 0); // so that the translation is automatically adjusted to not go too far over
                // kinda trash but who cares
                break;
            default:
                break;
        }
    }

    // web browsers automatically unload unfocused tabs and
    // how emscripten treats window focus is if the canvas is selected,
    // and when you try to click back on to the canvas it won't trigger an onWindowFocus event,
    // so it wont be registered, so we just don't sleep like we normally would.
#ifndef __EMSCRIPTEN__
    if (!windowFocused) {
        SDL_Delay(10);
        return (int)quit;
    }
#endif

    Pen* pen = ctx.pen;
    if ((mouseButtons & SDL_BUTTON_LMASK) == SDL_BUTTON_LMASK || (mouseButtons & SDL_BUTTON_RMASK) == SDL_BUTTON_RMASK) {

        // save old pixel setting so we can set it back after
        Pixel savedPenPixel = pen->pixel;
        if ((mouseButtons & SDL_BUTTON_RMASK) == SDL_BUTTON_RMASK) {
            pen->pixel = (Pixel){0, 0, 0, 0};
        }

        // check if mouse is within bounds of canvas
        Vec2 mouseCanvasPos = screenToCanvasCoords(canvas, mouseX, mouseY);
        if (mouseCanvasPos.x >= 0 && mouseCanvasPos.x < canvas->width &&
            mouseCanvasPos.y >= 0 && mouseCanvasPos.y < canvas->height) {
            // then do drawing on mouse
            int penX = (int)floor(mouseCanvasPos.x);
            int penY = (int)floor(mouseCanvasPos.y);

            penDrawOnCanvas(canvas, pen, penX, penY, ctx.sdlCtx->ren);
        }

        // the mouse state for the last update
        // loop over with high iq techniques (size 5 array, just add 4 instead of subtracting 1)
        MouseState lastMouseState = ctx.mouseStateHistory[(*ctx.mouseStateHistoryQueueIndex+4) % 5];
        // connect lines between updates if both were drawing,
        // for smoother line drawing on lower fps

        if (lastMouseState.mouseButtons & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK)) {

            /* draw along old and new pen positions to smooth out quickly drawn lines */
            Vec2 lastMouseCanvasPos = screenToCanvasCoords(canvas, lastMouseState.mouseX, lastMouseState.mouseY);

            Vec2 mouseDelta = {mouseCanvasPos.x - lastMouseCanvasPos.x, mouseCanvasPos.y - lastMouseCanvasPos.y};
            
            float step;
            if (abs(mouseDelta.x) >= abs(mouseDelta.y)) {
                step = abs(mouseDelta.x);
            } else {
                step = abs(mouseDelta.y);
            }
            float dx = mouseDelta.x / step;
            float dy = mouseDelta.y / step;
            float x = lastMouseCanvasPos.x;
            float y = lastMouseCanvasPos.y;
            int i = 1;
            while (i <= step) {
                penDrawOnCanvas(canvas, pen, (int)floor(x), (int)floor(y), ctx.sdlCtx->ren);
                x += dx;
                y += dy;
                i++;
            }
        }

        // SDL_Log("saved pixel: %d,%d,%d", savedPenPixel.r, savedPenPixel.g, savedPenPixel.b);
        pen->pixel = savedPenPixel;
    }

    render(ctx.sdlCtx->ren, ctx.sdlCtx->scale, ctx.canvas, ctx.pen, ctx.gui, ctx.metaData);

    ctx.mouseStateHistory[*ctx.mouseStateHistoryQueueIndex] = {
        .mouseX = mouseX,
        .mouseY = mouseY,
        .mouseButtons = mouseButtons
    };
    (*ctx.mouseStateHistoryQueueIndex)++;
    *ctx.mouseStateHistoryQueueIndex %= 5;

    if (quit) {
        SDL_Log("Update loop quitting...");
        return 1;
    }
    return 0;
}

// update wrapper function to unwrap the void pointer main loop parameter into its properties
int updateWrapper(void *param) {
    struct Context *ctx = (struct Context*)param;
    return update(*ctx);
}

// load assets and stuff
int load(SDL_Renderer *ren, float renderScale = 1.0f) {
    tex = IMG_LoadTexture(ren, "assets/pixelart/WatermelonSlice.png");
    FreeSans = FC_CreateFont();
    FC_LoadFont(FreeSans, ren, "assets/fonts/FreeSans.ttf", 24*renderScale, FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);

    TitleFont = FC_CreateFont();
    FC_LoadFont(TitleFont, ren, "assets/fonts/Ubuntu-Regular.ttf", 36*renderScale, FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);
    
    InfoFont = FC_CreateFont();
    FC_LoadFont(InfoFont, ren, "assets/fonts/FreeSans.ttf", 12*renderScale, FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);

    return 0;
}

// unload assets and stuff
void unload() {
    SDL_DestroyTexture(tex);
    FC_FreeFont(FreeSans);
    FC_FreeFont(TitleFont);
}

int main() {
    SDLSettings settings = DefaultSDLSettings;
    settings.windowTitle = "Hello There!";
    settings.windowIconPath = "assets/windowIcon.png";
    settings.allowHighDPI = true;
    settings.vsync = true;
    SDLContext sdlCtx = initSDLAndContext(&settings); // SDL Context
    SDL_Log("Window scale: %f", sdlCtx.scale);
    int windowWidth,windowHeight;
    SDL_GetWindowSize(sdlCtx.win, &windowWidth, &windowHeight);
    struct MetaData metaData = initMetaData();
    Canvas canvas;
    canvas.pixelSize = 1;
    canvas.scale = canvas.pixelSize * sdlCtx.scale;
    canvas.width = 256;
    canvas.height = 256;
    canvas.pixels = createPixelBuffer(canvas.width, canvas.height);
    canvas.windowOffsetX = 0;
    canvas.windowOffsetY = 0;
    canvas.zoom = 1.0f;
    canvas.translateX = 0;
    canvas.translateY = 0;
    canvas.texture = SDL_CreateTexture(sdlCtx.ren, SDL_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, canvas.width, canvas.height);
    SDL_SetTextureBlendMode(canvas.texture, SDL_BLENDMODE_BLEND);
    resizeCanvas(&canvas, windowWidth, windowHeight, sdlCtx.scale);


    Pen pen;
    pen.pixel = (Pixel){0, 255, 255, 120};
    pen.size = 1;

    GUI gui;
    {
        int buttonsStartX = 50;
        int buttonsStartY = 50;
        int buttonWidth = 60;
        int buttonHeight = 60;
        for (int i = 0; i < 5; i++) {
            BasicButton button;
            button.rect = (SDL_Rect){
                buttonsStartX + i * buttonWidth,
                buttonsStartY,
                buttonWidth,
                buttonHeight
            };
            button.onClick = []() {
                printf("hello?\n");
            };
            gui.buttons.push_back(button);
        }
    }

    load(sdlCtx.ren, sdlCtx.scale);

    // do initial rendering
    renderEntireCanvas(sdlCtx.ren, &canvas);

    windowFocused = true;
    
    struct Context context;
    context.sdlCtx = &sdlCtx;
    context.metaData = &metaData;
    context.canvas = &canvas;
    context.pen = &pen;
    context.gui = &gui;
    context.mouseStateHistory = (MouseState*)malloc(5 * sizeof(MouseState));
    for (int i = 0; i < 5; i++) {
        context.mouseStateHistory[i].mouseButtons = 0;
        context.mouseStateHistory[i].mouseX = 0;
        context.mouseStateHistory[i].mouseY = 0;
    }
    int mouseStateHistoryQueueIndex = 0;
    context.mouseStateHistoryQueueIndex = &mouseStateHistoryQueueIndex;

    NC_SetMainLoop(updateWrapper, &context);

    free(canvas.pixels);
    SDL_DestroyTexture(canvas.texture);

    unload();

    quitSDLContext(sdlCtx);

    return 0;
}