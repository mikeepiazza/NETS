#include <ad7124.h>
//Slave select pin for SPI
const int ssPin = 10;

//Create an ADC instance
Ad7124Chip adc;

//Initialize variables
double voltage = 0;
long timer = 0;
long counter = 0;
int n = 1;

void setup() {
  //Open serial connection
  Serial.begin (115200); 
  while (!Serial) {
    ; 
  }

  //Configure ADC
  adc.begin (ssPin); 
  adc.setConfig (0, Ad7124::RefInternal, Ad7124::Pga1, false, Ad7124::BurnoutOff);
  adc.setConfigOffset  (0, 0x7FE600);
//  adc.setConfigFilter  (0, Ad7124::Sinc3Filter, 1, Ad7124::NoPostFilter, false, true);
  adc.setChannel (0, 0, Ad7124::AIN1Input, Ad7124::AIN0Input, true);
  adc.setAdcControl (Ad7124::ContinuousMode, Ad7124::FullPower, true, Ad7124::InternalClk); 
}

void loop() {
  //Check for elapsed interval and display ADC data 
  if ((micros()-timer) > 1e6*n){ 
    showResult();
    counter = 0;
    timer = micros();
  }
  
  //Retrieve current ADC sample and convert it to a voltage
  voltage = Ad7124Chip::toVoltage (adc.getData(), 1, 2.5, false);
  //Keeps track of which sample number we are on
  counter++;

}

void showResult() {
    Serial.print("Elapsed Time: ");
    Serial.print((micros()-timer)/1e6);
    Serial.println("s");
    Serial.print("Sample Rate: ");
    Serial.print(counter/n);
    Serial.println(" SPS");
    Serial.print("Voltage: ");
    printDouble(voltage, 10000);
    Serial.println("V");
}

void printDouble( double val, unsigned int precision){
// prints val with number of decimal places determine by precision
// NOTE: precision is 1 followed by the number of zeros for the desired number of decimial places
// example: printDouble( 3.1415, 100); // prints 3.14 (two decimal places)

   Serial.print (int(val));  //prints the int part
   Serial.print("."); // print the decimal point
   unsigned int frac;
   if(val >= 0)
     frac = (val - int(val)) * precision;
   else
      frac = (int(val)- val ) * precision;
   int frac1 = frac;
   while( frac1 /= 10 )
       precision /= 10;
   precision /= 10;
   while(  precision /= 10)
       Serial.print("0");

   Serial.print(frac,DEC) ;
}
