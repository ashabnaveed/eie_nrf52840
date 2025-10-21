/*
 * main.c
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>
#include <BTN.h>
#include <LED.h>
#include <stdbool.h>

#define PASS_LEN 3
#define SLEEP_MS 10  

typedef enum {
  STATE_LOCKED,   // LED0 on, collect enter
  STATE_WAITING   // LED0 off, any button press resets to LOCKED
} app_state_t;

static int password[PASS_LEN] = {0, 2, 1};
static int user_input[PASS_LEN];
static int input_index = 0;

int get_pressed_button(void) {
    if (BTN_is_pressed(BTN0)) return 0;
    if (BTN_is_pressed(BTN1)) return 1;
    if (BTN_is_pressed(BTN2)) return 2;
    if (BTN_is_pressed(BTN3)) return 3;
    return -1;
} //correlates button press with integer returns

static void reset_to_locked(void) {
  input_index = 0;
  for (int i = 0; i < PASS_LEN; i++) user_input[i] = -1;
  LED_set(LED0, LED_ON);
} // override inputs with -1 to be fully false, set LED on

static int compare_password(void) {
  if (input_index != PASS_LEN) return 0; //too many inputs 
  for (int i = 0; i < PASS_LEN; i++) {
    if (user_input[i] != password[i]) return 0; // otherwise check each input that was stored
  }
  return 1;
} 

int main(void) {
  BTN_init();
  LED_init();

  app_state_t state = STATE_LOCKED; //set the state to locked
  reset_to_locked(); //reset

  int prev_btn = -1;  // edge detection

  while (1) {
    int btn = get_pressed_button();

    // edge detection
    if (btn != prev_btn) {
      // PRESS edge 
      if (btn != -1) { 
        if (state == STATE_LOCKED) {

          if (btn == 3) {
            // Enter and compare
            if (compare_password()) {
              printk("Correct!\n");
              LED_set(LED0, LED_OFF);
              state = STATE_WAITING; //change state to ensure next poll and input can reset
            } else {
              printk("Incorrect!\n");
              reset_to_locked();
            }
          } 
          
          else if (btn >= 0 && btn <= 2) {
            if (input_index < PASS_LEN) {
              user_input[input_index++] = btn; // RECORD DIGIT
              printk("Digit %d (%d/%d)\n", btn, input_index, PASS_LEN);
            } else {
              printk("Extra digit ignored\n");
            }
          }
        } 
        
        else { // waiting state
          reset_to_locked();
          state = STATE_LOCKED;
        }

      }

      // remember last seen for edge detection
      prev_btn = btn;
    }
    k_msleep(SLEEP_MS);
  }
}
