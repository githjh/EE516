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
int button_state;
int button_irt;

struct timespec falling_time;
struct timespec rising_time;


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
		my_timer.expires = get_jiffies_64() + HZ *2 /10;
		/* leds will be turned off for 0.2 second */
		led_off();
	}
	else{
		toggle_state = 0;
		my_timer.expires = get_jiffies_64() + HZ *8 / 10;
		/* leds will be turned on for 0.8 second */ 
		led_on();
	}
	add_timer(&my_timer);
}

irqreturn_t my_butten_handler(int irq, void *dev_id, struct pt_regs *pegs){
	
	/* falling handler */
	if(button_state == 0){

		int i;
		printk("my_butten_falling_handler\n");
		falling_time = CURRENT_TIME;
		LED_STATE[0] ++;
		for (i = 0 ; i < NUM_LED; i ++){
			if(LED_STATE[i] >= 2)
			{
				LED_STATE[i] -= 2;
				if(i+1 < NUM_LED){
					LED_STATE[i+1] += 1;
				}
			}
		}
		button_state = 1;
	}
	/* rising handler : check time difference between falling time and rising time*/
	else{
		struct timespec diff_time;

		printk("my_butten_rising_handler\n");
		rising_time = CURRENT_TIME;
		diff_time = timespec_sub(rising_time, falling_time);
		printk ("time diff tv_sec: %ld , tv_nsec : %ld\n", diff_time.tv_sec, diff_time.tv_nsec);
		button_state = 0;
		if(diff_time.tv_sec >= 1){
			int i;
			for(i = 0; i< NUM_LED; i ++){
				LED_STATE[i] = 0;
			}

		}
	}
	return IRQ_HANDLED;
	
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

	gpio_request(BUTTON_GPIO,"BUTTON");

	for (i = 0 ; i< NUM_LED ; i++){
		LED_STATE[i] = 0;
		gpio_direction_output(LED0_GPIO + i, LED_STATE[i]);
	}

	gpio_direction_input(BUTTON_GPIO);
	button_state = 0;
	button_irt = gpio_to_irq(BUTTON_GPIO);
	ret_irq = request_irq(button_irt, my_butten_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "Button", NULL);

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

	gpio_free(BUTTON_GPIO);
	free_irq(button_irt, NULL);
}


MODULE_LICENSE("GPL");
module_init(bb_module_init);
module_exit(bb_module_exit);