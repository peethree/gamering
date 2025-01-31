#include "raylib.h"
#include "constants.h"
#include <math.h>
#include "resource_dir.h"	
#include <stdio.h> 

// TODO: 
// maybe add roguelike powers ?? 
// if falling velocity -> allow bug jumping
// add little hearts you can collect to restore health have it spin around or float up and down
// scoring is a MESS atm
// use the statuses instead of various booleans to clean up structs a bit??? might be a shit idea
// cooldown on bug spit, allow bursts of three bugs
// slowdown tongue attack ?
// move the bugs along with the tongue endpoint until the tongue's at the furthest point, then pull them back as it retracts
// make landing on lilypads smoother ideally only interact with the pad when falling from above. keep track of y coordinate when jump was initiated?
// use jumpheight to fix lilypad interactions as well as jumping on bugs
// fix fish hitboxes 
// add some kind of menu when the game is over
// add more bug movement patterns
// add proximity based bug buzzing hehehe
// find a better way to deal with level building
// don't allow mid air jump
// tongue hitbox very inaccurate when froggy y pos is higher (smaller) than bug y pos
// cool backgrounds, stages that change depending on y value
// stage 1: pond, stage 5: space, astronaut frog ???

typedef enum Direction {
	LEFT = -1,
	RIGHT = 1,
} Direction;

typedef enum Status{
	ALIVE = 1,
	DEAD = -1,
	ATTACKING = 2,
	SHOOTING = 3,
	JUMPING = 4,
	BOUNCING = 5,
	ACTIVE = 6,
	EATEN = 7,
	FISHTRAPPED = 8,
} Status;

typedef struct Frog {
    Texture2D texture;
    Rectangle position;
    Rectangle hitbox;
    Rectangle tongue;
    Rectangle tongueHitbox;
    Vector2 velocity;
    Rectangle mouthPosition;
    float tongueTimer;
    float attackDuration;
    float tongueAngle;
    float health;
    float jumpHeight;
    float jumpTimer;
    float frameTimer;
    float highestPosition;
    float size;
    float spitAngle;
    int frame;
    int score;
    int bugsEaten;
    Direction direction;
    Status status;
    bool isAttacking;
    bool isShooting;
    bool isJumping;
    bool isBouncing;
} Frog;

typedef struct Bug{	
	Texture2D texture;
	Rectangle position;
	Rectangle hitbox;		
	Direction direction;
	Vector2 velocity;
	Vector2 targetPosition;
	Vector2 previousPosition;
	Vector2 spawnPosition;
	Vector2 desiredVelocity;
	char* type;	
	float angle;
	float radius;
	float spiralSpeed;
	float convergence;	
	float minRadius; 	
	float waveFrequency;
	float waveAmplitude;
	float frameTimer;	
	float health;
	int frame;
	Status status;			
	bool isActive;	
	bool isEaten;	
} Bug;

typedef struct Bugspit{
	Texture2D texture;
	Rectangle hitbox;
	Rectangle position;
	Vector2 velocity;
	Status status;
	float angle;
	bool isActive;		
} Bugspit;

typedef struct Lilypad{
	Texture2D texture;
	Rectangle position;
	Rectangle hitbox;
	Status status;
	int frame;
	bool isActive;
	bool hasFishTrap;
	bool hasHeart;
} Lilypad;

typedef struct Fish{
	Texture2D texture;
	Rectangle position;
	Rectangle hitbox;
	float attackDuration;
	float attackTimer;
	float frameTimer;
	Status status;
	int frame;
	bool isActive;
	bool isAttacking;
} Fish;

typedef struct Heart{
	Texture2D texture;
	Rectangle position;
	Rectangle hitbox;
	bool isActive;
} Heart;

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

		// TODO: set this to false when colliding with a lilypad instead
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
			froggy->jumpHeight = froggy->position.y;		

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
			froggy->jumpHeight = froggy->position.y;		

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

// allow movement from left side of the screen to right side 
void screen_flip(Frog *froggy) {	
	// 8 pixel grids / frames in the texture image -> divide by 8
	if (froggy->position.x < 0.0f - froggy->texture.width / 8) {
		froggy->position.x = (float)GetScreenWidth();
	} else if (froggy->position.x > (float)GetScreenWidth() + froggy->texture.width / 8) {
		froggy->position.x = 0.0f;
	}
}

// update frog hitbox in one place
void hitbox_frog(Frog *froggy) {	
	froggy->hitbox = (Rectangle){
		.x = froggy->position.x - 20.0f,
		.y = froggy->position.y - 20.0f,
		.width = 35.0f,
		.height = 35.0f
	};	
}
 
