/**
 * @file my_state_machine.c
 */

 #include <zephyr/smf.h>
 #include "LED.h"
 #include "my_state_machine.h"
 #include "BTN.h"

 /* --------------------------------------------------------------------------------------------------------------
   Map Buttons to Return Values for Edge Detection
 -------------------------------------------------------------------------------------------------------------- */

 int button_press(){
  if (BTN_is_pressed(BTN0)) return 1;
  if (BTN_is_pressed(BTN1)) return 2;
  if (BTN_is_pressed(BTN2)) return 3;
  if (BTN_is_pressed(BTN3)) return 4;
  return -1;
 }

 static int last_button = -1;

 static int button_press_edge(){
  int current = button_press();
  int edge = -1;

  if (current != -1 && last_button == -1){
    edge = current;
  }

  last_button = current;
  return edge; //1-4 on new press, -1 on no press
 }

 /* --------------------------------------------------------------------------------------------------------------
   Function Prototypes, utilizing Entry States 
 -------------------------------------------------------------------------------------------------------------- */

 //Using Entry States for LED Control
 static void s0_state_entry(void * o);
 static void s1_state_entry(void * o);
 static void s2_state_entry(void * o);
 static void s3_state_entry(void * o);
 static void s4_state_entry(void * o);

 // Using Run States to wait for button presses to shift between states as per the diagaram
 static enum smf_state_result s0_state_run(void *o);
 static enum smf_state_result s1_state_run(void *o);
 static enum smf_state_result s2_state_run(void *o);
 static enum smf_state_result s3_state_run(void *o);
 static enum smf_state_result s4_state_run(void *o);

 /* --------------------------------------------------------------------------------------------------------------
   Names of States
 -------------------------------------------------------------------------------------------------------------- */
 enum state_machine_states{
    S0,
    S1,
    S2,
    S3,
    S4
 };

 //Needed to monitor current state
 typedef struct {
   struct smf_ctx ctx;
   uint16_t btn_press;
   uint16_t count;
 } state_object_t;

 static state_object_t state_object;

 /* --------------------------------------------------------------------------------------------------------------
   Define State Table
 -------------------------------------------------------------------------------------------------------------- */

 const struct smf_state state_machine_states[] = {
   [S0] = SMF_CREATE_STATE(s0_state_entry, s0_state_run, NULL, NULL, NULL),
   [S1] = SMF_CREATE_STATE(s1_state_entry, s1_state_run, NULL, NULL, NULL),
   [S2] = SMF_CREATE_STATE(s2_state_entry, s2_state_run, NULL, NULL, NULL),
   [S3] = SMF_CREATE_STATE(s3_state_entry, s3_state_run, NULL, NULL, NULL),
   [S4] = SMF_CREATE_STATE(s4_state_entry, s4_state_run, NULL, NULL, NULL)
 };

 /* --------------------------------------------------------------------------------------------------------------
   Initialize and Run
 -------------------------------------------------------------------------------------------------------------- */
 void state_machine_init(){
   smf_set_initial(SMF_CTX(&state_object), &state_machine_states[S0]);
 }

 int state_machine_run(){
   return smf_run_state(SMF_CTX(&state_object));
 }

 /* --------------------------------------------------------------------------------------------------------------
   Entry States
 -------------------------------------------------------------------------------------------------------------- */

 //WHEN ENTERING THE STATE, TURN ON OR OFF THE CORRECT LEDs (if an LED is meant to blink, start with it on)
 static void s0_state_entry(void * o){
   state_object.count = 0;
   LED_set(LED0, LED_OFF);
   LED_set(LED1, LED_OFF);
   LED_set(LED2, LED_OFF);
   LED_set(LED3, LED_OFF);
 }

 static void s1_state_entry(void * o){
   state_object.count = 0;
   LED_set(LED0, LED_ON);
   LED_set(LED1, LED_OFF);
   LED_set(LED2, LED_OFF);
   LED_set(LED3, LED_OFF);
 }

 static void s2_state_entry(void * o){
   state_object.count = 0;
   LED_set(LED0, LED_ON);
   LED_set(LED1, LED_OFF);
   LED_set(LED2, LED_ON);
   LED_set(LED3, LED_OFF);
 }

 static void s3_state_entry(void * o){
   state_object.count = 0;
   LED_set(LED0, LED_OFF);
   LED_set(LED1, LED_ON);
   LED_set(LED2, LED_OFF);
   LED_set(LED3, LED_ON);
 }

 static void s4_state_entry(void * o){
   state_object.count = 0;
   LED_set(LED0, LED_ON);
   LED_set(LED1, LED_ON);
   LED_set(LED2, LED_ON);
   LED_set(LED3, LED_ON);
 }

 /* --------------------------------------------------------------------------------------------------------------
   Run States
 -------------------------------------------------------------------------------------------------------------- */

 static enum smf_state_result s0_state_run(void *o){
  int edge = button_press_edge();
  //switch states
  if (edge == 1) {
    smf_set_state(SMF_CTX(&state_object), &state_machine_states[S1]);
  }
  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result s1_state_run(void *o){
  int edge = button_press_edge();
  if (edge == 4) smf_set_state(SMF_CTX(&state_object), &state_machine_states[S0]);
  if (edge == 3) smf_set_state(SMF_CTX(&state_object), &state_machine_states[S4]);
  if (edge == 2) smf_set_state(SMF_CTX(&state_object), &state_machine_states[S2]);

  if (edge == -1){
    if (state_object.count > 2000){
      LED_toggle(LED0);
      state_object.count = 0;
    } else {
      state_object.count++;
    }
  }
  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result s2_state_run(void *o){
  int edge = button_press_edge();
  if (edge == 4) smf_set_state(SMF_CTX(&state_object), &state_machine_states[S0]);

  if (edge == -1){
    if (state_object.count > 1000){
      state_object.count = 0;
      smf_set_state(SMF_CTX(&state_object), &state_machine_states[S3]);
    } else {
      state_object.count++;
    }
  }

  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result s3_state_run(void *o){
  int edge = button_press_edge();
  if (edge == 4) smf_set_state(SMF_CTX(&state_object), &state_machine_states[S0]);

  if (edge == -1){
    if (state_object.count > 2000){
      state_object.count = 0;
      smf_set_state(SMF_CTX(&state_object), &state_machine_states[S2]);
    } else {
      state_object.count++;
    }
  }
  
  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result s4_state_run(void *o){
  int edge = button_press_edge();
  if (edge == 4) smf_set_state(SMF_CTX(&state_object), &state_machine_states[S0]);

  if (edge == -1){
    if (state_object.count > 8000) {
      LED_toggle(LED0);
      LED_toggle(LED1);
      LED_toggle(LED2);
      LED_toggle(LED3);
      state_object.count = 0;
    } else {
      state_object.count++;
    }
  }

  return SMF_EVENT_HANDLED;
 }

