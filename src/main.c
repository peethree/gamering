#include "raylib.h"
#include <math.h>

#include "resource_dir.h"	

#define MAX_BUGS 100
#define BUG_SPAWN_INTERVAL 0.5f

#define MAX_LILLYPADS 1000
#define OFFSCREEN_LILYPAD_SPAWN_AMOUNT 12

#define FROGGY_HEALTH 100.0
#define FROGGY_VELOCITY_X 200.0
#define FROGGY_JUMP_VELOCITY_Y 1100.0
#define FROGGY_JUMP_VELOCITY_X 380.0
#define FROGGY_FALL_VELOCITY 450.0
#define FROGGY_TONGUE_LENGTH 350.0f
#define FROGGY_ATTACK_DURATION 0.4f
#define FROGGY_TONGUE_WIDTH 5.0f

// TODO: 
// keep track of highscore 
// clean up bugs too far off screen!
// add some kind of menu when the game is over
// add more bug movement patterns
// add proximity based bug buzzing
// find a better way to deal with level building
// don't allow mid air jump
// fix successfully jumping on mosquitoes taking away health
// multiplayer?


// cool backgrounds, stages that change depending on y value
// stage 1: pond, stage 5: space, astronaut frog ???

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
	Rectangle tongue;
	Rectangle tongueHitbox;
	bool isAttacking;
    float tongueTimer;
    float attackDuration;
	float health;
	Status alive;	
	bool isJumping; 
	bool isBouncing;		
	int frame;
	float jumpTimer; 
	float frameTimer; 
	float highestPosition;
	int score;
} Frog;

// struct for killer bugs
typedef struct Bug{
	Texture2D texture;
	Rectangle position;
	Vector2 velocity;
	Direction direction;
	Rectangle hitbox;
	int frame;
	Status alive;	
	Vector2 targetPosition;
	Vector2 desiredVelocity;
	Rectangle previousPosition;
	bool isActive;
	// heatseeking movement
	float angle;
	float radius;
	float spiralSpeed;
	float convergence;	
	float minRadius; 
	// wave movement
	Vector2 spawnPosition;
	float waveFrequency;
	float waveAmplitude;			
} Bug;

// platforms to jump on for the frog
typedef struct Lilypad{
	Texture2D texture;
	Rectangle position;
	Rectangle hitbox;
	int frame;
	bool isActive;
} Lilypad;

// gravity frog
void apply_gravity(Frog *froggy) {
	froggy->velocity.y += 36.0;

	if (froggy->velocity.y > FROGGY_FALL_VELOCITY) {
		froggy->velocity.y = FROGGY_FALL_VELOCITY;
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
				froggy->velocity.x = FROGGY_JUMP_VELOCITY_X;			
			} else {
				froggy->velocity.x = FROGGY_JUMP_VELOCITY_X;			
			}				
			froggy->direction = RIGHT;
		}

		if (IsKeyDown(KEY_A)) {
			if (froggy->isJumping) {
				froggy->velocity.x = -FROGGY_JUMP_VELOCITY_X;
			} else {
				froggy->velocity.x = -FROGGY_JUMP_VELOCITY_X;
			}				
			froggy->direction = LEFT;
		}

		// jump (prevent double jumps) TODO: fix mid air jump
		if (IsKeyPressed(KEY_SPACE) && !froggy->isJumping) {
			froggy->velocity.y = -FROGGY_JUMP_VELOCITY_Y;		
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

void frog_attack(Frog *froggy) {

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !froggy->isAttacking) {
        froggy->isAttacking = true;
        froggy->tongueTimer = 0.0f;
        froggy->attackDuration = FROGGY_ATTACK_DURATION;
    }

    // attack animation
    if (froggy->isAttacking) {
        froggy->tongueTimer += GetFrameTime();
        float progress;

        // tongue extends
        if (froggy->tongueTimer <= froggy->attackDuration / 2) {
            progress = froggy->tongueTimer / (froggy->attackDuration / 2);
        }
        // tongue retracts
        else {
            progress = 1.0f - ((froggy->tongueTimer - froggy->attackDuration / 2) / (froggy->attackDuration / 2));
        }

        // tongue length calculation
        float currentLength = FROGGY_TONGUE_LENGTH * progress;

        // tongue position is based on froggy direction
		// facing right
        if (froggy->direction == RIGHT) {
            froggy->tongue = (Rectangle){
                froggy->position.x + 45.0f,  
                froggy->position.y + 10.0f,  
                currentLength,
                FROGGY_TONGUE_WIDTH
            };
		// facing left
        } else {
            froggy->tongue = (Rectangle){
                froggy->position.x - currentLength + 15.0f,  
                froggy->position.y + 10.0f,
                currentLength,
                FROGGY_TONGUE_WIDTH
            };
        }

        froggy->tongueHitbox = froggy->tongue;

        // reset hitbox when attack is over
        if (froggy->tongueTimer >= froggy->attackDuration) {
            froggy->isAttacking = false;
            froggy->tongue = (Rectangle){0, 0, 0, 0};
            froggy->tongueHitbox = (Rectangle){0, 0, 0, 0};
        }
    }
}