void tongue_attack(Frog *froggy, float angle, float deltaTime, Vector2 cameraMousePosition) {
	if (froggy->isAttacking) {
        froggy->tongueTimer += deltaTime;
        float progress;

		// attack animation
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
        
        // tongue endpoint
        float tongueEndX = froggy->mouthPosition.x + cosf(angle) * currentLength;
        float tongueEndY = froggy->mouthPosition.y + sinf(angle) * currentLength;
        
        // TODO: this spazzes out the frog when aiming above it
		// turn frog in direction of the mouseclick, added deadzone		
        froggy->direction = (cameraMousePosition.x < froggy->mouthPosition.x + 10.0f) ? LEFT : RIGHT;		

		// tongue rectangle
        froggy->tongue = (Rectangle){
            froggy->mouthPosition.x,
            froggy->mouthPosition.y,
            currentLength,
            FROGGY_TONGUE_WIDTH
        };
        
        // angle needed for drawing
        froggy->tongueAngle = angle * RAD2DEG;          
		
        float tongueWidth = FROGGY_TONGUE_WIDTH * froggy->size;
		float halfWidth = tongueWidth / 2.0f;
        
        // hitbox corners
        Vector2 topLeft = {
            froggy->mouthPosition.x - halfWidth * sinf(angle),
            froggy->mouthPosition.y + halfWidth * cosf(angle)
        };        
        Vector2 bottomRight = {
            tongueEndX + halfWidth * sinf(angle),
            tongueEndY - halfWidth * cosf(angle)
        };
        
		// taper hitbox near the end of its length
		float currentHitboxWidth = tongueWidth * (1.0f - (currentLength / (FROGGY_TONGUE_LENGTH)) * 0.5f); 
        // ceate hitbox as a rectangle that encompasses the rotated tongue
        froggy->tongueHitbox = (Rectangle){
            fminf(topLeft.x, bottomRight.x),
            fminf(topLeft.y, bottomRight.y),
            fabsf(tongueEndX - froggy->mouthPosition.x) + halfWidth,
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

// use the eaten bugs as ammo to kill wasps
void spit_bug(Frog *froggy, float angle, Vector2 cameraMousePosition, Texture2D bugspit_texture, int *activeSpit, Bugspit *spitties) {	
	if (froggy->bugsEaten > 0 && *activeSpit < FROGGY_MAX_BUG_SPIT && froggy->status == ALIVE) {
		if (froggy->isShooting) {		
			// swap direction based on mouse cursor position compared to the frog	
			froggy->direction = (cameraMousePosition.x < froggy->mouthPosition.x + 10.0f) ? LEFT : RIGHT;

			// direction x, y
			Vector2 spitDirection = {
				cosf(angle),
				sinf(angle)
			};			

			// initialize new bug spit at activespit index
			// TODO: fix the texture, is now 2 frames, maybe make frumbled up sticky bug
			spitties[*activeSpit] = (Bugspit){
				.position = froggy->mouthPosition,
				.texture = bugspit_texture,
				.isActive = true,
				.velocity = (Vector2){
					spitDirection.x * FROGGY_BUG_SPIT_SPEED,
					spitDirection.y * FROGGY_BUG_SPIT_SPEED
				},
				.angle = angle * RAD2DEG
			};	

			// increment amount active
			(*activeSpit)++;	
			// decrement bugs eaten (ammo)
			froggy->bugsEaten--;	
		}
	} 
}

// update spit hitbox
void hitbox_spit(Bugspit *spitty) {
	spitty->hitbox = (Rectangle){			
		.x = spitty->position.x + 13.0f,
		.y = spitty->position.y + 1.0f,
		.width = 30.0f,
		.height = 30.0f
	};	
}

void collision_check_spit(Bugspit *spitty, Bug *bug) {
	if (CheckCollisionRecs(spitty->hitbox, bug->hitbox)) {
		// TODO: figure out a value that doesn't 1 shot.
		if (bug->type == "wasp") {
			bug->health -= 10.0;
		// oneshot
		} else if (bug->type = "mosquito") {
			bug->health -= 1.0;
		}		
	}
}

void draw_spit(Bugspit *spitty) {
	DrawTexture(
		spitty->texture, 
		spitty->position.x, 
		spitty->position.y, 
		RAYWHITE);

	// debug spit hitbox
    DrawRectanglePro(
		spitty->hitbox,
		(Vector2){ 0, 0},
		spitty->angle,
		RAYWHITE
	);
}

// apply velocity to spit projectile
void spit_velocity(Bugspit *spitty, float deltaTime) {
	spitty->position.x += spitty->velocity.x * deltaTime;
	spitty->position.y += spitty->velocity.y * deltaTime;
}

// deactivate offscreen spits
// TODO: some spits aren't deactivating
void deactivate_spit(Bugspit *spitty, Frog *froggy, int *activeSpit) {
	// 	clean up spits at y distance difference
	if (spitty->isActive && (spitty->position.y > froggy->position.y + 1000.0f || spitty->position.y < froggy->position.y - 1000.0f)) {
		spitty->isActive = false;
		(*activeSpit)--;
	}

	if (spitty->isActive && (spitty->position.x > 1300.0f || spitty->position.x < -100.0f)) {
		spitty->isActive = false;
		(*activeSpit)--;
	}
}

void frog_mouth_position(Frog *froggy) {
	if (froggy->direction == RIGHT) {				   //offsets to spawn tongue closer to mouth
		froggy->mouthPosition.x = froggy->position.x + (7.0f * froggy->size * 1.9);
		froggy->mouthPosition.y = froggy->position.y - (4.0f * froggy->size * 1.9);
	} else {
		froggy->mouthPosition.x = froggy->position.x - (7.0f * froggy->size * 1.9);
		froggy->mouthPosition.y = froggy->position.y - (4.0f * froggy->size * 1.9);				
	}
}
 
void frog_attacks(Frog *froggy, float deltaTime, Camera2D camera, Texture2D bugspit_texture, int *activeSpit, Bugspit *spitties) {	
	// despawn tongue when froggy dies
	if (froggy->status == DEAD) {
		froggy->isAttacking = false;
		froggy->isShooting = false;
	}

	// keep track of cursor coordinates, translate them into camera coords for hitbox logic
	Vector2 cursorPosition = GetMousePosition();
	Vector2 cameraMousePosition = GetScreenToWorld2D(cursorPosition, camera);

	// keep track of where on the sprite the tongue should appear from


	// direction vectors
	float dx = cameraMousePosition.x - froggy->mouthPosition.x;
	float dy = cameraMousePosition.y - froggy->mouthPosition.y;
	
	// angle
	float angle = atan2f(dy, dx);

	// tongue lash
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !froggy->isAttacking && !froggy->isShooting) {
        froggy->isAttacking = true;
        froggy->tongueTimer = 0.0f;        
    }	
	tongue_attack(froggy, angle, deltaTime, cameraMousePosition);	

	// spit bug
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !froggy->isAttacking && !froggy->isShooting) {
		froggy->isShooting = true;
	}
	spit_bug(froggy, angle, cameraMousePosition, bugspit_texture, activeSpit, spitties);
	froggy->isShooting = false;
}

void draw_tongue(Frog *froggy) {
	if (froggy->isAttacking) {
        DrawRectanglePro(
			froggy->tongue,
			(Vector2){0, FROGGY_TONGUE_WIDTH / 2},		
			froggy->tongueAngle,
			RED
		);
		
		// DrawRectanglePro(
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
		mosquito->status = DEAD;
	// bug should fall off the screen in case of jump death
	} else {			
		mosquito->velocity.y = MOSQUITO_VELOCITY_DEATH_FALL; 
		mosquito->position.y += mosquito->velocity.y * deltaTime;	
	}
}

// TODO: consider adding another wasp frame that's attempting to grab the froggy once
// the wasp reaches a certain distance away from the froggy.
void animate_wasp(Bug *wasp, float deltaTime, int maxFramesWasp) {
	// flying animation for wasps
	float frameDuration = 0.1f;
	wasp->frameTimer += deltaTime;

	if (wasp->frameTimer >= frameDuration) {
		wasp->frameTimer = 0.0f;
		wasp->frame++;
		wasp->frame %= maxFramesWasp;
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

		if (dif > 0.0) {
			wasp->direction = LEFT;
		} else {
			wasp->direction = RIGHT;
		}
	// bug should fall off the screen in case of jump death
	} else {			
		wasp->velocity.y = WASP_VELOCITY_DEATH_FALL; 
		wasp->position.y += wasp->velocity.y * deltaTime;	
		wasp->status = DEAD;
		froggy->score += 5;
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
void bug_spit_death(Bug *bug, float deltaTime, Frog *froggy) {
	if (bug->status == DEAD) {
		if (bug->type == "wasp") {
			bug->velocity.y = WASP_VELOCITY_DEATH_FALL; 
			froggy->score += 5;
		} else if (bug->type == "mosquito") {
			bug->velocity.y = MOSQUITO_VELOCITY_DEATH_FALL; 
			froggy->score++;
		}		
		bug->position.y += bug->velocity.y * deltaTime;
	}
}

// you will eat the bugs
void eat_bug(Frog *froggy, Bug *bug, float deltaTime) {
    // start pulling the bug toward the froggy when the tongue starts retracting
    if (froggy->status == ALIVE && bug->isEaten && froggy->tongueTimer >= 0.5 * FROGGY_TONGUE_TIMER) {      
		
        // direction vector        
        float dx = froggy->mouthPosition.x - (bug->position.x + bug->texture.width / 4);
        float dy = froggy->mouthPosition.y - (bug->position.y + bug->texture.height / 2);         	

        // movement amount this frame
        float moveAmount = FROGGY_TONGUE_BUG_PULL_SPEED * deltaTime;
        float distance = sqrtf(dx * dx + dy * dy);

		// move bug toward froggy
        if (distance > 5.0) {
            // normalize direction, move
            bug->position.x += (dx / distance) * moveAmount;
            bug->position.y += (dy / distance) * moveAmount;
        } else {
			bug->position = froggy->mouthPosition;
		}	

		// increase in size from eating bug
		if (CheckCollisionRecs(froggy->hitbox, bug->hitbox)) {
			froggy->size += 0.05;
			bug->isActive = false;
			froggy->bugsEaten++;
		}	
    }
}

void hitbox_bug(Bug *bug) {
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
			.x = bug->position.x + 10.0f,
			.y = bug->position.y + 1.0f,
			.width = 100.0f,
			.height = 100.0f
		};
	}
}

void collision_check_bugs(Frog *froggy, Bug *bug, float deltaTime) {
	// when the tongue hits a mosquito, eat it
	if (CheckCollisionRecs(froggy->tongueHitbox, bug->hitbox) && bug->type == "mosquito") {
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
			// froggy->velocity.y *= 0.50;

			// TODO: horizontal bumps do not work
			// froggy is further left than bug
			// if (froggy->position.x < bug->position.x) {
			// 	froggy->velocity.x = -FROGGY_BUMP_VELOCITY_X;
			// } 

			// // froggy is further right
			// if (froggy->position.x > bug->position.x) {
			// 	froggy->velocity.x = FROGGY_BUMP_VELOCITY_X;
			// } 

		// froggy is on TOP wew!!!!	
		} else if (froggy->position.y < bug->position.y && (froggy->isJumping || froggy->velocity.y <= FROGGY_FALL_VELOCITY)) {
			// allow the frog to bounce off bug for a boost and kill the bug						
			if (bug->status == ALIVE) {			
				froggy->velocity.y = -FROGGY_JUMP_VELOCITY_Y * 0.65;	
				bug->status = DEAD;		
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

void bug_death(Bug *bug) {
	if (bug->health <= 0) {		
		bug->health = 0;
		bug->status = DEAD;				
	}
}

// collision with lilypads
void collision_check_pads(Frog *froggy, Lilypad *pad) {	
	// lilypad hitbox(es)
	switch (pad->frame) {
		case 0:
			pad->hitbox = (Rectangle){
				.x = pad->position.x + 5.0f,
				.y = pad->position.y + 15.0f,
				.width = 90.0f,
				.height = 3.0f
			};
			break;
		case 1:
			pad->hitbox = (Rectangle){
				.x = pad->position.x + 5.0f,
				.y = pad->position.y + 15.0f,
				.width = 85.0f,
				.height = 3.0f
			};
			break;
		case 2:
			pad->hitbox = (Rectangle){
				.x = pad->position.x + 5.0f,
				.y = pad->position.y + 15.0f,
				.width = 60.0f,
				.height = 3.0f
			};
			break;
		case 3:
			pad->hitbox = (Rectangle){
				.x = pad->position.x + 0.0f,
				.y = pad->position.y + 15.0f,
				.width = 50.0f,
				.height = 3.0f
			};
			break;
		default:
			break;
	}

    //TODO: frog lilypad collision is REAL nasty atm, look INTO it
   	
//         // if froggy is below the pad
// 	if (froggy->position.y > pad->position.y && pad->isActive) {	
// 		if (CheckCollisionRecs(froggy->hitbox, pad->hitbox)) {		
// 			froggy->position.y = pad->hitbox.y - froggy->hitbox.height;
// 			froggy->onPad = true;
// 			froggy->frame = 1;
// 			// froggy->velocity.y = 0.0f;
// 		}
// 	}
//     // if froggy is above the pad
// 	else if (froggy->position.y < pad->position.y && pad->isActive) {
// 		if (CheckCollisionRecs(froggy->hitbox, pad->hitbox)) {
//             froggy->position.y = pad->hitbox.y - froggy->hitbox.height; 
//             froggy->velocity.y = 0.0f;             
//             froggy->isJumping = false;	
// 			froggy->onPad = true;
//         }
//     }
// }

	// ideally: if frog velocity is negative (meaning frog is going up, do nothing when colliding with lilypad)
	// otherwise, catch the froggy (update the position upon collision)

	// froggy is below the pad
	if (froggy->position.y > pad->position.y && pad->isActive) {
		if (CheckCollisionRecs(froggy->hitbox, pad->hitbox)) {									
			froggy->position.y += (pad->position.y - froggy->position.y) * 0.8f;	

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
		// either spawn on the left or the right side just past the window's edge	
		640.0f + (GetRandomValue(0, 1) == 0 ? -1440.0f : 800.0f),
		froggy->position.y + GetRandomValue(-500, -400), 
		0, 
		0
	};	

	mosquito->spawnPosition = (Vector2){ mosquito->position.x, mosquito->position.y };

    if (mosquito->spawnPosition.x < 640.0f) { 
        mosquito->direction = RIGHT;          
    } else {
        mosquito->direction = LEFT;         
    }

    mosquito->frame = 0;
	mosquito->status = ALIVE;
	mosquito->isActive = true;
	mosquito->type = "mosquito";
	mosquito->isEaten = false;
	mosquito->health = 1.0;
}

void spawn_wasp(Bug *wasp, Frog *froggy, Texture2D wasp_texture) {	
	// initialize wasps
    wasp->texture = wasp_texture;
    wasp->position = (Rectangle){
		froggy->position.x + (float)GetRandomValue(-250, 250), 
		froggy->position.y + (float)GetRandomValue(200, 300), 
		0, 
		0
	};

	wasp->spawnPosition = (Vector2){ wasp->position.x, wasp->position.y };
    wasp->angle = 0;
    wasp->radius = (float)GetRandomValue(500, 700);	
	// TODO: spiralspeed 0 should be prevented?
    wasp->spiralSpeed = (float)GetRandomValue(-3,3);
    wasp->convergence = (float)GetRandomValue(12,18);
    wasp->minRadius = 3.0f;	
	wasp->status = ALIVE;
	wasp->isActive = true;
	wasp->type = "wasp";
	wasp->isEaten = false;
	wasp->frameTimer = 0;
	wasp->health = 10.0;
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
	pad->hasFishTrap = false;
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
	pad->hasFishTrap = false;
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

// once fish is active, animate it (once).
void animate_fish(Fish *fishy, float deltaTime, int maxFramesFish) {	
	if (fishy->isAttacking) {
		float frameDuration = 0.2f;
		fishy->frameTimer += deltaTime;

		if (fishy->frameTimer >= frameDuration) {
			fishy->frameTimer = 0.0f;
			fishy->frame++;
			fishy->frame %= maxFramesFish;
		}	
	}
}

// fish lays (swims) in ambush at location of the lilypads
void spawn_fish(Fish *fishy, Texture2D fish_texture, Lilypad *pads, int activePads) {
	// pick a random lilypad between 0 and the max active pads for a fish to spawn under
	int randomValue = GetRandomValue(0, activePads - 1);	

	// don't double spawn fish on lilypads
	if (!pads[randomValue].hasFishTrap) {
		fishy->position = pads[randomValue].position;	
		fishy->isActive = true;
		pads[randomValue].hasFishTrap = true;
	}

	fishy->frameTimer = 0.0f;
	fishy->texture = fish_texture;
	fishy->isAttacking = false;
	fishy->attackDuration = 1.6f;	
	fishy->isActive = true;
	fishy->attackTimer = 0.0f;
	fishy->frame = 0;
}

// trigger the fish by stepping on its initial hitbox 
void activate_fish(Fish *fishy, Frog *froggy) {
	if (!fishy->isAttacking) {
		// fish hitbox 
		fishy->hitbox = (Rectangle){
			.x = fishy->position.x,
			.y = fishy->position.y,
			.width = 60.0f,
			.height = 20.0f
		};	

		if (CheckCollisionRecs(fishy->hitbox, froggy->hitbox) && fishy->isActive) {
			fishy->isAttacking = true;
		}
	}
}

// after fish has been activated, change hitbox based on the frame
void attacking_fish_collision(Fish *fishy, Frog *froggy) {
	switch(fishy->frame) {
		case 0:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y,
				.width = 60.0f,
				.height = 20.0f
			};
			break;
		case 1:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y + 20.0f,
				.width = 60.0f,
				.height = 30.0f
			};		
			break;	
		case 2:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y + 15.0f,
				.width = 60.0f,
				.height = 40.0f
			};
			break;
		case 3:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y - 10.0f,
				.width = 60.0f,
				.height = 60.0f
			};
			break;
		case 4:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y - 40.0f,
				.width = 60.0f,
				.height = 80.0f
			};
			break;
		case 5:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y - 30.0f,
				.width = 60.0f,
				.height = 70.0f
			};
			break;	
		case 6:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y - 20.0f,
				.width = 60.0f,
				.height = 40.0f
			};
			break;
		case 7:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y - 20.0f,
				.width = 60.0f,
				.height = 1.0f
			};
			break;
		default:
			break;
	}

	if (fishy->isAttacking && fishy->frame > 1) {
		if (CheckCollisionRecs(fishy->hitbox, froggy->hitbox)) {
			froggy->health -= 0.15;
		}
	}
}

