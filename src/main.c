#include "raylib.h"
#include "constants.h"
#include <math.h>
#include "resource_dir.h"	
#include <stdio.h> 
#include <stdlib.h> 

// TODO: 
// consider putting big jump on a timer now that there's a small jump
// slowdown tongue attack ?
// make a cool wasp Sprite
// keep track of highscore 
// add some kind of menu when the game is over
// add more bug movement patterns
// add proximity based bug buzzing
// find a better way to deal with level building
// don't allow mid air jump
// tongue hitbox very inaccurate when froggy y pos is higher (smaller) than bug y pos

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
	float tongueAngle;
	float health;
	Status status;	
	bool isJumping; 
	bool isBouncing;	
	int frame;
	float jumpTimer; 
	float frameTimer; 
	float highestPosition;
	float size;
	int score;
} Frog;

typedef struct Bug{	
	Texture2D texture;
	Rectangle position;
	Vector2 velocity;
	Direction direction;
	Rectangle hitbox;
	int frame;
	Status status;	
	Vector2 targetPosition;
	Vector2 desiredVelocity;
	Rectangle previousPosition;
	bool isActive;	
	bool isEaten;	
	float angle;
	float radius;
	float spiralSpeed;
	float convergence;	
	float minRadius; 
	Vector2 spawnPosition;
	float waveFrequency;
	float waveAmplitude;
	char* type;			
} Bug;

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

void jump_animation(Frog *froggy, int maxFrames, float deltaTime, float frameDuration) {
	if (froggy->isJumping) {			
		froggy->frameTimer += deltaTime;
		
		if (froggy->frameTimer >= frameDuration) {
			froggy->frameTimer = 0.0f;
			froggy->frame++;
			// cycling frames, result being jump animation
			froggy->frame %= maxFrames; 
		}	
	}

	// countdown jumptimer to 0
	froggy->jumpTimer -= deltaTime;

	if (froggy->jumpTimer <= 0.0f) {
		froggy->jumpTimer = 0.0f; 

		// reset jump status
		froggy->isJumping = false;

		// resting frog texture
		froggy->frame = 0;       
	}
}

// smaller jump
void frog_baby_jump(Frog *froggy, int maxFrames, float deltaTime) {

	// TODO: don't quite like the look of this animation
	const float frameDuration = 0.10f;  
	const float jumpDuration = 1.1f; 

	// froggy->velocity.x = 0.0;
	if (froggy->status == ALIVE) {
		if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_SPACE) && !froggy->isJumping) {
			froggy->velocity.y = -FROGGY_JUMP_VELOCITY_Y * 0.7;		
			froggy->isJumping = true;
			froggy->jumpTimer = jumpDuration;			

			// start at second/third frame for more believable jump animation
			froggy->frame = 2;          
			froggy->frameTimer = 0.0f;  
			jump_animation(froggy, maxFrames, deltaTime, frameDuration);
		}	
	}			
}

void frog_big_jump(Frog *froggy, int maxFrames, float deltaTime) {
	const float frameDuration = 0.25f;  
	const float jumpDuration = 1.4f;  

	if (froggy->status == ALIVE) {
		// jump (prevent double jumps) TODO: fix mid air jump
		if (IsKeyPressed(KEY_SPACE) && !froggy->isJumping && !IsKeyPressed(KEY_LEFT_SHIFT)) {
			froggy->velocity.y = -FROGGY_JUMP_VELOCITY_Y;		
			froggy->isJumping = true;
			froggy->jumpTimer = jumpDuration;			

			// start at second/third frame for more believable jump animation
			froggy->frame = 2;          
			froggy->frameTimer = 0.0f;  
			
		}	
	}

	jump_animation(froggy, maxFrames, deltaTime, frameDuration);		
}



// movement	frog
void move_frog(Frog *froggy) { 

	froggy->velocity.x = 0.0;

	if (froggy->status == ALIVE) {
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
	}
}	

