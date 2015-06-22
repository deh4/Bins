#include <SPI.h>
#include <MySensor.h>  
#include <NewPing.h>

#define SERIAL_DEBUG
#define VCC_IN_MV 5000
#define CHILD_ID 1
#define TRIGGER_PIN  6  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     5  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 300 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
unsigned long SLEEP_TIME = 5000; // Sleep time between reads (in milliseconds)
int SLEEP_TIME_IN_SEC = SLEEP_TIME / 1000 ; 
MySensor gw;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
MyMessage msg(CHILD_ID, V_DISTANCE);
int lastDist;
int sleepTimeLoopCount = 1;
int noChangeCount = 0;
boolean metric = true; 

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Distance Sensor", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID, S_DISTANCE);
  boolean metric = gw.getConfig().isMetric;
}

void loop()      
{     
  int dist = metric?sonar.ping_cm():sonar.ping_in();
  String batteryVoltage = "";
  batteryVoltage += vccVoltage();
  int batteryPercentage = (batteryVoltage * 100)/ VCC_IN_MV
#ifdef SERIAL_DEBUG  
  char batteryVoltageCharBuffer[10];
  batteryVoltage.toCharArray(batteryVoltageCharBuffer, 10);
  Serial.print("Voltage: ");
  Serial.print(batteryVoltageCharBuffer);
  Serial.println(" mV");  

  Serial.print("Ping: ");
  Serial.print(dist); // Convert ping time to distance in cm and print result (0 = outside set distance range)
  Serial.println(metric?" cm":" in");
#endif
  int deltaDistance = lastDist - dist;
  if(deltaDistance < -10 || deltaDistance > 10)
  {
    gw.send(msg.set(dist));
    gw.sendBatteryLevel(batteryPercentage,false);
    lastDist = dist;
    noChangeCount=0;
    sleepTimeLoopCount = 2;
  }
  else
  {
    if(noChangeCount == 6)
    {
      // 12 * sleepTimeLoopCount(1) * SLEEP_TIME_IN_SEC(5) == 60 Seconds
      // Decrease sampling frequency to 1 min
      sleepTimeLoopCount = 12 ;
      noChangeCount++;
    }
    else if(noChangeCount == 16)
    {
      //No activity for 10 min
      //Decrease sampling frequency to 10 min
      sleepTimeLoopCount = 120 ;
      noChangeCount++;      
    }
    else
    {
      noChangeCount++;
    }
  }
  for(int i=0;i<sleepTimeLoopCount;i++)
  {
    gw.sleep(SLEEP_TIME);
  }
}

long vccVoltage() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}