void deactivate_fish(Fish *fishy, Frog *froggy, float deltaTime) {	
	// deactivate fish at y distance difference			
	if (fishy->isActive && (fishy->position.y > froggy->position.y + 1000.0f)) {
		fishy->isActive = false;
		fishy->isAttacking = false;
	}

	// deactivate after 1 attack
	if (fishy->isAttacking) {
		fishy->attackTimer += deltaTime;
		
		if (fishy->attackTimer >= fishy->attackDuration) {
			fishy->isActive = false;
			fishy->isAttacking = false;
			fishy->attackTimer = 0.0f; 
		}
	}
}

void spawn_healing_heart(Heart *hearty, Texture2D heart_texture) {
	hearty->texture = heart_texture;
	hearty->position = (Rectangle){250,400,0,0}; // TODO: spawn on random lilypad above the frog -- NEEDS to be quite rare/ can be on a random timer
	// hearty->frame = 0;
	hearty->hitbox = (Rectangle){
		hearty->position.x,
		hearty->position.y,
		12.0,
		12.0
	};
	hearty->isActive = true; // set to inactive when froggy interacts with it and uses it
}

void animate_heart(Heart *hearty, float deltaTime) {
	// bounce up and down

	// move y posish smoothly
}

void draw_heart(Heart *hearty) {
	DrawTextureRec(
		hearty->texture,
		(Rectangle){
			0,
			0,
			hearty->texture.width,
			hearty->texture.height
		},
		(Vector2){
			hearty->position.x,
			hearty->position.y
		},
		RAYWHITE
	);
}