void screen_flip(Frog *froggy) {
	// allow movement from left side of the screen to right side 
	// 8 pixel grids / frames in the texture image -> divide by 8
	if (froggy->position.x < 0.0f - froggy->texture.width / 8) {
		froggy->position.x = (float)GetScreenWidth();
	} else if (froggy->position.x > (float)GetScreenWidth() + froggy->texture.width / 8) {
		froggy->position.x = 0.0f;
	}
}
 
void frog_attack(Frog *froggy, float deltaTime, Camera2D camera) {	
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !froggy->isAttacking) {
        froggy->isAttacking = true;
        froggy->tongueTimer = 0.0f;        
    }

    // attack animation
    if (froggy->isAttacking) {
        froggy->tongueTimer += deltaTime;
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

		// keep track of cursor coordinates, translate them into camera coords for hitbox logic
        Vector2 cursorPosition = GetMousePosition();
		Vector2 cameraMousePosition = GetScreenToWorld2D(cursorPosition, camera);

		// keep track of where on the sprite the tongue should appear from
		Vector2 frogMouthPosition;
		if (froggy->direction == RIGHT) {				//offsets to spawn tongue closer to mouth
			frogMouthPosition.x = froggy->position.x + (7.0f * froggy->size * 1.9);
			frogMouthPosition.y = froggy->position.y - (4.0f * froggy->size * 1.9);
		} else {
			frogMouthPosition.x = froggy->position.x - (7.0f * froggy->size * 1.9);
			frogMouthPosition.y = froggy->position.y - (4.0f * froggy->size * 1.9);				
		}

        // direction vectors
        float dx = cameraMousePosition.x - frogMouthPosition.x;
        float dy = cameraMousePosition.y - frogMouthPosition.y;
        
        // angle
        float angle = atan2f(dy, dx);
        
        // tongue endpoint
        float tongueEndX = frogMouthPosition.x + cosf(angle) * currentLength;
        float tongueEndY = frogMouthPosition.y + sinf(angle) * currentLength;
        
        // turn frog in direction of the mouseclick 
        froggy->direction = (cameraMousePosition.x > frogMouthPosition.x) ? RIGHT : LEFT;
        
		// tongue rectangle
        froggy->tongue = (Rectangle){
            frogMouthPosition.x,
            frogMouthPosition.y,
            currentLength,
            FROGGY_TONGUE_WIDTH
        };
        
        // angle needed for drawing
        froggy->tongueAngle = angle * RAD2DEG;          
		
        float tongueWidth = FROGGY_TONGUE_WIDTH * froggy->size;
		float halfWidth = tongueWidth / 2.0f;
        
        // hitbox corners
        Vector2 topLeft = {
            frogMouthPosition.x - halfWidth * sinf(angle),
            frogMouthPosition.y + halfWidth * cosf(angle)
        };        
        Vector2 bottomRight = {
            tongueEndX + halfWidth * sinf(angle),
            tongueEndY - halfWidth * cosf(angle)
        };
        
		// taper hitbox near the end of its length
		// 
		float currentHitboxWidth = tongueWidth * (1.0f - (currentLength / (FROGGY_TONGUE_LENGTH)) * 0.5f); 
        // ceate hitbox as a rectangle that encompasses the rotated tongue
        froggy->tongueHitbox = (Rectangle){
            fminf(topLeft.x, bottomRight.x),
            fminf(topLeft.y, bottomRight.y),
            fabsf(tongueEndX - frogMouthPosition.x) + halfWidth,
            currentHitboxWidth
        };

        // reset hitbox when attack is over.
		// was killing bugs at 0,0 coords before when initialized at those coords 
		// might not be necessary ?
        if (froggy->tongueTimer >= froggy->attackDuration) {
            froggy->isAttacking = false;
            froggy->tongue = (Rectangle){ 800, 1280, 0, 0 };
            froggy->tongueHitbox = (Rectangle){ 800, 1280, 0, 0 };
            froggy->tongueAngle = 0.0f;
        }
    }
}

