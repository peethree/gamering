#include "raylib.h"

#include "resource_dir.h"	

// TODO: 
// more BUGS 
// bug movement
// big tongue shooting out at bugs?
// scoreboard

// keep track of direction
typedef enum Direction {
	LEFT = -1,
	RIGHT = 1,
} Direction;

typedef enum Life{
	YES = 1,
	NO = -1,
} Life;

// struct for frog sprite
typedef struct Frog{
	Texture2D texture;		
	Rectangle destinationPosition;
	Vector2 velocity;
	Direction direction;
	Rectangle hitbox;
	float health;
	Life alive;
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
	Rectangle hitbox;
	int frame;
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

	const float frameDuration = 0.25f;  
	const float jumpDuration = 1.4f;   

	froggy->velocity.x = 0.0;

	// side movement
	if (IsKeyDown(KEY_D)) {
		if (froggy->isJumping) {
			// faster side movement in mid-air
			froggy->velocity.x = 380.0;			
		} else {
			froggy->velocity.x = 200.0;			
		}				
		froggy->direction = RIGHT;
	}

	if (IsKeyDown(KEY_A)) {
		if (froggy->isJumping) {
			froggy->velocity.x = -380.0;
		} else {
			froggy->velocity.x = -200.0;
		}				
		froggy->direction = LEFT;
	}

	// jump (prevent double jumps)
	if (IsKeyPressed(KEY_SPACE) && !froggy->isJumping) {
		froggy->velocity.y = -1100.0;		
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

// collision
void collision_check(Frog *froggy, Bug *mosquito) {

	// frog hitbox
	Rectangle frog_hitbox = (Rectangle){
      .x = froggy->destinationPosition.x + 10.0f,
      .y = froggy->destinationPosition.y + 1.0f,
      .width = 35.0f,
      .height = 35.0f	  
  	};

	// mosquito hitbox
	Rectangle bug_hitbox = (Rectangle){
      .x = mosquito->destinationPosition.x + 13.0f,
      .y = mosquito->destinationPosition.y + 1.0f,
      .width = 50.0f,
      .height = 50.0f	  
  	};

	// store the hitboxes
	froggy->hitbox = frog_hitbox;
	mosquito->hitbox = bug_hitbox;

	if (CheckCollisionRecs(froggy->hitbox, mosquito->hitbox)) {
		if (froggy->health >= 0.0) {
		froggy->health -= 1.8;	
		}

		// health cannot go below 0
		if (froggy->health <= 0) {
			froggy->health = 0;
			froggy->alive = NO;			
		}
	}
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
    					.frameTimer = 0.0f,
						.health = 100.0,
						.alive = YES};

	Texture2D mosquitoTexture = LoadTexture("bug.png");

	Bug mosquito = (Bug){.texture = mosquitoTexture,
						.destinationPosition = 
							(Rectangle){
								.x = 600.0,
								.y = 500.0,
								.width = 0.0,
								.height = 36.0,
							},	
						.direction = LEFT,
						.frame = 0};	
					
 
	// debug logging
	SetTraceLogLevel(LOG_INFO);  

	// 8 pictures on the frog sprite sheet -> 
	const float frameWidth = (float)(frog_texture.width / 8);
	const int maxFrames = (int)frog_texture.width / (int)frameWidth;

	// mosquito (only 2 for directions)
	const float frameWidthBug = (float)(mosquitoTexture.width / 2);
	
	
	// game loop
	while (!WindowShouldClose()) // run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		// update 
		apply_gravity(&froggy);
		move_frog(&froggy, maxFrames);		
		apply_velocity(&froggy);
		collision_check(&froggy, &mosquito);

		  // if below ground, put back on ground
    	if (froggy.destinationPosition.y > GetScreenHeight() - froggy.destinationPosition.height) {
			froggy.destinationPosition.y = GetScreenHeight() - froggy.destinationPosition.height;
		}

		// drawing
		BeginDrawing();

		// debug 
		DrawText(TextFormat("Frame: %d", froggy.frame), 10, 10, 20, WHITE);
		DrawText(TextFormat("Is Jumping: %s", froggy.isJumping ? "true" : "false"), 10, 40, 20, WHITE);
		DrawText(TextFormat("Frame Timer: %.2f", froggy.frameTimer), 10, 70, 20, WHITE);
		DrawText(TextFormat("Jump Timer: %.2f", froggy.jumpTimer), 10, 100, 20, WHITE);
		DrawText(TextFormat("Health: %.2f", froggy.health), 10, 130, 20, WHITE);
		// debugging: visual hitboxes
		DrawRectangleLinesEx(froggy.hitbox, 1, GREEN); 
		DrawRectangleLinesEx(mosquito.hitbox, 1, RED);   


		

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);
		
		Vector2 frogPosition = { 
			(float)froggy.destinationPosition.x, 
			(float)froggy.destinationPosition.y 
		};
		
		
		// draw frog
		DrawTextureRec(
			froggy.texture, 
			(Rectangle){
				frameWidth * froggy.frame, 
				0, 
				// flip the texture horizontally depending on direction it's facing
				(froggy.direction == RIGHT) ? -frameWidth : frameWidth,	
				// flip the texture vertically when the frog is dead			
				froggy.texture.height * froggy.alive}, 
			frogPosition, 
			// change color based on whether alive or not
			(froggy.alive == YES) ? RAYWHITE : RED);
			 

		Vector2 mosquitoPosition = { 
			(float)mosquito.destinationPosition.x, 
			(float)mosquito.destinationPosition.y 
		};

		// draw mosquito(s)
		DrawTextureRec(
			mosquito.texture,
			(Rectangle){
				frameWidthBug * mosquito.frame,
				0,
				(mosquito.direction == RIGHT) ? -frameWidthBug : frameWidthBug,
				mosquito.texture.height},
			mosquitoPosition,
			RAYWHITE);		
		

		// draw platforms		
		// DrawCircle(250, 500, 300.0, RED);
		// DrawLine(250, 500, 950, 500, RED);
		// DrawRectangle(600, 600, 260, 36, RED);		

		// end the frame and get ready for the next one (display frame, poll input, etc...)
		EndDrawing();
	}

	// cleanup
	// unload our texture so it can be cleaned up
	UnloadTexture(frog_texture);
	// UnloadTexture(mosquito.png);	

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}