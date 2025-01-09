#include "raylib.h"

#include "resource_dir.h"	

// TODO: 
// collision 
// SOMETHING TO COLLIDE WITH (GAMING) KiLLeR BuGS
// big tongue shooting out at bugs?
// scoreboard


// keep track of direction
typedef enum Direction {
	LEFT = -1,
	RIGHT = 1,
} Direction;

// struct for frog sprite
typedef struct Frog{
	Texture2D texture;		
	Rectangle destinationPosition;
	Vector2 velocity;
	Direction direction;
	//jump related variables
	bool isJumping; 		
	int frame;
	float jumpTimer; 
	float frameTimer; 		
} Frog;

// struct for killer bugs
typedef struct Bug{
	Texture2D texture;
	Rectangle destinationPosition;
	Vector2 velocity;
	Direction direction;
	// load progressively more bugs from off screen moving toward / circling around the frog getting ever nearer
	// collision with frog tongue = KILL
} Bug;

// gravity
void apply_gravity(Frog *froggy) {
	froggy->velocity.y += 36.0;

	if (froggy->velocity.y > 450.0) {
		froggy->velocity.y = 450.0;
	}
}

// movement	
void move_frog(Frog *froggy, int maxFrames) {

	const float frameDuration = 0.2f;  
	const float jumpDuration = 1.4f;   

	froggy->velocity.x = 0.0;

	// side movement
	if (IsKeyDown(KEY_D)) {
		froggy->velocity.x = 200.0;
		froggy->direction = RIGHT;
	}

	if (IsKeyDown(KEY_A)) {
		froggy->velocity.x = -200.0;
		froggy->direction = LEFT;
	}

	// jump (prevent double jumps)
	if (IsKeyPressed(KEY_SPACE) && !froggy->isJumping) {
		froggy->velocity.y = -990.0;

		if (froggy->direction == RIGHT) {
			froggy->velocity.x = 360;
		}

		if (froggy->direction == LEFT) {
			froggy->velocity.x = -360;
		}
		
		froggy->isJumping = true;
		froggy->jumpTimer = jumpDuration;

		// start at second/third frame for more believable jump animation
		froggy->frame = 2;          
		froggy->frameTimer = 0.0f;  
	}
	
	if (froggy->isJumping) {			
		froggy->frameTimer += GetFrameTime();
		
		if (froggy->frameTimer >= frameDuration) {
			froggy->frameTimer = 0.0f;
			froggy->frame++;
			// cycling frames, result being jump animation
			froggy->frame %= maxFrames; 
		}
		
		// countdown jumptimer to 0
		froggy->jumpTimer -= GetFrameTime();
		if (froggy->jumpTimer <= 0.0f) {
			froggy->jumpTimer = 0.0f; 

			// reset jump status
			froggy->isJumping = false;

			// resting frog texture
			froggy->frame = 0;       
		}
	}
}	

// velocity
void apply_velocity(Frog *froggy) {
	froggy->destinationPosition.x += froggy->velocity.x * GetFrameTime();
	froggy->destinationPosition.y += froggy->velocity.y * GetFrameTime();
}	

int main () {	
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	SetTargetFPS(60);

	// Create the window and OpenGL context
	InitWindow(1280, 800, "Frog");

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	Texture2D frog_texture = LoadTexture("frog.png");	

	Frog froggy = (Frog){.texture = frog_texture,
						.destinationPosition =
							(Rectangle){
								.x = 600.0,
								.y = 800.0,
								.width = 0.0,
								.height = 36.0,
							},						
						.direction = RIGHT,
						.isJumping = false,
						.frame = 0,
						.jumpTimer = 0.0f,   
    					.frameTimer = 0.0f};


 
	// debug logging
	SetTraceLogLevel(LOG_INFO);  

	// 8 pictures on the sprite sheet -> 
	const float frameWidth = (float)(frog_texture.width / 8);
	const int maxFrames = (int)frog_texture.width / (int)frameWidth;
	
	// game loop
	while (!WindowShouldClose()) // run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		// update 
		apply_gravity(&froggy);
		move_frog(&froggy, maxFrames);		
		apply_velocity(&froggy);

		  // if below ground, put back on ground
    	if (froggy.destinationPosition.y > GetScreenHeight() - froggy.destinationPosition.height) {
			froggy.destinationPosition.y = GetScreenHeight() - froggy.destinationPosition.height;
		}

		// drawing
		BeginDrawing();

		// debug 
		// DrawText(TextFormat("Frame: %d", froggy.frame), 10, 10, 20, WHITE);
		// DrawText(TextFormat("Is Jumping: %s", froggy.isJumping ? "true" : "false"), 10, 40, 20, WHITE);
		// DrawText(TextFormat("Frame Timer: %.2f", froggy.frameTimer), 10, 70, 20, WHITE);
		// DrawText(TextFormat("Jump Timer: %.2f", froggy.jumpTimer), 10, 100, 20, WHITE);

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);

		
		// float frogOffset = (froggy.direction == RIGHT) ? -frameWidth : 0; + frogOffset,
		Vector2 frogPosition = { (float)froggy.destinationPosition.x, (float)froggy.destinationPosition.y };
		
		// draw frog
		DrawTextureRec(
			froggy.texture, 
			(Rectangle){
				frameWidth * froggy.frame, 
				0, 
				(froggy.direction == RIGHT) ? -frameWidth : frameWidth,				
				(float)froggy.texture.height}, 
				frogPosition, 
			RAYWHITE);  

		// draw platforms		
		DrawCircle(250, 500, 300.0, RED);
		DrawLine(250, 500, 950, 500, RED);
		DrawRectangle(600, 600, 260, 36, RED);		

		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	// cleanup
	// unload our texture so it can be cleaned up
	UnloadTexture(frog_texture);

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}