#include <linux/kernel.h>

#include <linux/platform_device.h>

#include <linux/gpio.h>

#include <linux/of_platform.h>

#include <linux/of_gpio.h>

#include <linux/module.h>

#include <linux/irq.h>

#include <linux/interrupt.h>

#define MIOBASE 906

#define EMIOBASE 960

//#include <linux/gpio/driver.h>

/*struct gpio{
	unsigned gpio;
	unsigned long flags;
	const char *label;
}*/


//INIT TYPE define in gpio.h 
static struct gpio leds_gpios[] = {
	{EMIOBASE, GPIOF_OUT_INIT_LOW, "led0"},
	{EMIOBASE+1, GPIOF_OUT_INIT_LOW, "led1"},
	{EMIOBASE+2, GPIOF_OUT_INIT_LOW, "led2"},
	{EMIOBASE+3, GPIOF_OUT_INIT_LOW, "led3"},
	{EMIOBASE+4, GPIOF_OUT_INIT_LOW, "led4"},
	{EMIOBASE+5, GPIOF_OUT_INIT_LOW, "led5"},
	{EMIOBASE+6, GPIOF_OUT_INIT_LOW, "led6"},
	{EMIOBASE+7, GPIOF_OUT_INIT_LOW, "led7"},
};

static struct gpio sws_gpios[] = {
	{EMIOBASE+8, GPIOF_IN, "sw0"},
	{EMIOBASE+9, GPIOF_IN, "sw1"},
	{EMIOBASE+10, GPIOF_IN, "sw2"},
	{EMIOBASE+11, GPIOF_IN, "sw3"},
	{EMIOBASE+12, GPIOF_IN, "sw4"},
	{EMIOBASE+13, GPIOF_IN, "sw5"},
	{EMIOBASE+14, GPIOF_IN, "sw6"},
	{EMIOBASE+15, GPIOF_IN, "sw7"},
};

static const struct of_device_id test_match[] = {

	{ .compatible = "test", },

	{},

};

int sw_gpio_irq[8];

irqreturn_t hanlder(int irq, void* unknown)

{

	int i;
	printk(KERN_EMERG "irq:%d in hanlder\n", irq);

	for(i=0; i<ARRAY_SIZE(sw_gpio_irq); i++){
		if(sw_gpio_irq[i] == irq)
			break;
	}
	int value = gpio_get_value(sws_gpios[i].gpio);
	gpio_set_value(leds_gpios[i].gpio, value);

return 0;

}

static int test_probe(struct platform_device *pdev)

{
	int i, ret;
	printk(KERN_EMERG "in test_probe\n"); 
	//gpio.h
	//extern int gpio_request_array(const struct gpio *array, size_t num);
	ret = gpio_request_array(leds_gpios, ARRAY_SIZE(leds_gpios)); //request led gpio array

	if(ret < 0){
	    printk("led:gpio_request_array FAILED!\n");
	    return -1;
	}

	// for(i=0; i<ARRAY_SIZE(leds_gpios); i++){
	// 	gpio_direction_output(leds_gpios[i].gpio, 1);
	// } //debug

	ret = gpio_request_array(sws_gpios, ARRAY_SIZE(sws_gpios)); //request led gpio array

	if(ret < 0){
	    printk("sws:gpio_request_array FAILED!\n");
	    return -1;
	}

	for(i=0; i<ARRAY_SIZE(sws_gpios); i++){
		int irq_num = gpio_to_irq(sws_gpios[i].gpio);
		ret = request_irq(irq_num, hanlder, NULL, NULL, NULL);
		if (ret < 0)
	    	return -1; 
	    sw_gpio_irq[i] = irq_num;
		printk(KERN_EMERG "gpio_to_irq sw%d:%d\n", i, irq_num);
		irq_set_irq_type(irq_num, IRQ_TYPE_EDGE_BOTH);
	}

	return 0;

}

static struct platform_driver test_driver = {

	.probe		=test_probe,

	.driver		= {

		.name	= "test",

		.of_match_table = test_match,

	},

};
module_platform_driver(test_driver);
