#include "raylib.h"
#include <math.h>

#include "resource_dir.h"	

// TODO: 
// platforms to move on
// vertical progress

// camera movement
// make the score / health etc dynamic (based upon frog location)

// big tongue shooting out at bugs?

// used to flip pictures
typedef enum Direction {
	LEFT = -1,
	RIGHT = 1,
} Direction;

typedef enum Status{
	ALIVE = 1,
	DEAD = -1,
} Status;

// struct for frog sprite
typedef struct Frog{
	Texture2D texture;		
	Rectangle position;
	Vector2 velocity;
	Direction direction;
	Rectangle hitbox;
	float health;
	Status alive;
	//jump related variables	
	bool isJumping; 		
	int frame;
	float jumpTimer; 
	float frameTimer; 
} Frog;

// max mosquitos
#define MAX_BUGS 100
#define MAX_LILLYPADS 1000
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800

// struct for killer bugs
typedef struct Bug{
	Texture2D texture;
	Rectangle position;
	Vector2 velocity;
	Direction direction;
	Rectangle hitbox;
	int frame;
	// bug movement
	Vector2 targetPosition;
	Vector2 desiredVelocity;
	Rectangle previousPosition;
	float angle;
	float radius;
	float spiralSpeed;
	float convergence;
	// minimum distance from player
	float minRadius; 		
	// collision with frog tongue = KILL
} Bug;

// platforms to jump on for the frog
typedef struct Lilypad{
	Texture2D texture;
	Rectangle position;
	Rectangle hitbox;
	int frame;
} Lilypad;

// gravity frog
void apply_gravity(Frog *froggy) {
	froggy->velocity.y += 36.0;

	if (froggy->velocity.y > 450.0) {
		froggy->velocity.y = 450.0;
	}
}