void draw_tongue(Frog *froggy) {
	if (froggy->isAttacking) {

        DrawRectanglePro(
			froggy->tongue,
			(Vector2){0, FROGGY_TONGUE_WIDTH / 2},		
			froggy->tongueAngle,
			RED
		);
		
		//    DrawRectanglePro(
		// 	froggy->tongueHitbox,
		// 	(Vector2){ 0, FROGGY_TONGUE_WIDTH },
		// 	froggy->tongueAngle,
		// 	WHITE
		// );
	}
}

// movement mosquito
void move_mosquito(Bug *mosquito, float deltaTime) {	
	
	if (mosquito->status == ALIVE) {
		mosquito->waveAmplitude = MOSQUITO_WAVE_AMPLITUDE; 
		mosquito->waveFrequency = MOSQUITO_WAVE_FREQUENCY; 
		mosquito->velocity.x = MOSQUITO_VELOCITY_X; 
		mosquito->angle += deltaTime;	

		// update position
		mosquito->position.x += (float)mosquito->direction * mosquito->velocity.x * deltaTime;
		mosquito->position.y = mosquito->spawnPosition.y + mosquito->waveAmplitude * sinf(mosquito->waveFrequency * mosquito->angle);
	// don't update position anymore in this func when bug is eaten
	} else if (mosquito->isEaten) {
		//
	// bug should fall off the screen in case of jump death
	} else {			
		mosquito->velocity.y = MOSQUITO_VELOCITY_DEATH_FALL; 
		mosquito->position.y += mosquito->velocity.y * deltaTime;	
	}
}

// TODO: maybe invert the spiral so the wasp doesn't auto kill itself
// movement wasp
void move_wasp(Bug *wasp, Frog *froggy, float deltaTime) {	

	if (wasp->status == ALIVE) {
		// previous position for frame flipping
		wasp->previousPosition.x = wasp->position.x;

		// update angle
		wasp->angle += wasp->spiralSpeed * deltaTime;

		// decrease radius from froggy
		if (wasp->radius > wasp->minRadius) {
			wasp->radius -= wasp->convergence * deltaTime;
			// increase the convergence over time
			wasp->convergence += 0.1;
		}

		// convert radius and angle into x, y coordinates
		float wasp_x = wasp->radius * cosf(wasp->angle); 
		float wasp_y = wasp->radius * sinf(wasp->angle);	

		// set the target position to chase
		wasp->targetPosition.x = froggy->position.x + wasp_x; 
		wasp->targetPosition.y = froggy->position.y + wasp_y;

		// modify how fast the wasp wants to reach the froggy
		wasp->desiredVelocity.x = (wasp->targetPosition.x - wasp->position.x) * 4.6f,
		wasp->desiredVelocity.y = (wasp->targetPosition.y - wasp->position.y) * 4.6f;

		// gradually increase velocity
		wasp->velocity.x += (wasp->desiredVelocity.x - wasp->velocity.x) * 0.2f;
		wasp->velocity.y += (wasp->desiredVelocity.y - wasp->velocity.y) * 0.2f;

		// apply velocity with delay modifier from previous block
		wasp->position.x += wasp->velocity.x * deltaTime;
		wasp->position.y += wasp->velocity.y * deltaTime;	
		
		// if horizontal movement is +x flip frame right, if -x flip frame pointing left
		float dif = wasp->position.x - wasp->previousPosition.x;

		// TODO: implemetn wasp sprite with at least 2 frames
		if (dif > 0.0) {
			wasp->frame = 1;
		} else {
			wasp->frame = 0;
		}

	} else if (wasp->isEaten) {
		//
	// bug should fall off the screen in case of jump death
	} else {			
		wasp->velocity.y = WASP_VELOCITY_DEATH_FALL; 
		wasp->position.y += wasp->velocity.y * deltaTime;	
	}	
}

