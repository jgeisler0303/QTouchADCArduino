This very simple sketch shows how to implement a so called *QTouch ADC* one wire touch button.

# Intro
*QTouch ADC* is a technology by Atmel Corporation offered in form of a free precompiled library but unfortunately  currently only for the *attiny 40*. QTouchADC touch measurement is extremely **fast, robust to EMI, reliable** touch detection, very very easy hardware setup and most importantly: only one pin per touch button or three per small slider or wheel. The pin needs to be an analog input pin. So, on the Uno you have *6 channels* and eight on a Mega (I believe), leaving all the digital pins for other cool stuff!!! This is in contrast to the standard QTouch where two (digital) pins are needed per channel.  
Another alternative is the well known *Capacitive Sensing Library* (http://playground.arduino.cc//Main/CapacitiveSensor?from=Main.CapSense) which uses one pin per button plus one extra pin that can be used for all buttons together. The hardware setup for this method is more complicated and I didn't have much luck with the accuracy of the measurements.  
So, in summary, QTouch ADC IMHO is very cool!

# Theory
I took inspiration and hints on the working principle and programming techniques from these pages:  
http://www.mikrocontroller.net/topic/260841  
http://tuomasnylund.fi/drupal6/content/capacitive-touch-sensing-avr-and-single-adc-pin  
http://www.atmel.com/Images/doc8497.pdf  
But it basically follows the code given here:  
http://www.mikrocontroller.net/topic/156809#2975291

At first I thought it would be possible to charge/discharge the sample&hold capacitor of the ADC unit via an internal reference. But then I realized that this is not done by the original QTouchADC library either: Application Note AVR3001 states that if only one touch button is needed it needs a "partner" adjacent pin that cannot be used for anything else. This pin is then used to provide the GND and Vcc input to the s&h cap. When more than one touch channels are used any channel that is not currently being probed can provide this function.

# Hardware
To make the code work you will have to solder a two cent coin to a 1k resistor. The other end of the resistor goes into Arduino port A0. You can watch the raw touch values at the serial monitor or make them visible with an LED connected to pin 9. Make sure the LED has an appropriate resistor to limit the current (resistance= (5V - LED_voltage)/LED_current). This basic setup can be seen in this video: http://youtu.be/-30wSuzNkvg

# Code
The working principle and programming techniques are explained in the comments in the source code. The code is a simple sketch (no library yet, sorry). For efficiency it makes direct use of atmega registers and ports. So, it's not directly portable. But porting, changing or extending it should be no big problem as the principle is very very simple indeed. I think the ADC registers and constants are the same for every atmega used in Arduino boards. Only the digital port C associated  with the ADC ports might need to be changed.

