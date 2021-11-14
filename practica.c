/**
 * @file   practica.c
 * @author Aran Ventura
 * @date   14/11/2021
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>                 // Required for the GPIO functions
#include <linux/interrupt.h>            // Required for the IRQ code

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aran Ventura");
MODULE_DESCRIPTION("Some Buttons/LEDs test driver");
MODULE_VERSION("0.1");

static unsigned int gpioLED1 = 20;       // Primer LED
static unsigned int gpioLED2 = 16;       // Segundo LED
static unsigned int gpioButtonA = 26;   // Buto A (Encen el primer led)
static unsigned int gpioButtonB = 19;	// Button B (Apagar el primer led)	
static unsigned int gpioButtonC = 13;	// Button C (Encen el segon led)	
static unsigned int gpioButtonD = 21;	// Button C (Encen el segon led)	


static unsigned int irqNumberA;          ///< Used to share the IRQ number for the button A
static unsigned int irqNumberB;
static unsigned int irqNumberC;
static unsigned int irqNumberD;
static bool	    ledOn1 = 0;          ///< Is the state of the firs LED
static bool	    ledOn2 = 0;          ///< Is the state of the second LED
static unsigned int numberPresses = 0;


/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t handler_btnA(unsigned int irq, void *dev_id, struct pt_regs *regs);
static irq_handler_t handler_btnB(unsigned int irq, void *dev_id, struct pt_regs *regs);
static irq_handler_t handler_btnC(unsigned int irq, void *dev_id, struct pt_regs *regs);
static irq_handler_t handler_btnD(unsigned int irq, void *dev_id, struct pt_regs *regs);

static int __init ebbgpio_init(void){
   int result = 0;
   printk(KERN_INFO "GPIO_TEST: Initializing the GPIO_TEST LKM\n");
   // Is the GPIO a valid GPIO number (e.g., the BBB has 4x32 but not all available)
   if (!gpio_is_valid(gpioLED1)){
      printk(KERN_INFO "GPIO_TEST: invalid LED GPIO\n");
      return -ENODEV;
   }

   // Going to set up the LED. It is a GPIO in output mode and will be on by default
   ledOn1 = false;
   gpio_request(gpioLED1, "sysfs");          // gpioLED is hardcoded to 20, request it
   gpio_direction_output(gpioLED1, ledOn1);   // Set the gpio to be in output mode and on
// gpio_set_value(gpioLED1, ledOn1);          // Not required as set by line above (here for reference)
   gpio_export(gpioLED1, false);             // Causes gpio49 to appear in /sys/class/gpio
			                    // the bool argument prevents the direction from being changed
   ledOn2 = false;
   gpio_request(gpioLED2, "sysfs");          // gpioLED is hardcoded to 16, request it
   gpio_direction_output(gpioLED2, ledOn2);   // Set the gpio to be in output mode and on
// gpio_set_value(gpioLED2, ledOn2);          // Not required as set by line above (here for reference)
   gpio_export(gpioLED2, false);             // Causes gpio49 to appear in /sys/class/gpio
      
   gpio_request(gpioButtonA, "sysfs");       // Set up the gpioButton
   gpio_direction_input(gpioButtonA);        // Set the button GPIO to be an input
   gpio_set_debounce(gpioButtonA, 200);      // Debounce the button with a delay of 200ms
   gpio_export(gpioButtonA, false);          // Causes gpio115 to appear in /sys/class/gpio
			                    // the bool argument prevents the direction from being changed
   gpio_request(gpioButtonB, "sysfs");
   gpio_direction_input(gpioButtonB);
   gpio_set_debounce(gpioButtonB, 200);
   gpio_export(gpioButtonB, false);

   gpio_request(gpioButtonC, "sysfs");
   gpio_direction_input(gpioButtonC);
   gpio_set_debounce(gpioButtonC, 200);
   gpio_export(gpioButtonC, false);

   gpio_request(gpioButtonD, "sysfs");
   gpio_direction_input(gpioButtonD);
   gpio_set_debounce(gpioButtonD, 200);
   gpio_export(gpioButtonD, false);
 
   // GPIO numbers and IRQ numbers are not the same! This function performs the mapping forfor the button 
   irqNumberA = gpio_to_irq(gpioButtonA);
   irqNumberB = gpio_to_irq(gpioButtonB);
   irqNumberC = gpio_to_irq(gpioButtonC);
   irqNumberD = gpio_to_irq(gpioButtonD);
   
   // This next call requests an interrupt line for the button A
   result = request_irq(irqNumberA,             // The interrupt number requested
                        (irq_handler_t) handler_btnA, // The pointer to the handler function below
                        IRQF_TRIGGER_FALLING,   // Interrupt on rising edge (button press, not release)
                        "handler_btnA",    // Used in /proc/interrupts to identify the owner
                        NULL);                 // The *dev_id for shared interrupt lines, NULL is okay

   result = request_irq(irqNumberB,
		   	(irq_handler_t) handler_btnB,
			IRQF_TRIGGER_FALLING,
			"handler_btnB",
			NULL);

   result = request_irq(irqNumberC,
		   	(irq_handler_t) handler_btnC,
			IRQF_TRIGGER_FALLING,
			"handler_btnC",
			NULL);

   result = request_irq(irqNumberD,
		   	(irq_handler_t) handler_btnD,
			IRQF_TRIGGER_FALLING,
			"handler_btnD",
			NULL);

   
   printk(KERN_INFO "GPIO_TEST: The interrupt request result is: %d\n", result);
   return result;
}

static void __exit ebbgpio_exit(void){
   gpio_set_value(gpioLED1, 0);              // Turn the LED off, makes it clear the device was unloaded
   gpio_set_value(gpioLED2, 0);              // Turn the LED off, makes it clear the device was unloaded
   gpio_unexport(gpioLED1);                  // Unexport the LED GPIO
   gpio_unexport(gpioLED2);                  // Unexport the LED GPIO
	
   free_irq(irqNumberA, NULL);               // Free the IRQ number, no *dev_id required in this case
   free_irq(irqNumberB, NULL);
   free_irq(irqNumberC, NULL);
   free_irq(irqNumberD, NULL);

   gpio_unexport(gpioButtonA);               // Unexport the Button GPIO
   gpio_unexport(gpioButtonB);
   gpio_unexport(gpioButtonC);
   gpio_unexport(gpioButtonD);

   gpio_free(gpioLED1);   // Free the LED 1 GPIO
   gpio_free(gpioLED2);   // Free the LED 2 GPIO

   gpio_free(gpioButtonA);                   // Free the Button A GPIO
   gpio_free(gpioButtonB);		     // Free the button B GPIO
   gpio_free(gpioButtonC);		     // Free the button C GPIO
   gpio_free(gpioButtonD);		     // Free the button C GPIO
   printk(KERN_INFO "The buttons were pressed %d times\n", numberPresses);
   printk(KERN_INFO "GPIO_TEST: Goodbye from the LKM!\n");
}

static irq_handler_t handler_btnA(unsigned int irq, void *dev_id, struct pt_regs *regs){
   static char *envp[] = {"HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL};
   char * argv4[] = {"/bin/bash", "/home/pi/aso/practica/led1Open.sh", NULL};
   call_usermodehelper(argv4[1], argv4, envp, UMH_NO_WAIT);

   numberPresses++;
   printk(KERN_INFO "S'ha activat el buto A (Encenem el LED 1)");
   return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

static irq_handler_t handler_btnB(unsigned int irq, void *dev_id, struct pt_regs *regs){
   static char *envp[] = {"HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL};
   char * argv4[] = {"/bin/bash", "/home/pi/aso/practica/led1Close.sh", NULL};
   call_usermodehelper(argv4[1], argv4, envp, UMH_NO_WAIT);
   numberPresses++;
   printk(KERN_INFO "S'ha activat el buto B (Apaguem el LED 1)");
   return (irq_handler_t) IRQ_HANDLED;
}

static irq_handler_t handler_btnC(unsigned int irq, void *dev_id, struct pt_regs *regs){
   static char *envp[] = {"HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL};
   char * argv4[] = {"/bin/bash", "/home/pi/aso/practica/led2Open.sh", NULL};

   call_usermodehelper(argv4[1], argv4, envp, UMH_NO_WAIT);
   numberPresses++;
   printk(KERN_INFO "S'ha activat el buto C (Encenem el LED 2)");
   return (irq_handler_t) IRQ_HANDLED;
}

static irq_handler_t handler_btnD(unsigned int irq, void *dev_id, struct pt_regs *regs){
   static char *envp[] = {"HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL};
   char * argv4[] = {"/bin/bash", "/home/pi/aso/practica/led2Close.sh", NULL};

   call_usermodehelper(argv4[1], argv4, envp, UMH_NO_WAIT);
   numberPresses++;
   printk(KERN_INFO "S'ha activat el buto D (Apaguem el LED 2)");
   return (irq_handler_t) IRQ_HANDLED;
}

/// This next calls are  mandatory -- they identify the initialization function
/// and the cleanup function (as above).
module_init(ebbgpio_init);
module_exit(ebbgpio_exit);



