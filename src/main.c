/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include "raylib.h"

#include "resource_dir.h"	// utility header for SearchAndSetResourceDir

int main ()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	SetTargetFPS(60);

	// Create the window and OpenGL context
	InitWindow(1280, 800, "Hello Raylib");

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	// Load a texture from the resources directory
	Texture frog = LoadTexture("frog.png");

	Vector2 frogPosition = { (float)GetScreenWidth()/2, (float)GetScreenHeight()/2 };

	// frame width of individual textures in the frog sprite sheet
	float frameWidth = (float)(frog.width / 8)

	// game loop
	while (!WindowShouldClose())		// run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		// drawing
		BeginDrawing();

		if (IsKeyDown(KEY_RIGHT)) frogPosition.x += 2.0f;
        if (IsKeyDown(KEY_LEFT)) frogPosition.x -= 2.0f;
        if (IsKeyDown(KEY_UP)) frogPosition.y -= 2.0f;
        if (IsKeyDown(KEY_DOWN)) frogPosition.y += 2.0f;

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);

		// draw some text using the default font
		DrawText("Hello Baby!!!!", 200,200,20,WHITE);

		// draw our texture to the screen
		DrawTextureRec(
			frog, 
			Rectangle{frogPosition.x, frogPosition.y, frameWidth, (float)frog.height},
			frogPosition,				
			RAYWHITE
		);      
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	// cleanup
	// unload our texture so it can be cleaned up
	UnloadTexture(frog);

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
