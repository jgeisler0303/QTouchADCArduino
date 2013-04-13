int adc1,adc0;
unsigned int adc2, adc3;

byte pwm;

#define ADMUX_MASK  0b00001111
#define MUX_GND 0b00001111
#define MUX_1p1V 0b0001110
#define MUX_REF_VCC 0b01000000
#define MUX_REF_1p1 0b11000000

const uint16_t pwmtable_8D[32] =
{0, 1, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 9, 10, 12, 15, 17, 21, 25, 30, 36, 43, 51, 61, 73, 87, 104, 125, 149, 178, 213, 255
};


#define QTDELAY  5 //Wartezeit in mikrosekunden

void setup() {
  Serial.begin(9600);
 
  ADMUX = 0b01000000; // Vcc as voltage reference (76), right adjustment (5), use ADC0 as input (3210)
  ADCSRA = 0b11000111; // enable ADC (7), initialize ADC (6), no autotrigger (5), don't clear int-flag ADIF (4), no interrupt ADIE (3), clock div by 128@16Mhz=125kHz ADPS (210)
  ADCSRB = 0b00000000; // no analog compare ACME (6), autotrigger scource free running ADTS, (210)
  while(ADCSRA & (1<<ADSC)){  } // wait for first conversion to complete
  adc1 = ADC;
  //pinMode(13, OUTPUT);
  // DDRC = 0x00;                //Port auf Ausgang setzten
}
 
 
void loop() {
  for (int i=0; i<16; i++) {
    // charge sensor
    DDRC = 0b00000001;                //Port auf Ausgang setzten
    PORTC = 0b00000001;          // pullup PINC0 auf High um CS zu laden und PINC1 auf LOW um C Sample & Hold entladen zu können
    
    // discharge
    ADMUX = MUX_REF_VCC | MUX_GND; //select ADC1 to discharge s&h cap
    ADCSRA |= (1<<ADSC);          // start conversion
    while(ADCSRA & (1<<ADSC)){  } // wait for conversion complete
    adc0 = ADC;                  // save conversion
    delayMicroseconds(QTDELAY);        // kurz warten bis geladen
    
    DDRC = 0x00;                //Port auf Ausgang setzten
    PORTC = 0x00;                  //Tristate
    
    // charge s&h cap
    ADMUX = MUX_REF_VCC;  // select ADC0
    delayMicroseconds(QTDELAY);          //warten?! geht auch ohne...
    
    // measure
    ADCSRA |= (1<<ADSC);            //Messung
    while(ADCSRA & (1<<ADSC)){  }
    adc1 = ADC;
    adc2 += adc1;                  //Mittelwertbildung

    // discharge sensor
    DDRC = 0b00000001;                //Port auf Ausgang setzten
    PORTC = 0b00000000;          // pullup PINC0 auf High um CS zu laden und PINC1 auf LOW um C Sample & Hold entladen zu können
    
    // charge
    ADMUX = MUX_REF_1p1 | MUX_1p1V; //select ADC1 to discharge s&h cap
    ADCSRA |= (1<<ADSC);          // start conversion
    while(ADCSRA & (1<<ADSC)){  } // wait for conversion complete
    adc0 = ADC;                  // save conversion
    delayMicroseconds(QTDELAY);        // kurz warten bis geladen
    
    DDRC = 0x00;                //Port auf Ausgang setzten
    //PORTC = 0x00;                  //Tristate
    
    // charge s&h cap
    ADMUX = MUX_REF_1p1;  // select ADC0
    delayMicroseconds(QTDELAY);          //warten?! geht auch ohne...
    
    // measure
    ADCSRA |= (1<<ADSC);            //Messung
    while(ADCSRA & (1<<ADSC)){  }
    adc1 = ADC;
    adc3 += adc1;                  //Mittelwertbildung
  }
  adc2>>=4;
  adc3>>=4;
 
  int16_t idx= (adc2-adc3-333);
  if(idx<0) idx= 0;
  idx/= 5;
  if(idx>31) idx= 31;

  Serial.print(adc2);
  Serial.print('|');
  Serial.print(idx);
  Serial.print('|');
  Serial.print(adc2-adc3);
  Serial.print('|');
  analogWrite(9, pwmtable_8D[idx]);
  Serial.println(pwmtable_8D[idx]);
  pwm++;
  if(pwm>32) pwm= 0;
  adc2=0;
  adc3= 0;
  delay(10);                          //je nachdem wieviele Messungen man pro sekunde haben möchte...
}
 
