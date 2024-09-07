#include <iostream>
#include "my_lib.h"
#include <SDL3/SDL.h>

int main(int argc, char* argv[])
{
	std::cout << "lib call: " << lib(100) << std::endl;

	/*uint64_t ch;
	std::cout << "Enter the number: ";
	std::cin >> ch;
	*/

	//std::cout << "Hello World:" << ch << std::endl;

	(void)argc;
	(void)argv;

	if (!SDL_Init(SDL_INIT_VIDEO)){
		SDL_Log("SDL Init failed (%s)", SDL_GetError());
		return 1;
	}

	SDL_Window *window = nullptr;
	SDL_Renderer *renderer = nullptr;

	if (!SDL_CreateWindowAndRenderer("SDL issue", 640, 480, 0, &window, &renderer)){
		SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	while (true){
		int finished = 0;
		SDL_Event event;
		while (SDL_PollEvent(&event)){
			if (event.type == SDL_EVENT_QUIT){
				finished = 1;
				break;
			}
		}
		if (finished){
			break;
		}

		SDL_SetRenderDrawColor(renderer, 80, 80, 80, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
	}
	
	return 0;
}