void draw_tongue(Frog *froggy) {
	if (froggy->isAttacking) {
        DrawRectangleRec(froggy->tongue, RED); 
		// DrawRectangleRec(froggy->tongueHitbox, WHITE);
	}
}

void wave_bug(Bug *mosquito) {	
	// mosquito->previousPosition.x = mosquito->position.x;
	if (mosquito->alive == ALIVE) {
		mosquito->waveAmplitude = 60.0f;
		mosquito->waveFrequency = 5.5f;
		mosquito->velocity.x = 360.0f;
		mosquito->angle += GetFrameTime();	

		// update position
		mosquito->position.x += (float)mosquito->direction * mosquito->velocity.x * GetFrameTime();
		mosquito->position.y = mosquito->spawnPosition.y + mosquito->waveAmplitude * sinf(mosquito->waveFrequency * mosquito->angle);
	} else {
		// bug should fall off the screen	
		mosquito->velocity.y = 300.0;
		mosquito->position.y += mosquito->velocity.y * GetFrameTime();	
	}
}

// movement bug
void move_bug(Bug *mosquito, Frog *froggy, float deltaTime) {	

	if (mosquito->alive == ALIVE) {
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

	// bug should fall off the screen
	if (mosquito->alive == DEAD) {
		mosquito->velocity.y = 300.0;
		mosquito->position.y += mosquito->velocity.y * GetFrameTime();
	}
}

// velocity
void apply_velocity(Frog *froggy, float deltaTime) {
	froggy->position.x += froggy->velocity.x * deltaTime;
	froggy->position.y += froggy->velocity.y * deltaTime;
}	

void deactivate_bug(Bug *mosquito, Frog *froggy) {	
	if (mosquito->alive == DEAD) {
		// deactivate bug at y distance difference			
		if (mosquito->isActive && (mosquito->position.y > froggy->highestPosition + 1000.0f)) {
			mosquito->isActive = false;
		}
	}
}

void collision_check_bugs(Frog *froggy, Bug *mosquito) {
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

	// when the tongue hits a bug, kill it, increment score
	if (mosquito->alive == ALIVE) {
		if (CheckCollisionRecs(froggy->tongueHitbox, mosquito->hitbox)) {
			mosquito->alive = DEAD;
			froggy->score++;
		}
	}
	
	// froggy mosquito collision
	if (CheckCollisionRecs(froggy->hitbox, mosquito->hitbox)) {
		// if the y value of the frog is bigger than the mosquito, deduce health
		if (froggy->position.y < mosquito->position.y) {		
			if (froggy->health >= 0.0) {
			froggy->health -= 1.8;	
			}	

			// otherwise allow the frog to bounce off mosquito for a boost and kill the bug						
			if (froggy->alive == ALIVE && mosquito->alive == ALIVE) {			
				froggy->velocity.y = -FROGGY_JUMP_VELOCITY_Y * 0.75;	
				mosquito->alive = DEAD;	
				froggy->score++;		
			}
		}		

		// frog dies when health goes to 0 . . . .
		if (froggy->health <= 0) {
			froggy->health = 0;
			froggy->alive = DEAD;	
			froggy->frame = 5;		
		}
	}
}


// collision
void collision_check_pads(Frog *froggy, Lilypad *pad) {	
	// frog hitbox
	froggy->hitbox = (Rectangle){
		.x = froggy->position.x + 10.0f,
		.y = froggy->position.y + 1.0f,
		.width = 35.0f,
		.height = 35.0f
	};

	// lilypad hitbox(es)
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

	if (pad->frame == 3) {
		pad->hitbox = (Rectangle){
			.x = pad->position.x + 0.0f,
			.y = pad->position.y + 40.0f,
			.width = 50.0f,
			.height = 1.0f
		};
	}

	// frog lilypad collision
	// allow the frog to jump through the lilypads, but catch it when it falls
	if (froggy->position.y > pad->position.y && pad->isActive) {
		if (CheckCollisionRecs(froggy->hitbox, pad->hitbox)) {	
			// smoother transition						
			froggy->position.y += (pad->position.y - froggy->position.y) * 0.9f;	
			
			// froggy is not moving up or down vertically (affected by gravity --> velocity.y 450.0)
			if (froggy->velocity.y == 450.0) {
				froggy->frame = 1;
				froggy->isJumping = false;	
			}
		}
	}
}

void spawn_bug(Bug *mosquito, Frog *froggy, Texture2D mosquito_texture) {	
	// initialize bug(s)
    mosquito->texture = mosquito_texture;

	// this i had for move_bug function

    // mosquito->position = (Rectangle){
	// 	froggy->position.x + (float)GetRandomValue(-250, 250), 
	// 	froggy->position.y + (float)GetRandomValue(200, 300), 
	// 	0.0f, 
	// 	36.0f
	// };

	mosquito->position = (Rectangle){
		froggy->position.x + (GetRandomValue(0, 1) == 0 ? -1000.0f : 1000.0f),
		froggy->position.y - GetRandomValue(-500, -400), 
		0.0f, 
		36.0f
	};	

	mosquito->spawnPosition = (Vector2){ mosquito->position.x, mosquito->position.y };
    // mosquito->direction = LEFT;
	if (mosquito->spawnPosition.x < froggy->position.x) {
		mosquito->direction = RIGHT;
	} else {
		mosquito->direction = LEFT;
	}
	// mosquito->direction = GetRandomValue(true, false) ? LEFT : RIGHT;
    mosquito->frame = 0;
    mosquito->angle = 0.0f;
    mosquito->radius = (float)GetRandomValue(500, 700);	
	// TODO: spiralspeed 0 should be prevented?
    mosquito->spiralSpeed = (float)GetRandomValue(-3,3);
    mosquito->convergence = (float)GetRandomValue(12,18);
    mosquito->minRadius = 3.0f;	
	mosquito->alive = ALIVE;
	mosquito->isActive = true;
}

void make_lilypads(Lilypad *pad, Texture2D lilypad_texture, Frog *froggy) {	
	// initialize the platforms	
	pad->texture = lilypad_texture;
	pad->position = (Rectangle){
		.x = froggy->position.x + GetRandomValue(-800, 600),
		.y = froggy->position.y + GetRandomValue(-1600, 300),
		.width = 0.0,
		.height = 36.0,
	};
	pad->frame = GetRandomValue(0,3);
	pad->isActive = true;	
}

void make_lilypads_offscreen(Lilypad *pad, Texture2D lilypad_texture, Frog *froggy) {
	pad->texture = lilypad_texture;
	pad->position = (Rectangle){
		.x = froggy->position.x + GetRandomValue(-600, 600),
		.y = froggy->position.y + GetRandomValue(-800, -1000),
		.width = 0.0,
		.height = 36.0,
	};	
	pad->frame = GetRandomValue(0,3);
	pad->isActive = true;
}

// set lilypad to inactive
void deactivate_lilypad(Lilypad *pad) {
    pad->isActive = false;    
}

void remove_lilypads_below(Lilypad *pad, Frog *froggy) {
	// get the highest y value visited
	if (froggy->position.y < froggy->highestPosition) {
        froggy->highestPosition = froggy->position.y;
    }

	// remove lilypads when froggy has climbed certain distance
    if (pad->isActive && pad->position.y > froggy->highestPosition + 1500.0f) {
        deactivate_lilypad(pad);
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
		.health = FROGGY_HEALTH,
		.alive = ALIVE,
		.highestPosition = 0.0f,
		.isBouncing = false
	};					

	// 8 pictures on the frog sprite sheet -> 
	const float frameWidth = (float)(frog_texture.width / 8);
	const int maxFrames = (int)frog_texture.width / (int)frameWidth;

	// initialize camera 
	Camera2D camera = {0};
	camera.target = (Vector2){0, froggy.position.y};  
	camera.offset = (Vector2){0, GetScreenHeight() / 2.0f};  
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

	// lilipad init
	Texture2D lilypad_texture = LoadTexture("lilipads.png");

	// keep track of the lilypads
	Lilypad pads[MAX_LILLYPADS];
	int activePads = 0;
	
	// 4 frames in lilypad sprite sheet
	const float frameWidthLilypad = (float)(lilypad_texture.width / 4);	
	
	float nextLilypadSpawn = 0.0f;

	

	// game loop
	while (!WindowShouldClose()) // run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		// update
		bugSpawnTimer += GetFrameTime();

		// spawn bugs 
		if (bugSpawnTimer >= BUG_SPAWN_INTERVAL && activeBugs < MAX_BUGS) {
			spawn_bug(&mosquitoes[activeBugs], &froggy, mosquito_texture);
			activeBugs++;
			bugSpawnTimer = 0.0f;	
		}
	
		// make initial pads to jump on		
		if (activePads < 40) {
			for (int i = 0; i < 40; i++) {
				make_lilypads(&pads[activePads], lilypad_texture, &froggy);
				activePads++;
			}
		}

		// keep spawning lilypads, this time offscreen
		if (froggy.position.y < nextLilypadSpawn) {
			for (int i = 0; i < OFFSCREEN_LILYPAD_SPAWN_AMOUNT && i < MAX_LILLYPADS; i++) {
				make_lilypads_offscreen(&pads[activeBugs], lilypad_texture, &froggy);
				activePads++;				  
			}
			nextLilypadSpawn -= 400.0f;
		}	
				
		apply_gravity(&froggy);
		move_frog(&froggy, maxFrames);		
		frog_attack(&froggy);	
		apply_velocity(&froggy, GetFrameTime());	


		int activePadsAfterLoop = 0;
		// check collision with pads, remove the ones too far below
		for (int i = 0; i < activePads; i++) {	
			collision_check_pads(&froggy, &pads[i]);			
			remove_lilypads_below(&pads[i], &froggy); 

			// adjust the amount of active pads after without messing up the loop index
			if (pads[i].isActive) {
        		pads[activePadsAfterLoop++] = pads[i];
    		}			
		}	
		activePads = activePadsAfterLoop;

		int activeBugsAfterLoop = 0;
		// update bug movement and collision with bugs and frog
		for (int i = 0; i < activeBugs; i++) {
			// if (!mosquitoes[i].isActive) continue;

			wave_bug(&mosquitoes[i]);
			// move_bug(&mosquitoes[i], &froggy, GetFrameTime());

			collision_check_bugs(&froggy, &mosquitoes[i]);
			deactivate_bug(&mosquitoes[i], &froggy);
			
			// remove inactive mosquitoes after the loop 
			if (mosquitoes[i].isActive) {
				mosquitoes[activeBugsAfterLoop++] = mosquitoes[i];
			}			
		}	
		activeBugs = activeBugsAfterLoop;		

		// if froggy below ground, put it back on ground
    	if (froggy.position.y > GetScreenHeight() - froggy.position.height) {
			froggy.position.y = GetScreenHeight() - froggy.position.height;
		}
	
		// drawing
		BeginDrawing();

		BeginMode2D(camera);		
		
		// follow the froggy's y position
		camera.target.y = froggy.position.y;				
		
		// * smoothening factor
		camera.target.y = camera.target.y + (froggy.position.y - camera.target.y) * 0.1f;

		// prevent camera from going below ground:
		float minY = GetScreenHeight()/2.0f;
		if (camera.target.y > minY) {
			camera.target.y = minY;
		}

		// debugging: visual hitboxes
		// DrawRectangleLinesEx(froggy.hitbox, 1, GREEN); 	
			
		for (int i = 0; i < activePads; i++) {		
			if (!pads[i].isActive) continue;
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
					frameWidthLilypad,
					pads[i].texture.height	
				},
				lilypadPosition,
				RAYWHITE
			);
			
			// lilypad hitbox visual
			// DrawRectangleLinesEx(pads[i].hitbox, 1, RED); 			
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
			 
		draw_tongue(&froggy);

		DrawText(TextFormat("pos.x: %.2f", froggy.position.x), 10, 310, 20, WHITE);

		// draw the bugs
		for (int i = 0; i < activeBugs; i++) {
			Vector2 mosquitoPosition = { 
				(float)mosquitoes[i].position.x, 
				(float)mosquitoes[i].position.y
			};									

			// annoying twitching animation by scaling it from 90 / 100% size every frame :thumbs_up:
			DrawTexturePro(
				mosquitoes[i].texture,
				(Rectangle){
					frameWidthBug * mosquitoes[i].frame,
					0,
					(mosquitoes[i].direction == RIGHT) ? -frameWidthBug : frameWidthBug,
					mosquitoes[i].texture.height * mosquitoes[i].alive},
				(Rectangle){
					mosquitoPosition.x,
					mosquitoPosition.y,
					frameWidthBug * GetRandomValue(9,10) / 10.0,					
					(float)mosquitoes[i].texture.height * GetRandomValue(9,10) / 10.0},
				(Vector2){0,0},
				0.0f,
				(mosquitoes[i].alive == ALIVE) ? RAYWHITE : RED);	

			// DrawRectangleLinesEx(mosquitoes[i].hitbox, 1, RED); 
			// DrawText(TextFormat("spawn Position y: %.2f", mosquitoes[i].spawnPosition.y), 10, 310 + 20 * i, 20, WHITE);	
			// DrawText(TextFormat("spawn Position x: %.2f", mosquitoes[i].spawnPosition.x), 10, 330 + 20 * i, 20, WHITE);				
		}

		EndMode2D();

		
		DrawText(TextFormat("Health: %.2f", froggy.health), 500, 0, 40, WHITE);
		DrawText(TextFormat("Score: %d", froggy.score), 500, 40, 40, WHITE);

		// debug 
		DrawText(TextFormat("Position: %.2f", froggy.position.y), 10, 10, 20, WHITE);
		DrawText(TextFormat("Velocity: %.2f", froggy.velocity.y), 10, 40, 20, WHITE);
		DrawText(TextFormat("lilypads: %d", activePads), 10, 70, 20, WHITE);
		DrawText(TextFormat("bugs: %d", activeBugs), 10, 100, 20, WHITE);
		DrawText(TextFormat("next_spawn: %.2f", nextLilypadSpawn), 10, 130, 20, WHITE);
		DrawText(TextFormat("acivebugs_after_loop: %d", activeBugsAfterLoop), 10, 160, 20, WHITE);
		DrawText(TextFormat("frog tongue height: %.2f", froggy.tongue.height), 10, 190, 20, WHITE);
		DrawText(TextFormat("frog tongue width: %.2f", froggy.tongue.width), 10, 220, 20, WHITE);
		DrawText(TextFormat("frog tongue x: %.2f", froggy.tongue.x), 10, 250, 20, WHITE);
		DrawText(TextFormat("frog tongue y: %.2f", froggy.tongue.y), 10, 280, 20, WHITE);
		// DrawText(TextFormat("current_height: %.2f", currentHeight), 10, 70, 20, WHITE);


		if (froggy.alive == DEAD) {
			DrawText("FROGGY HAS PERISHED", 400, 400, 42, RED);
		}

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