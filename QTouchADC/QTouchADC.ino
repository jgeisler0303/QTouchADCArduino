unsigned int adc0; // temp var
unsigned int adc1, adc2; // store the avarage of the charge resp. discharge measurement
unsigned int probe_val; // store the resulting touch measurement

// ADC constants
#define ADMUX_MASK  0b00001111 // mask the mux bits in the ADMUX register
#define MUX_GND 0b00001111 // mux value for connecting the ADC unit internally to GND
#define MUX_1p1V 0b0001110 // mux value for connecting the ADC unit to the internal 1.1V band gap reference
#define MUX_REF_VCC 0b01000000 // value to set the ADC reference to Vcc
#define MUX_REF_1p1 0b11000000 // value to set the ADC reference to the internal 1.1V band gap reference

const uint16_t ledFadeTable[32] = {0, 1, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 9, 10, 12, 15, 17, 21, 25, 30, 36, 43, 51, 61, 73, 87, 104, 125, 149, 178, 213, 255}; // this is an exponential series to model the perception of the LED brightness by the human eye


#define QTDELAY  5 // time it takes for the capacitors to get charged/discharged in microseconds
#define TOUCH_VALUE_BASELINE 333 // this is the value my setup measures when the probe is not touched. For your setup this might be different. In order for the LED to fade correctly, you will have to adjust this value
#define TOUCH_VALUE_SCALE 5 // this is also used for the LED fading. The value should be chosen such that the value measured when the probe is fully touched minus TOUCH_VALUE_BASELINE is scaled to 31, e.g. untouched_val= 333, touched_val= 488, difference= 155, divide by 5 to get 31.

void setup() {
  Serial.begin(9600); // standard serial setup
 
  // prepare the ADC unit for one-shot measurements
  // see the atmega328 datasheet for explanations of the registers and values
  ADMUX = 0b01000000; // Vcc as voltage reference (bits76), right adjustment (bit5), use ADC0 as input (bits3210)
  ADCSRA = 0b11000111; // enable ADC (bit7), initialize ADC (bit6), no autotrigger (bit5), don't clear int-flag  (bit4), no interrupt (bit3), clock div by 128@16Mhz=125kHz (bit210) ADC should run at 50kHz to 200kHz
  ADCSRB = 0b00000000; // autotrigger source free running (bits210) doesn't apply
  while(ADCSRA & (1<<ADSC)){  } // wait for first conversion to complete
  adc0 = ADC; // not sure if the value of the conversion has to be read if not needed
}
 
 
void loop() {
  // 16 measurements are taken an d averaged to improve noise immunity
  for (int i=0; i<16; i++) {
    // first measurement: charge touch probe, discharge ADC s&h cap, connect the two, measure the volage
    // charge sensor
    DDRC= 0b00000001; // config pin as push-pull output (using the internal pullup in input mode is too slow). Beware this way of setting the port config changes the whole of PORTC!! DDRC|= 0b00000001 would be better
    PORTC= 0b00000001; // set PINC0 high to charge the touch probe cap
    
    // discharge s&h cap by measureing the internal GND reference
    ADMUX = MUX_REF_VCC | MUX_GND; // select the internal GND reference as input to the ADC unit
    ADCSRA |= (1<<ADSC); // apparently selecting the input is not enough. ASo, a conversion has to be started
    while(ADCSRA & (1<<ADSC)){  } // wait for conversion to complete
    adc0= ADC; // save conversion to dummy var. maybe not necessary?
    
    delayMicroseconds(QTDELAY); // wait for the touch probe cap to be fully charged
    
    DDRC= 0x00; // config pin as input. Beware this way of setting the port config changes the whole of PORTC!! DDRC&= ~0b00000001 would be better
    PORTC= 0x00; // disable the internal pullup to make the port tristate
    
    // connect touch probe cap to s&h cap to transfer the charge
    ADMUX= MUX_REF_VCC; // select ADC0 pin as ADC input (PINC0), setting Vcc as ADC reference voltage
    
    delayMicroseconds(QTDELAY); // wait for charge to be transfered. maybe not necessary?
    
    // measure
    ADCSRA|= (1<<ADSC); // start measurement
    while(ADCSRA & (1<<ADSC)){  } // wait for conversion to complete
    adc0= ADC; // save conversion result
    adc1+= adc0; // accumulate the results for the averaging

    // second measurement:discharge touch probe, charge ADC s&h cap, connect the two, measure the volage
    // discharge sensor
    DDRC= 0b00000001; // config pin as push-pull output
    PORTC= 0b00000000; // set PINC0 low to discharge the touch probe cap
    
    // charge  s&h cap by measuring the internal 1.1V reference
    ADMUX= MUX_REF_1p1 | MUX_1p1V; // select the internal 1.1V reference as input to the ADC unit and also as reference for the ADC conversion
    ADCSRA|= (1<<ADSC); // apparently selecting the input is not enough. So, a conversion has to be started
    while(ADCSRA & (1<<ADSC)){  }// wait for conversion to complete
    adc0= ADC; // save conversion to dummy var. maybe not necessary?
    
    delayMicroseconds(QTDELAY); // wait for the touch probe cap to be fully discharged
    
    DDRC= 0x00; // config pin as input (pullup is already disabled)
    
    // connect touch probe cap to s&h cap to transfer the charge
    ADMUX = MUX_REF_1p1; // select ADC0 pin as ADC input (PINC0), setting internal 1.1V as ADC reference voltage
    delayMicroseconds(QTDELAY); // wait for charge to be transfered. maybe not necessary?
    
    // measure
    ADCSRA |= (1<<ADSC); // start measurement
    while(ADCSRA & (1<<ADSC)){  } // wait for conversion to complete
    adc0= ADC; // save conversion result
    adc2+= adc0;// accumulate the results for the averaging
  }
  adc1>>=4; // divide the accumulated measurements by 16
  adc2>>=4;
 
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
 
