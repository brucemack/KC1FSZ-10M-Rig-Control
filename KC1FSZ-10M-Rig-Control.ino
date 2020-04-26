#include <SPI.h>
#include <Wire.h>
#include <si5351.h>
#include <EEPROM.h>

// set this to the hardware serial port you wish to use
#define HWSERIAL Serial1

Si5351 si5351;

unsigned int correction = 13000;
//unsigned int vfoA = 28333330;
unsigned int vfoA = 28400000;
unsigned int bfo =   9995000;
int txMode = 0;

void setVfoA(unsigned int f) {
  vfoA = f;
  // Calculatee clock 2
  unsigned int vco = vfoA - bfo - 230;
  si5351.set_freq((unsigned long long)vco * 100ULL, SI5351_CLK2);
  HWSERIAL.print("VFO ");  
  HWSERIAL.print(f);
  HWSERIAL.print(", Setting VCO to ");
  HWSERIAL.println(vco);
}

#define PA_BIAS_PIN 2
#define TR_PIN 3

void setTxMode(int t) {
  txMode = t;
  if (txMode) {    
    digitalWrite(PA_BIAS_PIN, 1);
    delay(50);
    digitalWrite(TR_PIN, 1);
    HWSERIAL.println("TX mode");
  } else {
    digitalWrite(TR_PIN, 0);
    delay(50);
    digitalWrite(PA_BIAS_PIN, 0);
    HWSERIAL.println("RX mode");
  }
}

void printSi5351Status() {
  // Get status
  si5351.update_status();
  delay(250);
  si5351.update_status();
  delay(250);
  HWSERIAL.print("Current status - SYS_INIT: ");
  HWSERIAL.print(si5351.dev_status.SYS_INIT);
  HWSERIAL.print("  LOL_A: ");
  HWSERIAL.print(si5351.dev_status.LOL_A);
  HWSERIAL.print("  LOL_B: ");
  HWSERIAL.print(si5351.dev_status.LOL_B);
  HWSERIAL.print("  LOS: ");
  HWSERIAL.print(si5351.dev_status.LOS);
  HWSERIAL.print("  REVID: ");
  HWSERIAL.println(si5351.dev_status.REVID);
}

void setup() {

  Serial.begin(9600);
  HWSERIAL.begin(9600);
  HWSERIAL.println("Running ...");

  pinMode(TR_PIN,OUTPUT);
  pinMode(PA_BIAS_PIN,OUTPUT);
  digitalWrite(TR_PIN,0);
  digitalWrite(PA_BIAS_PIN,0);

  // Si5351 initialization defaults
  si5351.init(SI5351_CRYSTAL_LOAD_8PF,0,0);

  printSi5351Status();

  // Si5351 Setup
  si5351.set_correction(correction, SI5351_PLL_INPUT_XO);
  // BFO setup
  si5351.output_enable(SI5351_CLK1,1);
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);
  si5351.set_freq((unsigned long long)bfo * 100ULL, SI5351_CLK1);
  // VCO setup
  si5351.output_enable(SI5351_CLK2,1);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);

  setVfoA(28400000);
  setTxMode(0);
}


void runCmd(char* cmd) {
  char resp[80];
  // Check VFO
  if (strcmp(cmd,"AI0") == 0) {
    //HWSERIAL.println("AI0 received");        
  } else if (strcmp(cmd,"RX") == 0) {
    setTxMode(0);     
  } else if (strcmp(cmd,"TX1") == 0) {
    setTxMode(1);    
  // VFO A
  } else if (cmd[0] == 'F' && cmd[1] == 'A') {
    // Read request
    if (cmd[2] == 0) {
      sprintf(resp,"FA%011u;",vfoA);
      Serial.print(resp);      
    } 
    // Write request
    else {
      String a(cmd+2);
      unsigned int f = 0;
      f = a.toInt();
      setVfoA(f);
    }
  // VFO B
  } else if (cmd[0] == 'F' && cmd[1] == 'B') {
    // Request
    if (cmd[2] == 0) {
      sprintf(resp,"FB%011u;",vfoA);
      Serial.print(resp);      
    } else {
      HWSERIAL.print("Got ");    
      HWSERIAL.println(cmd);    
    }
  } else if (strcmp(cmd,"PT") == 0) {
    sprintf(resp,"PT000;");
    HWSERIAL.print("Sending ");    
    HWSERIAL.println(resp);    
    Serial.print(resp);    
  } else if (cmd[0] == 'I' && cmd[1] == 'F') {
    sprintf(resp,"IF%011u     %05u00000%d2000000 ;",vfoA,0,txMode);
    Serial.print(resp);
  } else {
    HWSERIAL.print("Got command: ");
    HWSERIAL.println(cmd);
  }
}

// Where we read commands into
char cmdBuf[80];
int cmdBufPtr = 0;

void loop() {
  if (Serial.available() > 0) {
    char ib = Serial.read();
    Serial.print(ib);
    if (ib == ';') {
      cmdBuf[cmdBufPtr] = 0;
      runCmd(cmdBuf);
      cmdBufPtr = 0;
    } else {
      cmdBuf[cmdBufPtr++] = ib;
    }
  }
  if (HWSERIAL.available() > 0) {
    char ib = HWSERIAL.read();
    if (ib == 't') {
      if (txMode) {
        setTxMode(0);
      } else {
        setTxMode(1);
      }
    }
  }
}
