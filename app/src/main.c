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

#define SLEEP_MS 500

int main(void) {

  int count = 0b0000;

  if (0 > BTN_init()) {
    return 0;
  }

  if (0 > LED_init()) {
    return 0;
  }

  while(1) {

  if (BTN_is_pressed(BTN0)) {
    count++;
    printk("%d", count);
    k_msleep(SLEEP_MS);
  }
 
  if (count > 15) {
    count = 0;
  }

  if (count & 0b0001) {
    LED_toggle(LED0);
  } else LED_set(LED0, LED_OFF);

  if (count & 0b0010) {
    LED_toggle(LED1);
  } else LED_set(LED1, LED_OFF);

  if (count & 0b0100) {
    LED_toggle(LED2);
  } else LED_set(LED2, LED_OFF);

  if (count & 0b1000) {
    LED_toggle(LED3);
  } else LED_set(LED3, LED_OFF);


  /* for (int i = 0; i < 4; i++) {
    if (count & (1 << i)) {}
  } */ //COULD ALSO USE THIS, SHIFTS BITS BY i FOR COMPARISON (i.e. 0001, 0010, 01000, 1000). Compiled similarly to above.

 }

	return 0;
}
