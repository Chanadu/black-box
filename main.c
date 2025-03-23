#include <stdlib.h>
#include "blackbox.h"

// Store a handle to the timeout task in case of super fast presses
task_handle turnOffTaskHandler;

const uint8_t COUNTDOWN_SECONDS = 1;
const time_duration GAME_TICK_TIME_MS = 50;

uint32_t currentTotalSeconds = 0;
bool showingData = false;

const time_duration TARGET_FLASH_TIME_MS = 500;
const time_duration TARGET_FLASH_TIME_ON_MS = 400;
task_handle flashTaskHandler;

int8_t playerLocation[2] = {0, 0};
int8_t targetLocation[2] = {7, 7};

int8_t* flashLocations[10];

led_state boardState[8][8] = {
	{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0},
};

bool upPressed = false;
bool downPressed = false;
bool leftPressed = false;
bool rightPressed = false;

void setupVars() {
	flashLocations[0] = targetLocation;
	for (int i = 1; i < 10; i++) {
		flashLocations[i] = (int8_t*)malloc(2 * sizeof(int8_t));
		flashLocations[i][0] = -1;
		flashLocations[i][1] = -1;
	}
}

void showCountdown() {
	bb_matrix_all_off();

	bb_slice_set_int(
		33, 38,
		COUNTDOWN_SECONDS - currentTotalSeconds > 0 ? COUNTDOWN_SECONDS - currentTotalSeconds : 0);
	debug_print("Countdown: %d\n", COUNTDOWN_SECONDS - currentTotalSeconds);
}

void updateCountdown(task_handle self) {
	currentTotalSeconds += 1;
	if (showingData == false) {
		showCountdown();
	}
	if (COUNTDOWN_SECONDS <= currentTotalSeconds) {
		showingData = true;
	}
}

/* void turnOff(task_handle self) { */
/* 	// Turn off the pixel and stop playing the tone */
/* 	bb_matrix_set_pos(1, 1, LED_OFF); */
/* 	bb_tone_off(); */
/* } */
/**/
/* void turnOn(task_handle self) { */
/* 	if (!showingData) { */
/* 		return; */
/* 	} */
/**/
/* 	// Cancel the previous timeout task in case the button is pressed super
 * fast */
/* 	task_cancel(turnOffTaskHandler); */
/* 	// Turn on the pixel and play the tone */
/* 	bb_matrix_set_pos(1, 1, LED_ON); */
/* 	bb_tone(440); */
/* 	// Store it in `th_turn_off` */
/* 	turnOffTaskHandler = task_create_timeout(turnOff, 100); */
/* } */

//
void upJustPressed(task_handle self) {
	upPressed = true;
}

void downJustPressed(task_handle self) {
	downPressed = true;
}

void leftJustPressed(task_handle self) {
	leftPressed = true;
}

void rightJustPressed(task_handle self) {
	rightPressed = true;
}

void upJustReleased(task_handle self) {
	upPressed = false;
}

void downJustReleased(task_handle self) {
	downPressed = false;
}

void leftJustReleased(task_handle self) {
	leftPressed = false;
}

void rightJustReleased(task_handle self) {
	rightPressed = false;
}
int8_t clamp(const int8_t value, const int8_t min, const int8_t max) {
	if (value < min) {
		return min;
	} else if (value > max) {
		return max;
	}
	return value;
}

void setSquareState(const uint8_t x, const uint8_t y, const int16_t beep, const led_state state) {
	boardState[x][y] = state;

	if (beep > 0) {
		bb_tone(beep);
	}
}

void updatePlayer() {
	setSquareState(playerLocation[0], playerLocation[1], -1, LED_OFF);
	playerLocation[0] += rightPressed - leftPressed;
	playerLocation[1] += downPressed - upPressed;
	playerLocation[0] = clamp(playerLocation[0], 0, 7);
	playerLocation[1] = clamp(playerLocation[1], 0, 7);
	setSquareState(playerLocation[0], playerLocation[1], -1, LED_ON);
}
void enableFlashLocation(task_handle self) {
	for (int i = 0; i < 10; i++) {
		int8_t* location = flashLocations[i];
		if (location[0] == -1) {
			break;
		}
		setSquareState(location[0], location[1], -1, LED_ON);
		debug_print("Enable flash location: %d, %d\n", location[0], location[1]);
	}
}

void toggleFlashLocations(task_handle self) {
	if (!showingData) {
		return;
	}

	debug_print("toggleFlashRan");
	for (int i = 0; i < 10; i++) {
		int8_t* location = flashLocations[i];
		if (location[0] == -1) {
			break;
		}
		led_state newState = boardState[location[0]][location[1]] == LED_OFF;
		setSquareState(location[0], location[1], -1, newState);
		debug_print("Toggling flash location: %d, %d\n", location[0], location[1]);

		flashTaskHandler = task_create_timeout(enableFlashLocation,
											   TARGET_FLASH_TIME_MS - TARGET_FLASH_TIME_ON_MS);
	}
}

void updateBoard(task_handle self) {
	if (!showingData) {
		return;
	}

	updatePlayer();

	bb_matrix_all_off();
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			bb_matrix_set_pos(i, j, boardState[i][j]);
		}
	}
}

void setup() {
	setupVars();
	debug_print("Starting up!\n");

	flashLocations[0] = targetLocation;

	showCountdown();

	task_create_interval(updateCountdown, 1000);

	task_create_interval(toggleFlashLocations, TARGET_FLASH_TIME_MS);
	task_create_interval(updateBoard, GAME_TICK_TIME_MS);
	task_create_event(upJustPressed, EVENT_PRESS_UP);
	task_create_event(downJustPressed, EVENT_PRESS_DOWN);
	task_create_event(leftJustPressed, EVENT_PRESS_LEFT);
	task_create_event(rightJustPressed, EVENT_PRESS_RIGHT);
	task_create_event(upJustReleased, EVENT_RELEASE_UP);
	task_create_event(downJustReleased, EVENT_RELEASE_DOWN);
	task_create_event(leftJustReleased, EVENT_RELEASE_LEFT);
	task_create_event(rightJustReleased, EVENT_RELEASE_RIGHT);

	// Create an event task that triggers when the select button is pressed
	/* task_create_event(turnOn, EVENT_PRESS_SELECT); */
	debug_print("Starting up!\n");
}
