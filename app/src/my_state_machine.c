/**
 * @file my_state_machine.c
 */

 #include <zephyr/smf.h>
 #include "LED.h"
 #include "my_state_machine.h"
 #include "BTN.h"

 #define ASCIILEN 16
 #define BTN01_MASK ((1 << 0) | (1 << 1)) /* for convenience */
 #define sleep_time 5

  /* --------------------------------------------------------------------------------------------------------------
  Global Static Password Manager (Using Malloc for a variable array length as well)
 -------------------------------------------------------------------------------------------------------------- */

 static int half_len = ASCIILEN / 2;
 static int user_input[ASCIILEN];
 static int64_t standby_hold_start = -1;

 void clear_input(int start){
  for (int i = start; i < half_len + start; i++){
    user_input[i] = -1;
  }
 }

 /* --------------------------------------------------------------------------------------------------------------
   Map Buttons to Return Values for Edge Detection
 -------------------------------------------------------------------------------------------------------------- */

 int button_press(){
  uint16_t mask = 0;

  if (BTN_is_pressed(BTN0)) mask |= (1 << 0);
  if (BTN_is_pressed(BTN1)) mask |= (1 << 1);
  if (BTN_is_pressed(BTN2)) mask |= (1 << 2);
  if (BTN_is_pressed(BTN3)) mask |= (1 << 3);

  return mask;
 }

 static int last_button = 0;

 static int button_press_edge(){
  int current = button_press();
  int edge = current & ~last_button;
  last_button = current;
  return edge; 
 }

 /* --------------------------------------------------------------------------------------------------------------
   Function Prototypes, utilizing Entry States 
 -------------------------------------------------------------------------------------------------------------- */

 //Using Entry States for LED Control
 static void entrya_entry(void * o);
 static void entryb_entry(void * o);
 static void end_entry(void * o);
 static void standby_entry(void * o);

 // Using Run States to wait for state shifting
 static enum smf_state_result entrya_run(void *o);
 static enum smf_state_result entryb_run(void *o);
 static enum smf_state_result end_run(void *o);
 static enum smf_state_result standby_run(void *o);

 /* --------------------------------------------------------------------------------------------------------------
   Names of States
 -------------------------------------------------------------------------------------------------------------- */
 enum {
  ENTRYA,  
  ENTRYB,
  END,
  STANDBY   
}; 

 //Needed to monitor current state
 typedef struct {
   struct smf_ctx ctx;
   uint16_t input_count;
   uint16_t last_state;
   uint16_t pwm;
 } state_object_t;

 static state_object_t state_object; //creating state_object to monitor and change states


 /* --------------------------------------------------------------------------------------------------------------
   Define State Table
 -------------------------------------------------------------------------------------------------------------- */

 const struct smf_state state_machine_states[] = {
   [ENTRYA] = SMF_CREATE_STATE(entrya_entry, entrya_run, NULL, NULL, NULL),
   [ENTRYB] = SMF_CREATE_STATE(entryb_entry, entryb_run, NULL, NULL, NULL),
   [END] = SMF_CREATE_STATE(end_entry, end_run, NULL, NULL, NULL),
   [STANDBY] = SMF_CREATE_STATE(standby_entry, standby_run, NULL, NULL, NULL)
 };

 /* --------------------------------------------------------------------------------------------------------------
   Initialize and Run
 -------------------------------------------------------------------------------------------------------------- */
 void state_machine_init(){
   state_object.last_state = ENTRYA;
   smf_set_initial(SMF_CTX(&state_object), &state_machine_states[ENTRYA]);
 }

 int state_machine_run(){
   return smf_run_state(SMF_CTX(&state_object));
 }

 /* --------------------------------------------------------------------------------------------------------------
   Entry States
 -------------------------------------------------------------------------------------------------------------- */
 static void entrya_entry(void * o){
  state_object.last_state = ENTRYA;
  state_object.input_count = 0;
  clear_input(0);
  LED_set(LED0, LED_OFF);
  LED_set(LED1, LED_OFF);
  LED_set(LED2, LED_OFF);
  LED_set(LED3, LED_ON);
  LED_blink(LED3, 1);
 }

 static void entryb_entry(void * o){
  state_object.last_state = ENTRYB;
  clear_input(8);
  LED_set(LED0, LED_OFF);
  LED_set(LED1, LED_OFF);
  LED_set(LED2, LED_OFF);
  LED_set(LED3, LED_ON);
  LED_blink(LED3, 4);
 }

 static void end_entry(void * o){
  state_object.last_state = END;
  state_object.input_count = 0;
  LED_set(LED0, LED_OFF);
  LED_set(LED1, LED_OFF);
  LED_set(LED2, LED_OFF);
  LED_set(LED3, LED_ON);
  LED_blink(LED3, 16);
 }

 static void standby_entry(void * o){
  state_object.pwm = 0;
  LED_set(LED0, LED_ON);
  LED_set(LED1, LED_ON);
  LED_set(LED2, LED_ON);
  LED_set(LED3, LED_ON);
 }

 /* --------------------------------------------------------------------------------------------------------------
   Run States
 -------------------------------------------------------------------------------------------------------------- */

 static enum smf_state_result entrya_run(void *o){
  int press = button_press();
  int64_t current_time = k_uptime_get();
  
  LED_set(LED0, LED_OFF);
  LED_set(LED1, LED_OFF);

  if ((press & BTN01_MASK) == BTN01_MASK){
    if (standby_hold_start < 0){
      standby_hold_start = current_time; //set current time to find diff
    } else if ((current_time - standby_hold_start >= 3000)){
      standby_hold_start = -1;
      smf_set_state(SMF_CTX(&state_object), &state_machine_states[STANDBY]);
    } 
  } else {
    standby_hold_start = -1;
  }

  int edge = button_press_edge();
  if (edge != 0){

    if ((edge & (1 << 0)) && (state_object.input_count < half_len)){
      LED_set(LED0, LED_ON);
      k_msleep(sleep_time); //added for visibility
      user_input[state_object.input_count++] = 0;
    }

    if ((edge & (1 << 1)) && (state_object.input_count < half_len)){
      LED_set(LED1, LED_ON);
      k_msleep(sleep_time); //added for visibility
      user_input[state_object.input_count++] = 1;
    }

    if (edge & (1 << 2)){
      clear_input(0);
      state_object.input_count = 0;
    }

    if (edge & (1 << 3)){
      smf_set_state(SMF_CTX(&state_object), &state_machine_states[ENTRYB]);
    }

  }


  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result entryb_run(void *o){
  int press = button_press();
  int64_t current_time = k_uptime_get();
  
  LED_set(LED0, LED_OFF);
  LED_set(LED1, LED_OFF);

  if ((press & BTN01_MASK) == BTN01_MASK){
    if (standby_hold_start < 0){
      standby_hold_start = current_time; //set current time to find diff
    } else if ((current_time - standby_hold_start >= 3000)){
      standby_hold_start = -1;
      smf_set_state(SMF_CTX(&state_object), &state_machine_states[STANDBY]);
    } 
  } else {
    standby_hold_start = -1;
  }


  int edge = button_press_edge();
  if (edge != 0){

    if ((edge & (1 << 0)) && (state_object.input_count < ASCIILEN)){
      LED_set(LED0, LED_ON);
      k_msleep(sleep_time); //added for visibility
      user_input[state_object.input_count++] = 0;
    }

    if ((edge & (1 << 1)) && (state_object.input_count < ASCIILEN)){
      LED_set(LED1, LED_ON);
      k_msleep(sleep_time); //added for visibility
      user_input[state_object.input_count++] = 1;
    }

    if (edge & (1 << 2)){
      clear_input(8);
      state_object.input_count = 8;
    }

    if (edge & (1 << 3)){
      smf_set_state(SMF_CTX(&state_object), &state_machine_states[END]);
    }

  }


  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result end_run(void *o){
  int press = button_press();
  int64_t current_time = k_uptime_get();

  if ((press & BTN01_MASK) == BTN01_MASK){
    if (standby_hold_start < 0){
      standby_hold_start = current_time; //set current time to find diff
    } else if ((current_time - standby_hold_start >= 3000)){
      standby_hold_start = -1;
      smf_set_state(SMF_CTX(&state_object), &state_machine_states[STANDBY]);
    } 
  } else {
    standby_hold_start = -1;
  }


  int edge = button_press_edge();
  
  if (edge != 0){
    if (edge & (1 << 2)){
      clear_input(0);
      clear_input(8);
      smf_set_state(SMF_CTX(&state_object), &state_machine_states[ENTRYA]);
    }

    if (edge & (1 << 3)){
      for (int i = 0; i < ASCIILEN; i++){
        if (user_input[i] == -1){
          printk("Error when entering ASCII code, resetting. Please re-enter the code correctly (8 bits & 8 bits)\n");
          smf_set_state(SMF_CTX(&state_object), &state_machine_states[ENTRYA]);
          return SMF_EVENT_HANDLED;
        }
      }

      uint16_t first = 0;
      uint16_t second = 0;

      for (int i = 0; i < 8; i++){
        first = (first << 1) | (user_input[i] & 1);
      }

      for (int i = 8; i < 16; i++){
        second = (second << 1) | (user_input[i] & 1);
      }
      printk("Characters %c, %c ", first, second);
    }
  }
  

  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result standby_run(void *o){

  int edge = button_press_edge();

  if (edge != 0){
    state_object.pwm = 0;
    smf_set_state(SMF_CTX(&state_object), &state_machine_states[state_object.last_state]);
    return SMF_EVENT_HANDLED;
  }

  static int pwm_direction = 1; //needs to persist between calls otherwise gets stuck at pwm 100

  state_object.pwm += pwm_direction * 5;

  if (state_object.pwm >= 100) {
      state_object.pwm = 100;
      pwm_direction = -1;
  } 
  else if (state_object.pwm <= 0) {
      state_object.pwm = 0;
      pwm_direction = 1;
  }

  LED_pwm(LED0, state_object.pwm);
  LED_pwm(LED1, state_object.pwm);
  LED_pwm(LED2, state_object.pwm);
  LED_pwm(LED3, state_object.pwm);

  k_msleep(sleep_time * 10);

  return SMF_EVENT_HANDLED;
 }



