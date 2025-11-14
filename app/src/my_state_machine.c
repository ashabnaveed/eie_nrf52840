/**
 * @file my_state_machine.c
 */

 #include <zephyr/smf.h>
 #include "LED.h"
 #include "my_state_machine.h"
 #include "BTN.h"

 #define ASCIILEN 16

  /* --------------------------------------------------------------------------------------------------------------
  Global Static Password Manager (Using Malloc for a variable array length as well)
 -------------------------------------------------------------------------------------------------------------- */

 static int user_input[ASCIILEN];

 void clear_input(){
  for (int i = 0; i < ASCIILEN; i++){
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
   uint16_t btn_press;
   uint16_t count;
   uint16_t input_count;
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
   smf_set_initial(SMF_CTX(&state_object), &state_machine_states[ENTRYA]);
 }

 int state_machine_run(){
   return smf_run_state(SMF_CTX(&state_object));
 }

 /* --------------------------------------------------------------------------------------------------------------
   Entry States
 -------------------------------------------------------------------------------------------------------------- */
 static void entrya_entry(void * o){

 }
 static void entryb_entry(void * o){

 }
 static void end_entry(void * o){

 }
 static void standby_entry(void * o){

 }

 /* --------------------------------------------------------------------------------------------------------------
   Run States
 -------------------------------------------------------------------------------------------------------------- */

 static enum smf_state_result entrya_run(void *o){

  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result entryb_run(void *o){

  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result end_run(void *o){

  return SMF_EVENT_HANDLED;
 }

 static enum smf_state_result standby_run(void *o){

  return SMF_EVENT_HANDLED;
 }