// velocity frog
void apply_velocity(Frog *froggy, float deltaTime) {
	froggy->position.x += froggy->velocity.x * deltaTime;
	froggy->position.y += froggy->velocity.y * deltaTime;
}	

// clean up bugs too far away from the froggy 
void deactivate_bug(Bug *bug, Frog *froggy) {	
	// deactivate bug at y distance difference			
	if (bug->isActive && (bug->position.y > froggy->position.y + 1000.0f)) {
		bug->isActive = false;
	}

	// and x difference (these numbers need to be further than where the bugs spawn)
	if (bug->isActive && ((bug->direction == RIGHT && bug->position.x > 1500.0f) || (bug->direction == LEFT && bug->position.x < -900.0f))) {
		bug->isActive = false;
	}	
}

// helper function for bug death animation after getting jumped on
// void bug_jump_death(Bug *bug, float deltaTime) {
// 	if (bug->status == DEAD) {
// 		if (bug->type == "wasp") {
// 			bug->velocity.y = WASP_VELOCITY_DEATH_FALL; 
// 		} else if (bug->type == "mosquito") {
// 			bug->velocity.y = MOSQUITO_VELOCITY_DEATH_FALL; 
// 		}		
// 		bug->position.y += bug->velocity.y * deltaTime;
// 	}
// }

// you will eat the bugs
void eat_bug(Frog *froggy, Bug *bug, float deltaTime) {
    // start pulling the bug toward the froggy when the tongue starts retracting
    if (bug->isEaten && froggy->tongueTimer >= 0.5 * FROGGY_TONGUE_TIMER) {      
		
        // direction vector        
        float dx = froggy->position.x - bug->position.x;
        float dy = froggy->position.y - bug->position.y;         	

        // movement amount this frame
        float moveAmount = FROGGY_TONGUE_BUG_PULL_SPEED * deltaTime;
        float distance = sqrtf(dx * dx + dy * dy);

		// move bug toward froggy
        if (distance > 5.0) {
            // normalize direction, move
            bug->position.x += (dx / distance) * moveAmount;
            bug->position.y += (dy / distance) * moveAmount;
        } else {
			bug->position = froggy->position;
		}	

		if (CheckCollisionRecs(froggy->hitbox, bug->hitbox)) {
			froggy->size += 0.25;
			bug->isActive = false;
		}	
    }
}

void collision_check_bugs(Frog *froggy, Bug *bug, float deltaTime) {
	// hitboxes need to be updated every loop	
	// frog hitbox
	froggy->hitbox = (Rectangle){
		.x = froggy->position.x + 10.0f,
		.y = froggy->position.y + 1.0f,
		.width = 35.0f,
		.height = 35.0f
	};

	// mosquito hitbox
	if (bug->type == "mosquito") {	
		bug->hitbox = (Rectangle){
			.x = bug->position.x + 13.0f,
			.y = bug->position.y + 1.0f,
			.width = 50.0f,
			.height = 50.0f
		};
	}

	// wasp hitbox
	if (bug->type == "wasp") {		
		bug->hitbox = (Rectangle) {
			.x = bug->position.x + 40.0f,
			.y = bug->position.y + 1.0f,
			.width = 100.0f,
			.height = 100.0f
		};
		}

	// when the tongue hits a bug, kill it, increment score
	if (CheckCollisionRecs(froggy->tongueHitbox, bug->hitbox)) {
		bug->status = DEAD;
		bug->isEaten = true;		
		froggy->score++;
	}	
	
	// froggy bug collision
	if (CheckCollisionRecs(froggy->hitbox, bug->hitbox) && bug->status == ALIVE && froggy->status == ALIVE) {
		// froggy is hitting the bug from the bottom
		if (froggy->position.y > bug->position.y) {		
			if (froggy->health >= 0.0) {
				if (bug->type == "mosquito") {
					froggy->health -= 1.8;	
				}
				if (bug->type == "wasp") {
					froggy->health -= 3.6;
				}
			}	

			// bump froggy down when trying to jump through a bug
			// TODO: instead of bump down, slow velocity
			froggy->velocity.y *= 0.50;

			// TODO: get horizontal bump to work // add a graceperiod so bug damage is more consistent
			// froggy is further left than bug
			if (froggy->position.x < bug->position.x) {
				froggy->velocity.x = -FROGGY_BUMP_VELOCITY_X;
			} 

			// froggy is further right
			if (froggy->position.x > bug->position.x) {
				froggy->velocity.x = FROGGY_BUMP_VELOCITY_X;
			} 

		// froggy is on TOP wew!!!!	
		} else if (froggy->position.y < bug->position.y && (froggy->isJumping || froggy->velocity.y <= FROGGY_FALL_VELOCITY)) {
			// allow the frog to bounce off bug for a boost and kill the bug						
			if (bug->status == ALIVE) {			
				froggy->velocity.y = -FROGGY_JUMP_VELOCITY_Y * 0.65;	
				bug->status = DEAD;	
				// TODO: fix this
				// bug_jump_death(bug, deltaTime);
				froggy->score++;		
			}
		} 
	}	
}

