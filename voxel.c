#include <stdlib.h>
#include "SDL.h"

int elevation_map[128][128];

void prepare_elevation_to_colour_map() {
	// Should this be done by means of a SDL palette or a separate C data
	// structure?
	// It should be SDL palette. It's the only sensible answer
};


int main(int argc, char **argv) {
	/*
	 * A window is just a window, with height, width, a title and a
	 * position on the screen (to be managed by the Window Manager).
	 */
	SDL_Window *window;

	/*
	 * A renderer is tied to one specific window (and conversely, a window
	 * should have one renderer attached to it) and makes the magic happen
	 * inside that window.
	 *
	 * A renderer has a target which is *always* a texture
	 *
	 * It is possible to create a renderer for a surface(???)
	 *
	 * It is possible to draw lines, rectangles and shit on the renderer
	 * directly
	 */
	SDL_Renderer *renderer;

	/*
	 * This is basically what other APIs call a Canvas. You've got a 2D
	 * array of pixels in a certain format. SDL_Surface is the old,
	 * painful, unaccelerated equivalent of SDL_Texture and is mostly useful
	 * for pixel-level stuff
	 */
	SDL_Surface *my_surface;

	/*
	 * A texture is created for a specific rendering context. It has a
	 * width, height and a specific pixel format.
	 *
	 * A texture can be copied onto the renderer's target (after it's been
	 * used for "offline" rendering).
	 *
	 * A texture can be created from a surface
	 *
	 * Textures can be locked and unlocked
	 */
	SDL_Texture *source_texture;

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		"Voxel",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		400,
		400,
	);
	SDL_ShowCursor(false);

	renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_ACCELERATED
	);

	bool running = true;
	unsigned int time = 0;
	while (running) {

		// At this stage, the rough idea is to have a surface(texture???)
		// prepared of the same size as the surface/texture that will be used
		// to render the elevation map from back to front. The source
		// surface/texture would have NO transparent pixels but instead every
		// pixel should be painted with the elevation map-> color map, so it'd
		// look like horizontal stripes.
		// We'd then copy vertical rectangles from that source surface to the
		// destination surface starting from the bottom and going as far up as
		// the section of the elevation map we're plotting

		SDL_LockSurface(palette_surface);
		prepare_palette(palette_surface->format->palette, ++time);
		draw_plasma_to_surface(palette_surface, p1, p2, p3, p4);
		SDL_UnlockSurface(palette_surface);

		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);

		source_texture = SDL_CreateTextureFromSurface(
			renderer,
			palette_surface
		);

		SDL_RenderCopy(renderer, source_texture, NULL, NULL);
		SDL_DestroyTexture(source_texture);

		// Causes the renderer to push whatever it's done since last time
		// to the window it's tied to
		SDL_RenderPresent(renderer);

		SDL_Event event;
		while (0 != SDL_PollEvent(&event)) {
			if (SDL_QUIT == event.type)
				running = false;
			if (SDL_KEYDOWN == event.type) {
				if (event.key.keysym.sym == SDLK_q)
					running = false;
			}
		}
	}

	SDL_FreeSurface(palette_surface);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
}






	return EXIT_SUCCESS;
}
