#include "raylib.h"

#include "resource_dir.h"	

int main ()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	SetTargetFPS(60);

	// Create the window and OpenGL context
	InitWindow(1280, 800, "Frog");

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	// Load a texture from the resources directory
	// TODO: move into struct
	// Texture2D frog = LoadTexture("frog.png");

	// keep track of direction
	typedef enum Direction {
		LEFT = -1,
		RIGHT = 1,
	};


	// struct for frog sprite
	typedef struct Frog{
		Texture2D texture;
		// Rectangle currentPosition;
		Rectangle destinationPostion;
		Vector2 velocity;
		Direction direction;
	} Frog;


	// gravity
	void apply_gravity(Frog *frog) {
		frog->velocity.y += 25.0;
		if (sprite->vel.y > 450.0) {
			sprite->vel.y = 450.0;
		}
	}

	// movement
	void move_player(Frog *player) {
	// gradual -> multiply velocity with
	// float under 1.0f -> += -=
	

		// default: no .x movement
		player->velocity.x = 0.0;

		// left
		if (IsKeyDown(KEY_D)) {
			player->velocity.x = 120.0;			
			player->direction = RIGHT;
		}

		// right
		if (IsKeyDown(KEY_A)) {
			player->velocity.x = -120.0;
			player->direction = LEFT;
		}
	
		// jump
		if (IsKeyPressed(KEY_SPACE)) {
			player->velocity.y = -360.0;
		}
	}


	void apply_velocity_x(Frog *frog) {
		frog->destinationPosition.x += frog->velocity.x * GetFrameTime();
	}

	void apply_velocity_y(Frog *frog) {
		frog->destinationPosition.y += frog->velocity.y * GetFrameTime();
	}

	// TODO: update 
	// Vector2 frogPosition = { (float)GetScreenWidth()/2, (float)GetScreenHeight()/2 };


	// frame width of individual textures in the frog sprite sheet
	float frameWidth = (float)(frog.width / 8);

	int maxFrames = (int)frog.width / (int)frameWidth;
	float timer = 0.0f;
	int frame = 0;

	
	// jump animation variables
	bool isJumping = false;
	float jumpTimer = 0.0f;
	float jumpDuration = 1.9f; 

	// sprite direction variables
	// bool isFacingRight = false;

	// add gravity and velocity
	// make a struct for the frog that stores texture, position, velocity etc

	// game loop
	while (!WindowShouldClose()) // run the loop untill the user presses ESCAPE or presses the Close button on the window
	{

		// update 
		apply_gravity(&player)
		move_frog(&player)

		apply_velocity_x(&player)
		apply_velocity_y(&player)

		  // if below ground, put back on ground
    	if (player.destinationPosition.y > GetScreenHeight() - player.destinationPosition.height) {
			player.destinationPosition.y = GetScreenHeight() - player.destinationPosition.height;
		}

		// drawing
		BeginDrawing();

		
		
		// side movement
		// if (IsKeyDown(KEY_RIGHT)) {
		// 	frogPosition.x += 2.0f;
		// 	isFacingRight = true;
		// }
	    //     if (IsKeyDown(KEY_LEFT)) {
		// 	frogPosition.x -= 2.0f;
		// 	isFacingRight = false;
		// }

		// jump
		// if (IsKeyPressed(KEY_SPACE)) {
		// 	isJumping = true;

		// 	if (isFacingRight) {
		// 		// TODO: gradually update these values
		// 		frogPosition.x += 100.0f;
		// 		frogPosition.y -= 100.0f;
		// 	}
			
		// 	if (isFacingRight == false) {
		// 		frogPosition.x -= 100.0f;
		// 		frogPosition.y -= 100.0f;
		// 	}

		// 	jumpTimer = jumpDuration;
		// }

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);

		// sprite animation timer
		timer += GetFrameTime();

		// jump animation frame variable for sprite selection
		if (timer > 0.2f) {
			timer = 0.0;
			if (isJumping) {
				frame++;
				frame = frame % maxFrames;
			}
		}

		// jump duration
		if (isJumping) {
			jumpTimer -= GetFrameTime();
			if (jumpTimer < 0.0f) {
				isJumping = false;
				frame = 0;
			}
		}
		
		// draw frog
		DrawTextureRec(
			frog, 
			(Rectangle){
				frameWidth * frame, 
				0, 
				isFacingRight ? -frameWidth : frameWidth, 
				(float)frog.height}, 
			frogPosition, 
			RAYWHITE);  

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