void froggy_death(Frog *froggy) {
	// frog dies when health goes to 0 . . . .	
	if (froggy->health <= 0) {
		froggy->velocity.x = 0;
		froggy->health = 0;
		froggy->status = DEAD;	
		froggy->frame = 5;		
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
			
			// froggy is not moving up or down vertically (or affected by max gravity)
			if (froggy->velocity.y == FROGGY_FALL_VELOCITY && froggy->status == ALIVE) {
				froggy->frame = 1;
				froggy->isJumping = false;	
			}
		}
	}
}

void spawn_mosquito(Bug *mosquito, Frog *froggy, Texture2D mosquito_texture) {	
	// initialize mosquito(es)
    mosquito->texture = mosquito_texture;

	mosquito->position = (Rectangle){		
		640.0f + (GetRandomValue(0, 1) == 0 ? -1440.0f : 800.0f),
		froggy->position.y + GetRandomValue(-500, -400), 
		0.0f, 
		36.0f
	};	

	mosquito->spawnPosition = (Vector2){ mosquito->position.x, mosquito->position.y };

    if (mosquito->spawnPosition.x < 640.0f) { 
        mosquito->direction = RIGHT;          
    } else {
        mosquito->direction = LEFT;         
    }

	// mosquito->direction = GetRandomValue(true, false) ? LEFT : RIGHT;
    mosquito->frame = 0;
	mosquito->status = ALIVE;
	mosquito->isActive = true;
	mosquito->type = "mosquito";
	mosquito->isEaten = false;
}

void spawn_wasp(Bug *wasp, Frog *froggy, Texture2D wasp_texture) {	
	// initialize wasps
    wasp->texture = wasp_texture;

    wasp->position = (Rectangle){
		froggy->position.x + (float)GetRandomValue(-250, 250), 
		froggy->position.y + (float)GetRandomValue(200, 300), 
		0.0f, 
		36.0f
	};

	wasp->spawnPosition = (Vector2){ wasp->position.x, wasp->position.y };
    wasp->angle = 0.0f;
    wasp->radius = (float)GetRandomValue(500, 700);	
	// TODO: spiralspeed 0 should be prevented?
    wasp->spiralSpeed = (float)GetRandomValue(-3,3);
    wasp->convergence = (float)GetRandomValue(12,18);
    wasp->minRadius = 3.0f;	
	wasp->status = ALIVE;
	wasp->isActive = true;
	wasp->type = "wasp";
	wasp->isEaten = false;
}

