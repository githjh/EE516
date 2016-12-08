/*
   	LED and Button Driver for Beagle Bone Black
	Copyright (C) 1984-2015 Core lab. <djshin.core.kaist.ac.kr>

*/

#include <linux/module.h>
#include <linux/kernel.h> /* printk */
#include <linux/interrupt.h> /* irq_request */
#include <linux/irq.h>
#include <asm/uaccess.h> /* copy_to_user */
#include <linux/fs.h> /* file_operations */
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/delay.h>   /* msleep */
#include <linux/timer.h>   /* kernel timer */
#include <linux/slab.h>	   /* kmalloc */

#include <linux/time.h>

#define NUM_LED 4
#define TIME_STEP  (1*HZ)

#define LED0_GPIO   53    /* USER LED 0     */
#define LED1_GPIO   54    /* USER LED 1     */
#define LED2_GPIO   55    /* USER LED 2     */
#define LED3_GPIO   56    /* USER LED 3     */
#define BUTTON_GPIO 72    /* USER BUTTON S2 */

struct timer_list my_timer;

/* LED_STATE: save led state whether it is on or off */
int LED_STATE[NUM_LED];

int toggle_state;


struct timespec falling_time;
struct timespec rising_time;

int arr_pos;
int arr_len;
char light_pattern[] ={0, 1, 2, 3, 2, 1};

void led_on(void){
	int i;

	for (i = 0; i< NUM_LED ; i ++){
		if (LED_STATE[i] == 1){
			gpio_set_value(LED0_GPIO + i, 1);
		}
	}
}

void led_off(void ){
	int i;
	
	for (i = 0; i< NUM_LED ; i ++){
		gpio_set_value(LED0_GPIO + i, 0);
	}

}

void my_timer_handler (unsigned long arg)
{
	printk("my_timer_handler \n");

	if(toggle_state == 0){
		toggle_state = 1;
		my_timer.expires = get_jiffies_64() + HZ;
		/* leds will be turned off for 1 second */
		led_off();
	}
	else{
		toggle_state = 0;
		my_timer.expires = get_jiffies_64() + HZ;
		/* leds will be turned on for 1 second */ 
		LED_STATE[light_pattern[arr_pos]] = 1;
		led_on();
		LED_STATE[light_pattern[arr_pos]] = 0;
		arr_pos ++;
		if (arr_pos == arr_len){
			arr_pos = 0;
		}
	}
	add_timer(&my_timer);
}


int my_timer_init(struct timer_list * timer){
	init_timer(timer);
	timer->expires = get_jiffies_64() + (HZ*8 /10);
	timer->data = 0;
	timer->function = my_timer_handler;
	toggle_state = 0;
	return 0;
}


static int __init bb_module_init(void)
{	
	int i;
	int ret_irq;
	printk("[EE516] Initializing BB module completed.\n");
	gpio_request(LED0_GPIO, "LED0_GPIO");
	gpio_request(LED1_GPIO, "LED0_GPI1");
	gpio_request(LED2_GPIO, "LED0_GPI2");
	gpio_request(LED3_GPIO, "LED0_GPI3");


	arr_len = sizeof(light_pattern);
	arr_pos = 0;

	for (i = 0 ; i< NUM_LED ; i++){
		LED_STATE[i] = 0;
		gpio_direction_output(LED0_GPIO + i, LED_STATE[i]);
	}

	my_timer_init(&my_timer);
	add_timer(&my_timer);

	return 0;
}

static void __exit bb_module_exit(void)
{		
	printk("[EE516] BB module exit.\n");

	gpio_set_value(LED0_GPIO, 0);
	gpio_set_value(LED1_GPIO, 0);
	gpio_set_value(LED2_GPIO, 0);
	gpio_set_value(LED3_GPIO, 0);

	gpio_free(LED0_GPIO);
	gpio_free(LED1_GPIO);
	gpio_free(LED2_GPIO);
	gpio_free(LED3_GPIO);

	del_timer(&my_timer);
}


MODULE_LICENSE("GPL");
module_init(bb_module_init);
module_exit(bb_module_exit);