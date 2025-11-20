/**
 * @file my_state_machine.c
 */

#include <zephyr/smf.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "LED.h"
#include "my_state_machine.h"
#include "BTN.h"

#define ASCIILEN 512
#define BTN01_MASK ((1 << 0) | (1 << 1))
#define sleep_time 5

static int half_len = 8;   // fixed 8 bits per ASCII character
static int user_input[ASCIILEN];
static int64_t standby_hold_start = -1;

static void clear_input(int start){
    printk("Clearing 8-bit block starting at index %d\n", start);
    for (int i = start; i < start + half_len && i < ASCIILEN; i++){
        user_input[i] = -1;
    }
}

static void clear_all_input(void){
    printk("Clearing ALL input (512 bits)\n");
    for (int s = 0; s < ASCIILEN; s++){
        user_input[s] = -1;
    }
}

/* -------- Button Logic -------- */
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

/* -------- State Prototypes -------- */
static void entrya_entry(void * o);
static void entryb_entry(void * o);
static void end_entry(void * o);
static void standby_entry(void * o);

static enum smf_state_result entrya_run(void *o);
static enum smf_state_result entryb_run(void *o);
static enum smf_state_result end_run(void *o);
static enum smf_state_result standby_run(void *o);

/* -------- State Enum -------- */
enum {
    ENTRYA,
    ENTRYB,
    END,
    STANDBY
};

typedef struct {
    struct smf_ctx ctx;
    uint16_t input_count;
    uint16_t last_state;
    uint16_t pwm;
} state_object_t;

static state_object_t state_object;

const struct smf_state state_machine_states[] = {
    [ENTRYA]  = SMF_CREATE_STATE(entrya_entry, entrya_run, NULL, NULL, NULL),
    [ENTRYB]  = SMF_CREATE_STATE(entryb_entry, entryb_run, NULL, NULL, NULL),
    [END]     = SMF_CREATE_STATE(end_entry,  end_run, NULL, NULL, NULL),
    [STANDBY] = SMF_CREATE_STATE(standby_entry, standby_run, NULL, NULL, NULL)
};

/* -------- Init -------- */
void state_machine_init(){
    printk("Initializing FSM, entering ENTRYA...\n");
    state_object.last_state = ENTRYA;
    state_object.input_count = 0;
    clear_all_input();

    smf_set_initial(SMF_CTX(&state_object), &state_machine_states[ENTRYA]);
}

int state_machine_run(){
    return smf_run_state(SMF_CTX(&state_object));
}

/* -------- Standby Hold -------- */
static void handle_standby_hold(int press){
    int64_t now = k_uptime_get();
    if ((press & BTN01_MASK) == BTN01_MASK){
        if (standby_hold_start < 0){
            printk("BTN0+BTN1 pressed together, starting standby timer\n");
            standby_hold_start = now;
        } 
        else if (now - standby_hold_start >= 3000){
            printk("Entering STANDBY state\n");
            standby_hold_start = -1;
            smf_set_state(SMF_CTX(&state_object), &state_machine_states[STANDBY]);
        }
    } else {
        standby_hold_start = -1;
    }
}

/* --------------------------------------------------------------
   ENTRY A
-------------------------------------------------------------- */

static void entrya_entry(void * o){
    printk("\n=== ENTERING ENTRYA (bit entry state) ===\n");
    state_object.last_state = ENTRYA;
    LED_set(LED0, LED_OFF);
    LED_set(LED1, LED_OFF);
    LED_set(LED2, LED_OFF);
    LED_set(LED3, LED_ON);
    LED_blink(LED3, 1);
}

static enum smf_state_result entrya_run(void *o){
    int press = button_press();
    handle_standby_hold(press);

    int edge = button_press_edge();
    if (!edge) return SMF_EVENT_HANDLED;

    if ((edge & (1 << 0)) && state_object.input_count < ASCIILEN){
        printk("ENTRYA: Input bit 0 at index %u\n", state_object.input_count);
        user_input[state_object.input_count++] = 0;
        LED_set(LED0, LED_ON);
        k_msleep(sleep_time);
        LED_set(LED0, LED_OFF);
    }

    if ((edge & (1 << 1)) && state_object.input_count < ASCIILEN){
        printk("ENTRYA: Input bit 1 at index %u\n", state_object.input_count);
        user_input[state_object.input_count++] = 1;
        LED_set(LED1, LED_ON);
        k_msleep(sleep_time);
        LED_set(LED1, LED_OFF);
    }

    if (edge & (1 << 2)){
        int block_start = 0;
        if (state_object.input_count > 0)
            block_start = ((state_object.input_count - 1) / 8) * 8;

        printk("ENTRYA: BTN2 pressed, clearing 8-bit block starting at %d\n", block_start);
        clear_input(block_start);
        state_object.input_count = block_start;
    }

    if (edge & (1 << 3)){
        printk("ENTRYA: BTN3 pressed -> switching to ENTRYB\n");
        smf_set_state(SMF_CTX(&state_object), &state_machine_states[ENTRYB]);
    }

    return SMF_EVENT_HANDLED;
}

/* --------------------------------------------------------------
   ENTRY B
-------------------------------------------------------------- */

