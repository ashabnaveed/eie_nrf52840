/*
 * main.c
 */

 //ASK ABOUT HOW TO DEAL WITH ARRAY LIMITATIONS!!!!
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>
#include <BTN.h>
#include <LED.h>
#include <stdbool.h>

#define PASS_LEN 10
#define SLEEP_MS 10  
#define ENTRY_TIME_MS 3000

typedef enum {
  STATE_LOCKED,   // LED0 on, collect enter
  STATE_WAITING,
  STATE_ENTRY,
  STATE_BOOT   
} app_state_t;

static int password[PASS_LEN] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static int user_input[PASS_LEN];
static int input_index = 0;
static int password_index;

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
  if (input_index != password_index) return 0; //too many inputs 
  for (int i = 0; i < password_index; i++) {
    if ((user_input[i] != password[i])) return 0; // otherwise check each input that was stored
  }
  return 1;
} 

int main(void) {
  BTN_init();
  LED_init();
  int btn3count = 0;
  LED_set(LED3, LED_ON);
  app_state_t state = STATE_BOOT;
  reset_to_locked(); //reset
  int prev_btn = -1;  // edge detection
  int time_to_enter = k_uptime_get() + ENTRY_TIME_MS; //needed to ensure 3 seconds AFTER boot is used

  while (1) {
    int btn = get_pressed_button();
    int uptime_ms = k_uptime_get();   
  
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
            if (input_index < password_index) {
              user_input[input_index++] = btn; // RECORD DIGIT
              printk("Digit %d (%d/%d)\n", btn, input_index, password_index);
            } else {
              printk("Extra digit ignored\n");
            }
          }
        } 

        else if (state == STATE_ENTRY) {
          if (btn == 3) {
            btn3count++;
            printk("%d\n", btn3count);
          }

          if (btn3count == 0) {
            if (btn >= 0 && btn <= 2) {
              password[password_index++] = btn;
              printk("Digit %d stored for password length of %d\n", btn, password_index);
            }
          }
          
          if (btn3count == 1) {
            printk("Password stored, entering locked state\n");
            LED_set(LED3, LED_OFF);
            LED_set(LED2, LED_OFF);
            state = STATE_LOCKED;
          }
        }

        else if (state == STATE_WAITING) { 
          reset_to_locked();
          state = STATE_LOCKED;
        }

        else { //boot state
          if (uptime_ms < time_to_enter && btn == 3){
            LED_set(LED2, LED_ON); //added for a notifier of pass. select mode
            state = STATE_ENTRY;
          } else if (uptime_ms > time_to_enter) {
            LED_set(LED3, LED_OFF);
          }
        }

      }

      // remember last seen for edge detection
      prev_btn = btn;
    }
    k_msleep(SLEEP_MS);
  }
}