int get_highscore() {    
    int highscore = 0;

    // file doesn't exist, make it
    if (!FileExists("highscore.txt")) {        
        FILE *file = fopen("highscore.txt", "w");
        if (file) {
            fprintf(file, "%d", highscore);
            fclose(file);
        } 
	// file exists, read highscore
    } else {        
        FILE *file = fopen("highscore.txt", "r");
        if (file) {
            fscanf(file, "%d", &highscore);
            fclose(file);
        } 
    }

    return highscore;
}

void update_highscore(Frog *froggy, int highscore) {
    // current_score > highscore
    if (froggy->score > highscore) {
        FILE *file = fopen("highscore.txt", "w");
        if (file) {
            fprintf(file, "%d", froggy->score); 
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
		.tongue = (Rectangle){ 800, 1280, 0, 0 },
		.tongueHitbox = (Rectangle){ 800, 1280, 0, 0 },
		.attackDuration = FROGGY_ATTACK_DURATION,
		.size = 1.0,	
		.bugsEaten = 0,
		.isShooting = false
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
	Texture2D wasp_texture = LoadTexture("WASP.png");
	Texture2D fish_texture = LoadTexture("fish.png");
	Texture2D bugspit_texture = LoadTexture("bugspit.png");
	Texture2D heart_texture = LoadTexture("heart.png");

	// mosquito (only 2 frames for directions)
	const float frameWidthBug = (float)(mosquito_texture.width / 2);

	// wasp 2 frames for flying animation
	const float frameWidthWasp = (float)(wasp_texture.width / 2);
	const int maxFramesWasp = (int)(wasp_texture.width / frameWidthWasp);

	// fish 8 frames 
	const float frameWidthFish = (float)(fish_texture.width / 8);
	const int maxFramesFish = (int)(fish_texture.width / frameWidthFish);

	// array of mosquitoes / wasps / fish
	Bug mosquitoes[MAX_MOSQUITOES];
	Bug wasps[MAX_WASPS];
	Fish fishies[MAX_FISH];
	Bugspit spitties[FROGGY_MAX_BUG_SPIT];
	Lilypad pads[MAX_LILLYPADS];
	Heart hearties[MAX_HEARTS];

	int activeMosquitoes = 0;	
	int activeWasps = 0;	
	int activeFish = 0;
	int activeSpit = 0;
	int activePads = 0;
	int activeHearts = 0;

	// bug spawntimer
	float mosquitoSpawnTimer = MOSQUITO_SPAWN_TIMER;		
	float waspSpawnTimer = WASP_SPAWN_TIMER;	

	// lilipad init
	Texture2D lilypad_texture = LoadTexture("lilipads.png");	
	
	// 4 frames in lilypad sprite sheet
	const float frameWidthLilypad = (float)(lilypad_texture.width / 4);	
	
	float nextLilypadSpawn = 0.0f;	

	// TODO: heart spawn timer

	// game loop
	while (!WindowShouldClose()) // run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		// updates
		float deltaTime = GetFrameTime();

		mosquitoSpawnTimer += deltaTime;
		waspSpawnTimer += deltaTime;

		// spawn mosquitoes 
		if (mosquitoSpawnTimer >= MOSQUITO_SPAWN_INTERVAL && activeMosquitoes < MAX_MOSQUITOES) {
			spawn_mosquito(&mosquitoes[activeMosquitoes], &froggy, mosquito_texture);
			activeMosquitoes++;
			mosquitoSpawnTimer = 0.0f;	
		}		

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

		// spawn fish
		if (activeFish < MAX_FISH) {			
			spawn_fish(&fishies[activeFish], fish_texture, pads, activePads);
			activeFish++;		
		}

		// spawn hearts
		if (activeHearts < MAX_HEARTS) {
			spawn_healing_heart(&hearties[activeHearts], heart_texture);
			activeHearts++;
		}

		hitbox_frog(&froggy);		
		if (froggy.status == ALIVE) {									
			frog_baby_jump(&froggy, maxFrames, deltaTime);
			frog_big_jump(&froggy, maxFrames, deltaTime);
			move_frog(&froggy);						
		}
		frog_mouth_position(&froggy);
		// lots of function params, kind of N A S T Y 
		frog_attacks(&froggy, deltaTime, camera, mosquito_texture, &activeSpit, spitties);	
		screen_flip(&froggy);
		apply_velocity(&froggy, deltaTime);	
		apply_gravity(&froggy);
		froggy_death(&froggy);
		
		// update highscore in current game
        if (froggy.score > highscore) {
            highscore = froggy.score;
        }

		// update spitties
		int activeSpitAfterLoop = 0;
		for (int i = 0; i < activeSpit; i++) {
			hitbox_spit(&spitties[i]);
			spit_velocity(&spitties[i], deltaTime);
			deactivate_spit(&spitties[i], &froggy, &activeSpit);

			if (spitties[i].isActive) {
				spitties[activeSpitAfterLoop++] = spitties[i];
			}
		}
		activeSpit = activeSpitAfterLoop;

		// update hearts
		int activeHeartsAfterLoop = 0;
		for (int i = 0; i < activeHearts; i++) {
			// DO HEART STUFF
			// spawn healinh hearts
			if (hearties[i].isActive) {
				hearties[activeHeartsAfterLoop++] = hearties[i];
			}
		}
		activeHearts = activeHeartsAfterLoop;

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
			hitbox_bug(&mosquitoes[i]);
			collision_check_bugs(&froggy, &mosquitoes[i], deltaTime);
			eat_bug(&froggy, &mosquitoes[i], deltaTime);

			for (int j = 0; j < activeSpit; j++) {
				collision_check_spit(&spitties[j], &mosquitoes[i]);
			}

			bug_death(&mosquitoes[i]);
			bug_spit_death(&mosquitoes[i], deltaTime, &froggy);
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
			animate_wasp(&wasps[i], deltaTime, maxFramesWasp); 
			move_wasp(&wasps[i], &froggy, deltaTime);
			hitbox_bug(&wasps[i]);
			collision_check_bugs(&froggy, &wasps[i], deltaTime);				
			eat_bug(&froggy, &wasps[i], deltaTime);

			// this works but is it slow as shit?????
			for (int j = 0; j < activeSpit; j++) {
				collision_check_spit(&spitties[j], &wasps[i]);
			}

			bug_death(&wasps[i]);
			bug_spit_death(&wasps[i], deltaTime, &froggy);
			deactivate_bug(&wasps[i], &froggy);

			if (wasps[i].isActive) {
				wasps[activeWaspsAfterLoop++] = wasps[i];
			}	
		}
		activeWasps = activeWaspsAfterLoop;

		// update fish
		int activeFishAfterLoop = 0;
		for (int i = 0; i < activeFish; i++) {
			activate_fish(&fishies[i], &froggy);
			animate_fish(&fishies[i], deltaTime, maxFramesFish);	
			attacking_fish_collision(&fishies[i], &froggy);		
			deactivate_fish(&fishies[i], &froggy, deltaTime);			

			if (fishies[i].isActive) {
				fishies[activeFishAfterLoop++] = fishies[i];
			}	
		}
		activeFish = activeFishAfterLoop;	

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
		DrawRectangleLinesEx(froggy.hitbox, 1, GREEN); 	
			
		for (int i = 0; i < activePads; i++) {		
			if (!pads[i].isActive) continue;

			// draw lilypads
			DrawTextureRec(
				pads[i].texture,
				(Rectangle){
					frameWidthLilypad * pads[i].frame,
					0,
					frameWidthLilypad,
					pads[i].texture.height	
				},
				(Vector2){ (float)pads[i].position.x,(float)pads[i].position.y},
				RAYWHITE
			);
			
			// lilypad hitbox visual
			DrawRectangleLinesEx(pads[i].hitbox, 1, RED); 			
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
			 
		draw_tongue(&froggy);

		// draw bug spit
		for (int i = 0; i < activeSpit; i++) {
			if (!spitties[i].isActive) continue;
			draw_spit(&spitties[i]);			
		}

		// draw hearts 
		for (int i = 0; i < activeHearts; i++) {
			draw_heart(&hearties[i]);
		}

		// draw fish
		for (int i = 0; i < activeFish; i++) {
			// only draw while active
			if (!fishies[i].isActive) continue;

			DrawTextureRec(
				fishies[i].texture,
				(Rectangle){
					frameWidthFish * fishies[i].frame,
					0,
					frameWidthFish,
					fishies[i].texture.height
				},
				(Vector2){ fishies[i].position.x, fishies[i].position.y},
				RAYWHITE
			);

			// draw fish hitbox
			DrawRectangleLinesEx(fishies[i].hitbox, 1, RED); 
		}

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
			DrawTexturePro(
				wasps[i].texture,
				(Rectangle){
					frameWidthWasp * wasps[i].frame,
					0,
					frameWidthWasp * wasps[i].direction,
					wasps[i].texture.height * wasps[i].status
				},
				(Rectangle){
					wasps[i].position.x,
					wasps[i].position.y,
					frameWidthWasp * GetRandomValue(9, 10) / 10.0,					
					(float)wasps[i].texture.height * GetRandomValue(9, 10) / 10.0},
				(Vector2){ 0, 0},
				0.0f,
				(wasps[i].status == ALIVE) ? RAYWHITE : RED
			);

			DrawRectangleLinesEx(wasps[i].hitbox, 1, RED); 
		}

		EndMode2D();
		
		DrawText(TextFormat("Health: %.2f", froggy.health), 500, 0, 40, RED);
		DrawText(TextFormat("Score: %d", froggy.score), 500, 40, 40, WHITE);
		DrawText(TextFormat("Highscore: %d", highscore), 500, 80, 40, YELLOW);

		// debug 
		DrawText(TextFormat("Position: %.2f", froggy.position.y), 10, 10, 20, WHITE);
		DrawText(TextFormat("Velocity: %.2f", froggy.velocity.y), 10, 40, 20, WHITE);
		DrawText(TextFormat("lilypads: %d", activePads), 10, 70, 20, WHITE);
		DrawText(TextFormat("mosquitoes: %d", activeMosquitoes), 10, 100, 20, WHITE);
		DrawText(TextFormat("next_spawn: %.2f", nextLilypadSpawn), 10, 130, 20, WHITE);
		DrawText(TextFormat("acivebugs_after_loop: %d", activeMosquitoesAfterLoop), 10, 160, 20, WHITE);
		// DrawText(TextFormat("frog tongue height: %.2f", froggy.tongue.height), 10, 190, 20, WHITE);
		// DrawText(TextFormat("frog tongue width: %.2f", froggy.tongue.width), 10, 220, 20, WHITE);
		// DrawText(TextFormat("frog tongue x: %.2f", froggy.tongue.x), 10, 250, 20, WHITE);
		// DrawText(TextFormat("frog tongue y: %.2f", froggy.tongue.y), 10, 280, 20, WHITE);
		DrawText(TextFormat("acive wasps: %d", activeWasps), 10, 310, 20, WHITE);
		DrawText(TextFormat("tonguetimer: %.2f", froggy.tongueTimer), 10, 340, 20, WHITE);
		DrawText(TextFormat("attackduration: %.2f", froggy.attackDuration), 10, 370, 20, WHITE);
		DrawText(TextFormat("size: %.2f", froggy.size), 10, 400, 20, WHITE);
		DrawText(TextFormat("activeFish: %d", activeFish), 10, 430, 20, WHITE);
		DrawText(TextFormat("bugsEaten: %d", froggy.bugsEaten), 10, 460, 20, WHITE);
		DrawText(TextFormat("isShooting: %d", froggy.isShooting), 10, 490, 20, WHITE);
		DrawText(TextFormat("activeSpit: %d", activeSpit), 10, 520, 20, WHITE);
		DrawText(TextFormat("jumpheight: %.2f", froggy.jumpHeight), 10, 550, 20, WHITE);
		DrawText(TextFormat("activeHearts: %d", activeHearts), 10, 580, 20, WHITE);


		// :-C		
		if (froggy.status == DEAD) {
			DrawText("FROGGY HAS PERISHED", 400, 400, 42, RED);
		}

		// end the frame and get ready for the next one (display frame, poll input, etc...)
		EndDrawing();		
	}

	// update highscore
	highscore = get_highscore();
	update_highscore(&froggy, highscore);

	// cleanup
	// unload our textures so it can be cleaned up
	UnloadTexture(frog_texture);
	UnloadTexture(mosquito_texture);	
	UnloadTexture(lilypad_texture);
	UnloadTexture(fish_texture);
	UnloadTexture(bugspit_texture);

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}