static void entryb_entry(void * o){
    printk("\n=== ENTERING ENTRYB (control after a character) ===\n");
    state_object.last_state = ENTRYB;
    LED_set(LED0, LED_OFF);
    LED_set(LED1, LED_OFF);
    LED_set(LED2, LED_OFF);
    LED_set(LED3, LED_ON);
    LED_blink(LED3, 4);
}

static enum smf_state_result entryb_run(void *o){
    int press = button_press();
    handle_standby_hold(press);

    int edge = button_press_edge();
    if (!edge) return SMF_EVENT_HANDLED;

    if ((edge & (1 << 0)) && state_object.input_count < ASCIILEN){
        printk("ENTRYB: Input bit 0 at index %u (returning to ENTRYA)\n", state_object.input_count);
        user_input[state_object.input_count++] = 0;
        LED_set(LED0, LED_ON);
        k_msleep(sleep_time);
        LED_set(LED0, LED_OFF);
        smf_set_state(SMF_CTX(&state_object), &state_machine_states[ENTRYA]);
        return SMF_EVENT_HANDLED;
    }

    if ((edge & (1 << 1)) && state_object.input_count < ASCIILEN){
        printk("ENTRYB: Input bit 1 at index %u (returning to ENTRYA)\n", state_object.input_count);
        user_input[state_object.input_count++] = 1;
        LED_set(LED1, LED_ON);
        k_msleep(sleep_time);
        LED_set(LED1, LED_OFF);
        smf_set_state(SMF_CTX(&state_object), &state_machine_states[ENTRYA]);
        return SMF_EVENT_HANDLED;
    }

    if (edge & (1 << 2)){
        printk("ENTRYB: BTN2 pressed -> clearing ALL input and going to ENTRYA\n");
        clear_all_input();
        state_object.input_count = 0;
        smf_set_state(SMF_CTX(&state_object), &state_machine_states[ENTRYA]);
        return SMF_EVENT_HANDLED;
    }

    if (edge & (1 << 3)){
        printk("ENTRYB: BTN3 pressed -> going to END state\n");
        smf_set_state(SMF_CTX(&state_object), &state_machine_states[END]);
    }

    return SMF_EVENT_HANDLED;
}

/* --------------------------------------------------------------
   END
-------------------------------------------------------------- */

static void end_entry(void * o){
    printk("\n=== ENTERING END (final print) ===\n");
    state_object.last_state = END;
    LED_set(LED0, LED_OFF);
    LED_set(LED1, LED_OFF);
    LED_set(LED2, LED_OFF);
    LED_set(LED3, LED_ON);
    LED_blink(LED3, 16);
}

static enum smf_state_result end_run(void *o){
    int press = button_press();
    handle_standby_hold(press);

    int edge = button_press_edge();
    if (!edge) return SMF_EVENT_HANDLED;

    if (edge & (1 << 2)){
        printk("END: BTN2 pressed -> clearing ALL and returning to ENTRYA\n");
        clear_all_input();
        state_object.input_count = 0;
        smf_set_state(SMF_CTX(&state_object), &state_machine_states[ENTRYA]);
        return SMF_EVENT_HANDLED;
    }

    if (edge & (1 << 3)){
        printk("END: BTN3 pressed -> printing characters\n");

        if (state_object.input_count % 8 != 0){
            printk("ERROR: bit count %u is not a multiple of 8\n", state_object.input_count);
            smf_set_state(SMF_CTX(&state_object), &state_machine_states[ENTRYA]);
            return SMF_EVENT_HANDLED;
        }

        printk("Total bits: %u -> %u bytes\n",
               state_object.input_count,
               state_object.input_count / 8);

        char out[ASCIILEN/8 + 1];
        int chars = state_object.input_count / 8;

        for (int c = 0; c < chars; c++){
            int start = c * 8;
            uint8_t value = 0;

            for (int b = 0; b < 8; b++)
                value = (value << 1) | (user_input[start + b] & 1);

            out[c] = value;
            printk("Byte[%d] = 0x%02X ('%c')\n", c, value, value);
        }

        out[chars] = '\0';
        printk("FINAL STRING: \"%s\"\n", out);
    }

    return SMF_EVENT_HANDLED;
}

/* --------------------------------------------------------------
   STANDBY
-------------------------------------------------------------- */

static void standby_entry(void * o){
    printk("\n=== ENTERING STANDBY (PWM breathing) ===\n");
    state_object.pwm = 0;
    LED_set(LED0, LED_ON);
    LED_set(LED1, LED_ON);
    LED_set(LED2, LED_ON);
    LED_set(LED3, LED_ON);
}

static enum smf_state_result standby_run(void *o){

    int edge = button_press_edge();
    if (edge){
        printk("STANDBY: button pressed -> returning to last state (%u)\n",
               state_object.last_state);
        state_object.pwm = 0;
        smf_set_state(SMF_CTX(&state_object),
                      &state_machine_states[state_object.last_state]);
        return SMF_EVENT_HANDLED;
    }

    static int dir = 1;
    state_object.pwm += dir * 5;

    if (state_object.pwm >= 100){
        dir = -1;
        state_object.pwm = 100;
    }
    else if (state_object.pwm <= 0){
        dir = 1;
        state_object.pwm = 0;
    }

    LED_pwm(LED0, state_object.pwm);
    LED_pwm(LED1, state_object.pwm);
    LED_pwm(LED2, state_object.pwm);
    LED_pwm(LED3, state_object.pwm);

    k_msleep(50);
    return SMF_EVENT_HANDLED;
}
