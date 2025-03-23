#include "blackbox.h"

// Store a handle to the timeout task in case of super fast presses
task_handle th_turn_off;

const uint8_t countdown_seconds = 5;

uint32_t current_total_seconds;
bool showing_data = false;

void show_countdown() {
	bb_matrix_all_off();
	// Making sense of these formulas is left as an exercise to the reader
	int current_seconds = current_total_seconds % 60;
	int current_minutes = ((current_total_seconds - current_seconds) % 3600) / 60;
	int current_hours = (current_total_seconds - current_seconds - (current_minutes * 60)) / 3600;
	// Set slices
	bb_slice_set_int(33, 38, current_seconds);
}

void update_countdown(task_handle self) {
	current_total_seconds -= 1;
	if (showing_data == false) {
		show_countdown();
	}
	if (current_total_seconds == 0) {
		showing_data = true;
	}
}

// Stop showing a data array and display the countdown again
void stop_showing_data(task_handle self) {
	showing_data = false;
	bb_tone_off();
	show_countdown();
}

void show_data(const uint8_t* data, uint16_t frequency) {
	showing_data = true;
	bb_matrix_set_arr(data);
	bb_tone(frequency);
	task_create_timeout(stop_showing_data, 125);
}

void turn_off(task_handle self) {
	// Turn off the pixel and stop playing the tone
	bb_matrix_set_pos(1, 1, LED_OFF);
	bb_tone_off();
}

void turn_on(task_handle self) {
	if (!showing_data)
		return;
	// Cancel the previous timeout task in case the button is pressed super fast
	task_cancel(th_turn_off);
	// Turn on the pixel and play the tone
	bb_matrix_set_pos(1, 1, LED_ON);
	bb_tone(440);
	// Create a timeout task that triggers 100 milliseconds from now
	// Store it in `th_turn_off`
	th_turn_off = task_create_timeout(turn_off, 100);
}

void setup() {
	debug_print("Starting up!\n");
	current_total_seconds = countdown_seconds;
	show_countdown();

	task_create_interval(update_countdown, 1000);
	// Create an event task that triggers when the select button is pressed
	task_create_event(turn_on, EVENT_PRESS_SELECT);
}