// movement	frog
void move_frog(Frog *froggy, int maxFrames) {

	const float frameDuration = 0.25f;  
	const float jumpDuration = 1.4f;   

	froggy->velocity.x = 0.0;

	if (froggy->alive == ALIVE) {
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
}	

// movement bug
void move_bug(Bug *mosquito, Frog *froggy, float deltaTime) {	
	// previous position for frame flipping
	mosquito->previousPosition.x = mosquito->position.x;

	// update angle
	mosquito->angle += mosquito->spiralSpeed * deltaTime;

	// decrease radius
	if (mosquito->radius > mosquito->minRadius) {
		mosquito->radius -= mosquito->convergence * deltaTime;
		// increase the convergence over time
		mosquito->convergence += 0.1;
	}

	// convert radius, angle -> x, y
	float mosquito_x = mosquito->radius * cosf(mosquito->angle); 
	float mosquito_y = mosquito->radius * sinf(mosquito->angle);	

	// set the target position to chase
	mosquito->targetPosition.x = froggy->position.x + mosquito_x; 
	mosquito->targetPosition.y = froggy->position.y + mosquito_y;

	// modify how fast the mosquito wants to reach the froggy
    mosquito->desiredVelocity.x = (mosquito->targetPosition.x - mosquito->position.x) * 4.6f,
	mosquito->desiredVelocity.y = (mosquito->targetPosition.y - mosquito->position.y) * 4.6f;

	// gradually increase velocity
	mosquito->velocity.x += (mosquito->desiredVelocity.x - mosquito->velocity.x) * 0.2f;
    mosquito->velocity.y += (mosquito->desiredVelocity.y - mosquito->velocity.y) * 0.2f;

	// apply velocity with delay modifier from previous block
	mosquito->position.x += mosquito->velocity.x * deltaTime;
	mosquito->position.y += mosquito->velocity.y * deltaTime;	
	
	// if horizontal movement is +x flip frame right, if -x flip frame pointing left
	float dif = mosquito->position.x - mosquito->previousPosition.x;

	if (dif > 0.0) {
		mosquito->frame = 1;
	} else {
		mosquito->frame = 0;
	}
}

// velocity
void apply_velocity(Frog *froggy, float deltaTime) {
	froggy->position.x += froggy->velocity.x * deltaTime;
	froggy->position.y += froggy->velocity.y * deltaTime;
}	

// collision
void collision_check(Frog *froggy, Bug *mosquito, Lilypad *pad) {
	
	// frog hitbox
	froggy->hitbox = (Rectangle){
		.x = froggy->position.x + 10.0f,
		.y = froggy->position.y + 1.0f,
		.width = 35.0f,
		.height = 35.0f
	};

	// bug hitbox
	mosquito->hitbox = (Rectangle){
		.x = mosquito->position.x + 13.0f,
		.y = mosquito->position.y + 1.0f,
		.width = 50.0f,
		.height = 50.0f
	};

	// lilypad hitbox
	if (pad->frame == 0) {
		pad->hitbox = (Rectangle){
			.x = pad->position.x + 5.0f,
			.y = pad->position.y + 20.0f,
			.width = 90.0f,
			.height = 1.0f
		};
	} 

	if (pad->frame == 1) {
		pad->hitbox = (Rectangle){
			.x = pad->position.x + 5.0f,
			.y = pad->position.y + 15.0f,
			.width = 85.0f,
			.height = 1.0f
		};
	} 

	if (pad->frame == 2) {
		pad->hitbox = (Rectangle){
			.x = pad->position.x + 5.0f,
			.y = pad->position.y + 10.0f,
			.width = 60.0f,
			.height = 1.0f
		};
	} 

	// if (pad->frame == 3) {
	// 	pad->hitbox = (Rectangle){
	// 		.x = pad->position.x + 0.0f,
	// 		.y = pad->position.y + 45.0f,
	// 		.width = 50.0f,
	// 		.height = 1.0f
	// 	};
	// }

	// frog lilypad collision
	// allow the frog to jump through the lilypads, but catch it when it falls
	if (froggy->position.y > pad->position.y) {
		if (CheckCollisionRecs(froggy->hitbox, pad->hitbox)) {			
			froggy->position.y = pad->position.y;			
		}
	}

	// froggy mosquito collision
	if (CheckCollisionRecs(froggy->hitbox, mosquito->hitbox)) {
		if (froggy->health >= 0.0) {
		froggy->health -= 1.8;	
		}

		// frog dies when health goes to 0 . . . .
		if (froggy->health <= 0) {
			froggy->health = 0;
			froggy->alive = DEAD;	
			froggy->frame = 5;		
		}
	}
}

void spawn_bug(Bug *mosquito, Frog *froggy, Texture2D mosquito_texture) {	
	// initialize bug(s)
    mosquito->texture = mosquito_texture;
    mosquito->position = (Rectangle){
		froggy->position.x + (float)GetRandomValue(-250, 250), 
		froggy->position.y + (float)GetRandomValue(200, 300), 0.0f, 36.0f
	};
    mosquito->direction = LEFT;
    mosquito->frame = 0;
    mosquito->angle = 0.0f;
    mosquito->radius = (float)GetRandomValue(500, 700);	
	// TODO: spiralspeed 0 should be prevented?
    mosquito->spiralSpeed = (float)GetRandomValue(-3,3);
    mosquito->convergence = (float)GetRandomValue(12,18);
    mosquito->minRadius = 3.0f;	
}

void move_camera(Frog *froggy) {
	if (froggy->position.y >= 400) {
		// move camera
	}

}

void make_lilypad(Lilypad *pad, Texture2D lilypad_texture) {	
	// initialize the platforms	
	pad->texture = lilypad_texture;
	pad->position = (Rectangle){
		.x = GetRandomValue(100, 700),
		.y = GetRandomValue(-1000, 650),
		.width = 0.0,
		.height = 36.0,
	};
	// TODO: the third frame's hitbox is fucked up, omit for now
	pad->frame = GetRandomValue(0,2);
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

	// initialize froggy
	Frog froggy = (Frog){
		.texture = frog_texture,
		.position = (Rectangle){
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
		.alive = ALIVE
	};					

	// 8 pictures on the frog sprite sheet -> 
	const float frameWidth = (float)(frog_texture.width / 8);
	const int maxFrames = (int)frog_texture.width / (int)frameWidth;

	Camera2D camera = {0};
	camera.target = (Vector2){froggy.position.x + froggy.position.width / 2, froggy.position.y};
	camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;

	Texture2D mosquito_texture = LoadTexture("bug.png");

	// mosquito (only 2 for directions)
	const float frameWidthBug = (float)(mosquito_texture.width / 2);

	// array of mosquitoes
	Bug mosquitoes[MAX_BUGS];
	int activeBugs = 0;

	// bug spawntimer
	float bugSpawnTimer = 0.0f;	
	const float bugSpawnInterval = 0.5f;

	// TODO: lilipad level generator
	// lilipad init
	Texture2D lilypad_texture = LoadTexture("lilipads.png");

	// keep track of the lilypads
	Lilypad pads[MAX_LILLYPADS];
	int activePads = 0;
	
	// 4 frames in lilypad sprite sheet
	const float frameWidthLilypad = (float)(lilypad_texture.width / 4);
	
	int height_counter = 0;
	
	// game loop
	while (!WindowShouldClose()) // run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		float deltaTime = GetFrameTime();
		
   		bugSpawnTimer += deltaTime;

		// spawn bugs every 0.5f
		if (bugSpawnTimer >= bugSpawnInterval && activeBugs < MAX_BUGS) {
			spawn_bug(&mosquitoes[activeBugs], &froggy, mosquito_texture);
			activeBugs++;

			make_lilypad(&pads[activePads], lilypad_texture);
			activePads++;

			bugSpawnTimer = 0.0f;	



		}

		// logic needed: every x amount of y-axis climbing, make more lilypads
	
		// update 		
		apply_gravity(&froggy);
		move_frog(&froggy, maxFrames);	
		apply_velocity(&froggy, deltaTime);		

		for (int i = 0; i < activeBugs; i++) {
			move_bug(&mosquitoes[i], &froggy, deltaTime);
			collision_check(&froggy, &mosquitoes[i], &pads[i]);	
		}			

		// if froggy below ground, put it back on ground
    	if (froggy.position.y > GetScreenHeight() - froggy.position.height) {
			froggy.position.y = GetScreenHeight() - froggy.position.height;
		}

	

	
		// drawing
		BeginDrawing();

		BeginMode2D(camera);

		// camera.target.x = froggy.position.x + froggy.position.width / 2;

		// 
		if (froggy.position.y < camera.target.y - SCREEN_HEIGHT / 2) {
			camera.target.y = froggy.position.y + SCREEN_HEIGHT / 2;
		}

		
		if (camera.target.y > SCREEN_HEIGHT) {
			camera.target.y = SCREEN_HEIGHT;
		}



		// debug 
		// DrawText(TextFormat("Frame: %d", froggy.frame), 10, 10, 20, WHITE);
		// DrawText(TextFormat("Is Jumping: %s", froggy.isJumping ? "true" : "false"), 10, 40, 20, WHITE);
		// DrawText(TextFormat("Frame Timer: %.2f", froggy.frameTimer), 10, 70, 20, WHITE);
		// DrawText(TextFormat("Jump Timer: %.2f", froggy.jumpTimer), 10, 100, 20, WHITE);
		DrawText(TextFormat("Health: %.2f", froggy.health), 500, 0, 40, WHITE);
		// DrawText(TextFormat("Status: %d", froggy.alive), 10, 160, 20, WHITE);

		if (froggy.alive == DEAD) {
			DrawText("FROGGY HAS PERISHED", 400, 400, 42, RED);
		}
		// debugging: visual hitboxes
		// DrawRectangleLinesEx(froggy.hitbox, 1, GREEN); 	
			
		for (int i = 0; i < activePads; i++) {
			// lilypad position helper function
			Vector2 lilypadPosition = {
				(float)pads[i].position.x,
				(float)pads[i].position.y
			};

			// draw lilypads
			DrawTextureRec(
				pads[i].texture,
				(Rectangle){
					frameWidthLilypad * pads[i].frame,
					0,
					frameWidthBug,
					pads[i].texture.height	
				},
				lilypadPosition,
				RAYWHITE
			);
			
			// lilypad hitbox visual
			DrawRectangleLinesEx(pads[i].hitbox, 1, RED); 
		}

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);
		
		Vector2 frogPosition = { 
			(float)froggy.position.x, 
			(float)froggy.position.y 
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
			(froggy.alive == ALIVE) ? RAYWHITE : RED);
			 
		// draw the bugs
		for (int i = 0; i < activeBugs; i++) {

			Vector2 mosquitoPosition = { 
				(float)mosquitoes[i].position.x, 
				(float)mosquitoes[i].position.y
			};						

			// DrawTextureRec(
			// 	mosquitoes[i].texture,
			// 	(Rectangle){
			// 		frameWidthBug * mosquitoes[i].frame,
			// 		0,
			// 		(mosquitoes[i].direction == RIGHT) ? -frameWidthBug : frameWidthBug,
			// 		mosquitoes[i].texture.height},
			// 	mosquitoPosition,
			// 	RAYWHITE);				

			// annoying twitching animation by scaling it from 90 / 100% size every frame :thumbs_up:
			DrawTexturePro(
				mosquitoes[i].texture,
				(Rectangle){
					frameWidthBug * mosquitoes[i].frame,
					0,
					(mosquitoes[i].direction == RIGHT) ? -frameWidthBug : frameWidthBug,
					mosquitoes[i].texture.height},
				(Rectangle){
					mosquitoPosition.x,
					mosquitoPosition.y,
					frameWidthBug * (float)(GetRandomValue(9, 10) / 10.0),
					mosquitoes[i].texture.height * (float)(GetRandomValue(9, 10) / 10.0)},
				(Vector2){0,0},
				0.0f,
				RAYWHITE);	

			DrawRectangleLinesEx(mosquitoes[i].hitbox, 1, RED); 					
		}

		// EndMode2D();

		// end the frame and get ready for the next one (display frame, poll input, etc...)
		EndDrawing();		
	}

	// cleanup
	// unload our textures so it can be cleaned up
	UnloadTexture(frog_texture);
	UnloadTexture(mosquito_texture);	
	UnloadTexture(lilypad_texture);

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}