void make_lilypads(Lilypad *pad, Texture2D lilypad_texture, Frog *froggy) {	
	// initialize the platforms	
	pad->texture = lilypad_texture;
	pad->position = (Rectangle){
		.x = froggy->position.x + GetRandomValue(-700, 600),
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

void remove_lilypads_below(Lilypad *pad, Frog *froggy) {
	// get the highest y value visited
	if (froggy->position.y < froggy->highestPosition) {
        froggy->highestPosition = froggy->position.y;
    }

	// remove lilypads when froggy has climbed certain distance
    if (pad->isActive && pad->position.y > froggy->highestPosition + 1500.0f) {
        pad->isActive = false;
    }
}

int get_highscore() {    
    int highscore = 0;

    // file does not exist, make it
    if (!FileExists("highscore.txt")) {        
        FILE *file = fopen("highscore.txt", "wb");
        if (file) {
            fwrite(&highscore, sizeof(int), 1, file); 
            fclose(file);
        } 
	// file exists
    } else {        
        FILE *file = fopen("highscore.txt", "rb");
        if (file) {
            fread(&highscore, sizeof(int), 1, file);
            fclose(file);
        } 
    }

    return highscore;
}

void update_highscore(Frog *froggy, int highscore) {
	// if current_score > highscore
	if (froggy->score > highscore) {
		FILE *file = fopen("highscore.txt", "wb");
		if (file) {
			// write binary score integer
			fwrite(&froggy->score, sizeof(int), 1, file);
			fclose(file);
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

	int highscore = get_highscore();

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
		.status = ALIVE,
		.highestPosition = 0.0f,
		.isBouncing = false,
		.tongue = (Rectangle){800, 1280, 0, 0},
		.tongueHitbox = (Rectangle){800, 1280, 0, 0},
		.attackDuration = FROGGY_ATTACK_DURATION,
		.size = 1.0
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
	Texture2D wasp_texture = LoadTexture("wasp.png");

	// mosquito (only 2 frames for directions)
	const float frameWidthBug = (float)(mosquito_texture.width / 2);

	// array of mosquitoes/ wasps
	Bug mosquitoes[MAX_MOSQUITOES];
	Bug wasps[MAX_WASPS];

	int activeMosquitoes = 0;	
	int activeWasps = 0;	

	// bug spawntimer
	float mosquitoSpawnTimer = MOSQUITO_SPAWN_TIMER;		
	float waspSpawnTimer = WASP_SPAWN_TIMER;	

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
		// updates
		float deltaTime = GetFrameTime();

		mosquitoSpawnTimer += deltaTime;

		// spawn mosquitoes 
		if (mosquitoSpawnTimer >= MOSQUITO_SPAWN_INTERVAL && activeMosquitoes < MAX_MOSQUITOES) {
			spawn_mosquito(&mosquitoes[activeMosquitoes], &froggy, mosquito_texture);
			activeMosquitoes++;
			mosquitoSpawnTimer = 0.0f;	
		}

		waspSpawnTimer += deltaTime;

		// spawn wasp
		if (waspSpawnTimer >= WASP_SPAWN_INTERVAL && activeWasps < MAX_WASPS) {
			spawn_wasp(&wasps[activeWasps], &froggy, wasp_texture);
			activeWasps++;
			waspSpawnTimer = 0.0f;
		}		
	
		// make initial pads to jump on		
		if (activePads < 40) {
			for (int i = 0; i < 40; i++) {
				make_lilypads(&pads[activePads], lilypad_texture, &froggy);
				activePads++;
			}
		}

		// TODO: this needs looked at still
		// keep spawning lilypads, this time offscreen
		if (froggy.position.y < nextLilypadSpawn) {
			for (int i = 0; i < OFFSCREEN_LILYPAD_SPAWN_AMOUNT && i < MAX_LILLYPADS; i++) {
				make_lilypads_offscreen(&pads[activePads], lilypad_texture, &froggy);
				activePads++;				  
			}
			nextLilypadSpawn -= 400.0f;
		}	
		
		if (froggy.status == ALIVE) {							
			frog_baby_jump(&froggy, maxFrames, deltaTime);
			frog_big_jump(&froggy, maxFrames, deltaTime);
			move_frog(&froggy);				
			frog_attack(&froggy, deltaTime, camera);			
		}
		screen_flip(&froggy);
		apply_velocity(&froggy, deltaTime);	
		apply_gravity(&froggy);
		froggy_death(&froggy);
		
		// update highscore in current game
        if (froggy.score > highscore) {
            highscore = froggy.score;
        }
		// as well as on disk
		update_highscore(&froggy, highscore);

		// update lillypads
		int activePadsAfterLoop = 0;		
		for (int i = 0; i < activePads; i++) {	
			collision_check_pads(&froggy, &pads[i]);			
			remove_lilypads_below(&pads[i], &froggy); 

			// adjust the amount of active pads after without messing up the loop index
			if (pads[i].isActive) {
        		pads[activePadsAfterLoop++] = pads[i];
    		}			
		}	
		activePads = activePadsAfterLoop;

		// update mosquitoes
		int activeMosquitoesAfterLoop = 0;		
		for (int i = 0; i < activeMosquitoes; i++) {	
			move_mosquito(&mosquitoes[i], deltaTime);	
			collision_check_bugs(&froggy, &mosquitoes[i], deltaTime);
			eat_bug(&froggy, &mosquitoes[i], deltaTime);
			deactivate_bug(&mosquitoes[i], &froggy);
			
			// remove inactive mosquitoes after the loop 
			if (mosquitoes[i].isActive) {
				mosquitoes[activeMosquitoesAfterLoop++] = mosquitoes[i];
			}			
		}	
		activeMosquitoes = activeMosquitoesAfterLoop;	

		// update wasps		
		int activeWaspsAfterLoop = 0;
		for (int i = 0; i < activeWasps; i++) {
			move_wasp(&wasps[i], &froggy, deltaTime);
			collision_check_bugs(&froggy, &wasps[i], deltaTime);
			eat_bug(&froggy, &wasps[i], deltaTime);
			deactivate_bug(&wasps[i], &froggy);

			if (wasps[i].isActive) {
				wasps[activeWaspsAfterLoop++] = wasps[i];
			}	
		}
		activeWasps = activeWaspsAfterLoop;

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
		float minY = GetScreenHeight() / 2.0f;
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
		ClearBackground(BLUE);

		// TODO: this approach with stages, new mechanics based on distance travelled vertically
		if (froggy.highestPosition < -4000.0f) {
			ClearBackground(SKYBLUE);
		}
				
		// draw frog
		DrawTexturePro(
			froggy.texture, 
			(Rectangle){
				frameWidth * froggy.frame, 
				0, 
				// flip the texture horizontally depending on direction it's facing
				(froggy.direction == RIGHT) ? -frameWidth: frameWidth,	
				// flip the texture vertically when the frog is dead			
				froggy.texture.height * froggy.status }, 
			(Rectangle){
				froggy.position.x,
				froggy.position.y,
				frameWidth * froggy.size,					
				(float)froggy.texture.height * froggy.size },
			  (Vector2){ 
				frameWidth * froggy.size / 2, 
				(float)froggy.texture.height * froggy.size / 2 
   			}, 
			0,
			(froggy.status == ALIVE) ? RAYWHITE : RED);
	

			
		// DrawTextureRec(
		// 	froggy.texture, 
		// 	(Rectangle){
		// 		frameWidth * froggy.frame, 
		// 		0, 
		// 		// flip the texture horizontally depending on direction it's facing
		// 		(froggy.direction == RIGHT) ? -frameWidth: frameWidth,	
		// 		// flip the texture vertically when the frog is dead			
		// 		froggy.texture.height * froggy.status}, 
		// 	(Vector2){froggy.position.x, froggy.position.y}, 
		// 	// change color based on whether alive or not
		// 	(froggy.status == ALIVE) ? RAYWHITE : RED);				
			 
		draw_tongue(&froggy);



		// draw the bugs
		for (int i = 0; i < activeMosquitoes; i++) {									

			// annoying twitching animation by scaling it from 90 / 100% size every frame :thumbs_up:
			DrawTexturePro(
				mosquitoes[i].texture,
				(Rectangle){
					frameWidthBug * mosquitoes[i].frame,
					0,
					(mosquitoes[i].direction == RIGHT) ? -frameWidthBug : frameWidthBug,
					mosquitoes[i].texture.height * mosquitoes[i].status},
				(Rectangle){
					mosquitoes[i].position.x,
					mosquitoes[i].position.y,
					frameWidthBug * GetRandomValue(9,10) / 10.0,					
					(float)mosquitoes[i].texture.height * GetRandomValue(9,10) / 10.0},
				(Vector2){ 0, 0 },
				0.0f,
				(mosquitoes[i].status == ALIVE) ? RAYWHITE : RED);		

			DrawRectangleLinesEx(mosquitoes[i].hitbox, 1, RED); 
			// DrawText(TextFormat("spawn Position y: %.2f", mosquitoes[i].spawnPosition.y), 10, 310 + 20 * i, 20, WHITE);	
			// DrawText(TextFormat("spawn Position x: %.2f", mosquitoes[i].spawnPosition.x), 10, 330 + 20 * i, 20, WHITE);				
		}

		// draw wasps
		for (int i = 0; i < activeWasps; i++) {
			DrawTextureRec(
				wasps[i].texture,
				(Rectangle){
					0,
					0,
					wasps[i].texture.width,
					wasps[i].texture.height
				},
				(Vector2){ wasps[i].position.x, wasps[i].position.y },
				RAYWHITE
			);

			DrawRectangleLinesEx(wasps[i].hitbox, 1, RED); 
		}

		EndMode2D();
		
		DrawText(TextFormat("Health: %.2f", froggy.health), 500, 0, 40, WHITE);
		DrawText(TextFormat("Score: %d", froggy.score), 500, 40, 40, WHITE);
		DrawText(TextFormat("Highscore: %d", highscore), 500, 80, 40, WHITE);

		// debug 
		DrawText(TextFormat("Position: %.2f", froggy.position.y), 10, 10, 20, WHITE);
		DrawText(TextFormat("Velocity: %.2f", froggy.velocity.y), 10, 40, 20, WHITE);
		DrawText(TextFormat("lilypads: %d", activePads), 10, 70, 20, WHITE);
		DrawText(TextFormat("mosquitoes: %d", activeMosquitoes), 10, 100, 20, WHITE);
		DrawText(TextFormat("next_spawn: %.2f", nextLilypadSpawn), 10, 130, 20, WHITE);
		DrawText(TextFormat("acivebugs_after_loop: %d", activeMosquitoesAfterLoop), 10, 160, 20, WHITE);
		DrawText(TextFormat("frog tongue height: %.2f", froggy.tongue.height), 10, 190, 20, WHITE);
		DrawText(TextFormat("frog tongue width: %.2f", froggy.tongue.width), 10, 220, 20, WHITE);
		DrawText(TextFormat("frog tongue x: %.2f", froggy.tongue.x), 10, 250, 20, WHITE);
		DrawText(TextFormat("frog tongue y: %.2f", froggy.tongue.y), 10, 280, 20, WHITE);
		DrawText(TextFormat("acive wasps: %d", activeWasps), 10, 310, 20, WHITE);
		DrawText(TextFormat("tonguetimer: %.2f", froggy.tongueTimer), 10, 340, 20, WHITE);
		DrawText(TextFormat("attackduration: %.2f", froggy.attackDuration), 10, 370, 20, WHITE);
		DrawText(TextFormat("size: %.2f", froggy.size), 10, 400, 20, WHITE);
		
		if (froggy.status == DEAD) {
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