#include "DHT.h"
#include <EEPROM.h>
#define DHTPIN 11     // DHT connect at pin 11
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

int setting; // setting 1 (T1 - T2/O - G)
             // setting 2 (T1 - T2/G - O)

int EEValue;       // EEPROM
boolean nw = true; // state of device
                   // true = new
                   // false = used
boolean z;
boolean start = false;
boolean sysStart = false;


// ######## ports ########

int sv0 = 6; // one way switch valve
int sv1 = 7; // 3 way 2 switch valve
int sv2 = 8; // 3 way 2 switch valve
int h0 = 9;  // heater 1
int h1 = 10; // heater 2

// ######## milis variables ######
unsigned long currentTime;
unsigned long heatDur = 1200000; //3600000; // 1 hour heating time
unsigned long blowDur = 2400000; //7200000; // 2 hours blowing time
//unsigned long delayDur = 5000; // 2 hours blowing time
unsigned long intchgDur = blowDur + heatDur; // 3 hours in total
unsigned long initBlowTime;
unsigned long initHeatTime;
unsigned long initIntchgTime;
unsigned long delayStart;

// ######## SETUP #########
void setup()
{
  Serial.begin(9600);
  dht.begin();

  pinMode(sv0, OUTPUT);
  pinMode(sv1, OUTPUT);
  pinMode(sv2, OUTPUT);
  pinMode(h0, OUTPUT);
  pinMode(h1, OUTPUT);

  EEValue = EEPROM.read(0);
  Serial.print("setting value: ");
  Serial.println(setting);

  conf0();
  Serial.println("default config  ");
  
  checkEE();     // check EEPROM content
  checkDHT();
  checkHumid();
  delay (1000);
}

// ########## LOOPING ##########
void loop()
{
  currentTime = millis();   // init milis
  delayTime();
  checkDHT();       // check DHT all variables
  //checkHumid();     // check humidity
  delay(1000);
  checkTime();
  checkHeat();      // check heater process - check the heating duration

}


// ######### FUNCTIONS (CHECKINGS) ##########
void checkEE() // check eeprom
{
  if (EEValue != 123) // the device has not been used before
  {
    nw = true; // device state is new
    digitalWrite(sv0, LOW); //valve 1 on
    Serial.println("First Setup");
    Serial.println("Valve 1 CLOSED");
    setting = 1;
    Serial.print("setting = ");
    Serial.println(setting);
    EEPROM.write(0, 123);
    Serial.print ("EEValue:");
    Serial.print (EEValue);
  }
  else if (EEValue == 123) // the device has been used before
  {
    nw = false ;// device state is used
    digitalWrite(sv0, HIGH);
    Serial.println ("Continue where it left off ");
    Serial.println("Valve 1 OPEN");
    Serial.print ("EEValue:");
    Serial.println (EEValue);
  }
  Serial.println("");
}

void checkHumid()
{
  if (nw == false)
  {
    conf0();
    selSet();
    //   setting = 2;
    //   heatTube1();
  }
}


void checkTime()
{
  if (sysStart == true)
  {
    initIntchgTime = currentTime;
    Serial.print("System time: ");
    float sysTime = initIntchgTime / 1000;
    float sysTimeMin, sysTimeHr;
    if (sysTime > 60 && sysTime < 3600)
    {
      sysTimeMin = sysTime / 60;
      Serial.print(sysTimeMin);
      Serial.println(" mins elapsed");
    }
    else if (sysTime > 60 && sysTime > 3600)
    {
      sysTimeHr = sysTime/ 3600;
      Serial.print(sysTimeHr);
      Serial.println(" hrs elapsed");
    }
    else
    {
      Serial.print(sysTime);
      Serial.println(" secs elapsed");
    }
    Serial.println("");
    if (initIntchgTime > (initHeatTime + intchgDur))
    {
      intchgSet();
      start = true;
    }
  }
}

void checkDHT() // check whole dht - humid, temp in C & F
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);
  Serial.print("Humidity: ");
  Serial.println(h);
  Serial.print("Temperature in C: ");
  Serial.println(t);
  Serial.print("Temperature in F: ");
  Serial.println(h);
  Serial.print("Heat index in F: ");
  Serial.println(hif);
  Serial.print("Heat index in C: ");
  Serial.println(hic);
  Serial.println("");
  delay(2000);
  if (isnan(h) || isnan(t) || isnan(f))
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
}

void checkHeat() // check heater
{
  if (z == false && start == true )
  {
    if (setting == 1) {
      if (initIntchgTime > (initHeatTime + heatDur))
      {
        Serial.println("Tube 2 heater is off ");
        Serial.println("");
        digitalWrite(h1, LOW);
      }
      else
      {
        Serial.println("Tube 2 heater is on");
        Serial.println("");
      }
    }
    else if (setting == 2) {
      if (initIntchgTime > (initHeatTime + heatDur))
      {
        Serial.println("Tube 1 heater is off");
        Serial.println("");
        digitalWrite(h0, LOW);
      }
      else
      {
        Serial.println("Tube 1 heater is on");
        Serial.println("");
      }
    }
  }
}

//######## SETS & RESETS ##########
void intchgSet() // change setting once( humid reached / time reached )
{
  z = false;
  Serial.println("interchange occurs");
  if (start == true)
  {
    if (setting == 1)
    {
      setting = 2;
      Serial.println("configuration 2"); // heat tube 1 to exhaust - blow tube 2 to ozone
      heatTube1();
    }
    else if (setting == 2)
    {
      setting = 1;
      Serial.println("configuration 1"); // heat tube 2 to exhaust - blow tube 1 to ozone
      heatTube2();
    }
  }
}

void selSet()
{
  float h = dht.readHumidity();
  if (h > 40)
  {
    setting = 1;
    intchgSet();
    Serial.println("setting found: 1");
    Serial.println("");
  }
  else
  {
    conf1();  // starts here - checking 2 tubes dht
    if (h > 40)
    {
      intchgSet();
      setting = 2;
      Serial.println("setting 2");
      Serial.println("");
    }        // ends here
    else
    {
      conf0();
      setting = 1;
      Serial.println("setting found: 1");
      Serial.println("");
      sysStart = true;
    }
  }
}

void heatTube1() // heat tube 1, sol 1 to exhaust
{
  digitalWrite(h0, HIGH); //Turn on Heater 1
  digitalWrite(h1, LOW); // Turn off Heater 2
  digitalWrite(sv1, HIGH); // solenoid 1 to exhaust
  digitalWrite(sv2, LOW); // solenoid 2 to ozone
  Serial.println("heating tube 1");
  Serial.println("");
  inHeat();
}

void heatTube2() // heat tube 2, sol 2 to exhaus
{
  digitalWrite(h0, LOW); //Turn off Heater 1
  digitalWrite(h1, HIGH); // Turn on Heater 2
  digitalWrite(sv1, LOW); // solenoid 1 to ozone
  digitalWrite(sv2, HIGH); // solenoid 2 to exhaust
  Serial.println("heating tube 2");
  Serial.println("");
  inHeat();
}

void inHeat() // take initial heating time
{
  initHeatTime = currentTime;
  Serial.print("heating starts at:");
  Serial.println(initHeatTime);
  Serial.println("");
}

void conf0()
{
  digitalWrite(sv1, LOW);
  digitalWrite(sv2, HIGH);
}

void conf1()
{
  digitalWrite(sv1, HIGH);
  digitalWrite(sv2, LOW);
}

void delayTime()
{
  delayStart = currentTime;
  if (delayStart > delayTime)
  {
    sysStart = true;
  }
}
