unsigned int adc0; // temp var
unsigned int adc1, adc2; // store the avarage of the charge resp. discharge measurement
int probe_val; // store the resulting touch measurement

// ADC constants
#define ADMUX_MASK  0b00001111 // mask the mux bits in the ADMUX register
#define MUX_GND 0b00001111 // mux value for connecting the ADC unit internally to GND
#define MUX_REF_VCC 0b01000000 // value to set the ADC reference to Vcc

const uint16_t ledFadeTable[32] = {0, 1, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 9, 10, 12, 15, 17, 21, 25, 30, 36, 43, 51, 61, 73, 87, 104, 125, 149, 178, 213, 255}; // this is an exponential series to model the perception of the LED brightness by the human eye

#define TPIN1 0
#define TPIN1_MASK 0x01
#define TPIN2 1 // this is currently only used as a supply of Vcc to charge the s&h cap
#define TPIN2_MASK 0x02

#define CHARGE_DELAY  5 // time it takes for the capacitor to get charged/discharged in microseconds
#define TRANSFER_DELAY  5 // time it takes for the capacitors to exchange charge
#define TOUCH_VALUE_BASELINE -165 // this is the value my setup measures when the probe is not touched. For your setup this might be different. In order for the LED to fade correctly, you will have to adjust this value
#define TOUCH_VALUE_SCALE 5 // this is also used for the LED fading. The value should be chosen such that the value measured when the probe is fully touched minus TOUCH_VALUE_BASELINE is scaled to 31, e.g. untouched_val= 333, touched_val= 488, difference= 155, divide by 5 to get 31.

void setup() {
  Serial.begin(9600); // standard serial setup
 
  // prepare the ADC unit for one-shot measurements
  // see the atmega328 datasheet for explanations of the registers and values
  ADMUX = 0b01000000; // Vcc as voltage reference (bits76), right adjustment (bit5), use ADC0 as input (bits3210)
  ADCSRA = 0b11000100; // enable ADC (bit7), initialize ADC (bit6), no autotrigger (bit5), don't clear int-flag  (bit4), no interrupt (bit3), clock div by 16@16Mhz=1MHz (bit210) ADC should run at 50kHz to 200kHz, 1MHz gives decreased resolution
  ADCSRB = 0b00000000; // autotrigger source free running (bits210) doesn't apply
  while(ADCSRA & (1<<ADSC)){  } // wait for first conversion to complete
  adc0 = ADC; // not sure if the value of the conversion has to be read if not needed
}
 
 
void loop() {
  // 16 measurements are taken an d averaged to improve noise immunity
  for (int i=0; i<4; i++) {
    // first measurement: charge touch probe, discharge ADC s&h cap, connect the two, measure the volage
    // charge sensor, discharge s&h cap
    DDRC|= TPIN1_MASK | TPIN2_MASK; // config pins as push-pull output
    PORTC= (PORTC & ~TPIN2_MASK) | TPIN1_MASK; // set PINC0 high to charge the touch probe cap and PINC1 low to discharge s&h cap
    
    // discharge s&h cap by connecting it to PINC1
    ADMUX = MUX_REF_VCC | TPIN2; // select PINC1 as input to the ADC unit
    
    delayMicroseconds(CHARGE_DELAY); // wait for the touch probe and the s&h cap to be fully charged/dsicharged
    
    DDRC&= ~(TPIN1_MASK | TPIN2_MASK); // config pins as input
    PORTC&= ~TPIN1_MASK; // disable the internal pullup to make the port high Z
    
    // connect touch probe cap to s&h cap to transfer the charge
    ADMUX= MUX_REF_VCC | TPIN1; // select ADC0 pin as ADC input (PINC0)
    
    delayMicroseconds(TRANSFER_DELAY); // wait for charge to be transfered
    
    // measure
    ADCSRA|= (1<<ADSC); // start measurement
    while(ADCSRA & (1<<ADSC)){  } // wait for conversion to complete
    adc0= ADC; // save conversion result
    adc1+= adc0; // accumulate the results for the averaging

    // second measurement:discharge touch probe, charge ADC s&h cap, connect the two, measure the volage
    // discharge sensor, charge s&h cap
    DDRC|= TPIN1_MASK | TPIN2_MASK; // config pins as push-pull output
    PORTC= (PORTC & ~TPIN1_MASK) | TPIN2_MASK; // set PINC1 high to charge the touch probe cap and PINC0 low to discharge s&h cap
    
    // charge s&h cap by connecting it to PINC1
    ADMUX = MUX_REF_VCC | TPIN2; // select PINC1 as input to the ADC unit
    
    delayMicroseconds(CHARGE_DELAY); // wait for the touch probe and the s&h cap to be fully charged/dsicharged
    
    DDRC&= ~(TPIN1_MASK | TPIN2_MASK); // config pins as input
    PORTC&= ~TPIN2_MASK; // disable the internal pullup to make the port high Z
    
    // connect touch probe cap to s&h cap to transfer the charge
    ADMUX= MUX_REF_VCC | TPIN1; // select ADC0 pin as ADC input (PINC0)
    
    delayMicroseconds(TRANSFER_DELAY); // wait for charge to be transfered
    
    // measure
    ADCSRA|= (1<<ADSC); // start measurement
    while(ADCSRA & (1<<ADSC)){  } // wait for conversion to complete
    adc0= ADC; // save conversion result
    adc2+= adc0; // accumulate the results for the averaging
  }
  adc1>>=2; // divide the accumulated measurements by 16
  adc2>>=2;
 
  // resulting raw touch value
  probe_val= adc1-adc2; // the value of adc1 (probe charged) gets higher when the probe ist touched, the value of adc2 (s&h charged) gets lower when the probe ist touched, so, it has to be be subtracted to amplify the detection accuracy
  
  // calculate the index to the LED fading table
  int16_t idx= (probe_val-TOUCH_VALUE_BASELINE); // offset probe_val by value of untouched probe
  if(idx<0) idx= 0; // limit the index!!!
  idx/= TOUCH_VALUE_SCALE; // scale the index
  if(idx>31) idx= 31; // limit the index!!!

  // print some info to the serial
  Serial.print(probe_val);
  Serial.print('|');
  Serial.println(idx);
  
  // fade the LED
  analogWrite(9, ledFadeTable[idx]);
  
  adc1= 0; // clear the averaging variables for the next run
  adc2= 0;
  delay(10); // take 100 measurements per second
}
 
