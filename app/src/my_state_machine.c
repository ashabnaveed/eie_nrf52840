/**
 * @file my_state_machine.c
 */

 #include <zephyr/smf.h>
 #include "LED.h"
 #include "my_state_machine.h"
 #include "BTN.h"

 #define PASSLEN 10

  /* --------------------------------------------------------------------------------------------------------------
  Global Static Password Manager (Using Malloc for a variable array length as well)
 -------------------------------------------------------------------------------------------------------------- */

 static int password[PASSLEN];
 static int user_input[PASSLEN];

 void initialize_password(){
  for (int i = 0; i < PASSLEN; i++){
    password[i] = -1;
  }
 }

 void clear_input(){
  for (int i = 0; i < PASSLEN; i++){
    user_input[i] = -1;
  }
 }

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
 static void lock_entry(void * o);
 static void input_entry(void * o);
 static void waiting_entry(void * o);
 static void boot_entry(void * o);

 // Using Run States to wait for state shifting
 static enum smf_state_result lock_run(void *o);
 static enum smf_state_result input_run(void *o);
 static enum smf_state_result waiting_run(void *o);
 static enum smf_state_result boot_run(void *o);

 /* --------------------------------------------------------------------------------------------------------------
   Names of States
 -------------------------------------------------------------------------------------------------------------- */
 enum {
  LOCKED,  
  WAITING,
  INPUT,
  BOOT   
}; 

 //Needed to monitor current state
 typedef struct {
   struct smf_ctx ctx;
   uint16_t btn_press;
   uint16_t count;
   uint16_t input_count;
   uint16_t user_input_count;
 } state_object_t;

 static state_object_t state_object; //creating state_object to monitor and change states

 int compare_passwords(){
  if (state_object.user_input_count != state_object.input_count) return -1;

  for (int i = 0; i < state_object.input_count; i++){
    if (user_input[i] != password[i]) return -1;
  }

  return 1;
 }

 /* --------------------------------------------------------------------------------------------------------------
   Define State Table
 -------------------------------------------------------------------------------------------------------------- */

 const struct smf_state state_machine_states[] = {
   [LOCKED] = SMF_CREATE_STATE(lock_entry, lock_run, NULL, NULL, NULL),
   [WAITING] = SMF_CREATE_STATE(waiting_entry, waiting_run, NULL, NULL, NULL),
   [INPUT] = SMF_CREATE_STATE(input_entry, input_run, NULL, NULL, NULL),
   [BOOT] = SMF_CREATE_STATE(boot_entry, boot_run, NULL, NULL, NULL)
 };

 /* --------------------------------------------------------------------------------------------------------------
   Initialize and Run
 -------------------------------------------------------------------------------------------------------------- */
 void state_machine_init(){
   smf_set_initial(SMF_CTX(&state_object), &state_machine_states[BOOT]);
 }

 int state_machine_run(){
   return smf_run_state(SMF_CTX(&state_object));
 }

 /* --------------------------------------------------------------------------------------------------------------
   Entry States
 -------------------------------------------------------------------------------------------------------------- */
static void boot_entry(void * o){
  initialize_password();
  state_object.count = 0;
  LED_set(LED0, LED_OFF);
  LED_set(LED1, LED_OFF);
  LED_set(LED2, LED_ON); 
  LED_set(LED3, LED_OFF); //turn on LED3 (LED4 on board)
}

 static void lock_entry(void * o){
  state_object.count = 0;
  state_object.user_input_count = 0;
  clear_input();
  LED_set(LED0, LED_ON);
  LED_set(LED1, LED_OFF);
  LED_set(LED2, LED_OFF); 
  LED_set(LED3, LED_OFF);
 }

 static void input_entry(void * o){
  state_object.count = 0;
  state_object.input_count = 0;
  LED_set(LED0, LED_OFF);
  LED_set(LED1, LED_OFF);
  LED_set(LED2, LED_ON); 
  LED_set(LED3, LED_OFF);
 }

 static void waiting_entry(void * o){
  state_object.count = 0;
  LED_set(LED0, LED_OFF);
  LED_set(LED1, LED_OFF);
  LED_set(LED2, LED_OFF); 
  LED_set(LED3, LED_OFF);
 }

 /* --------------------------------------------------------------------------------------------------------------
   Run States
 -------------------------------------------------------------------------------------------------------------- */

 static enum smf_state_result boot_run(void *o){
  int edge = button_press_edge();
  
  if (edge != -1 && state_object.count < 10000){ //changed input time to 10 seconds for convenience
    smf_set_state(SMF_CTX(&state_object), &state_machine_states[INPUT]);
  } else {
    state_object.count++;
    if (state_object.count > 10000) {
      smf_set_state(SMF_CTX(&state_object), &state_machine_states[LOCKED]);
    }
  }

  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result input_run(void *o){
  int edge = button_press_edge();

  if (edge != -1){
    if (edge != 4 && state_object.input_count < PASSLEN) {
      password[state_object.input_count++] = edge; 
    } else {
      smf_set_state(SMF_CTX(&state_object), &state_machine_states[LOCKED]);
    }
  }

  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result lock_run (void *o){
  int edge = button_press_edge();

  if (edge != -1){
    if (edge != 4 && state_object.user_input_count < PASSLEN) {
      user_input[state_object.user_input_count++] = edge; 
    } else {
      if (compare_passwords() == -1){
        clear_input();
        state_object.user_input_count = 0;
        printk("Inputted Password Incorrect");
      } else {
        smf_set_state(SMF_CTX(&state_object), &state_machine_states[WAITING]);
      }


    }
  }
  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result waiting_run (void *o){
  int edge = button_press_edge();
  if (edge != -1) {
    smf_set_state(SMF_CTX(&state_object), &state_machine_states[LOCKED]);
  }
  return SMF_EVENT_HANDLED;
 }



