// https://raw.githubusercontent.com/dbuezas/lgt8fx/master/package_lgt8fx_index.json

// On-board LED, useful for debugging
#define LEDPIN     13 // D13
 
// Pulse width in µs
#define PULSEWIDTH 527
 
// Address that the radio responds to
#define ADDRESS 0x47

#define OUTPUTPIN   8 // D8

#define HOLDTIME 400 //Assign the time after which the button is considered clamped

// remote control commands for OPEL
const int TR_NEXT=1;
const int TR_PREV=2;
const int MODE=3;
const int VOL_UP=4;
const int VOL_DOWN=5;
const int ON_OFF=6;

#define CMD_ON_OFF      0x00
#define CMD_VOLUP       0x04
#define CMD_VOLDOWN     0x05
#define CMD_SOURCE      0x08
#define CMD_EQUALIZER   0x0D
#define CMD_MUTE        0x0E
#define CMD_TRACKFORW   0x12
#define CMD_TRACKBACK   0x13
#define CMD_FOLDERFORW  0x14
#define CMD_FOLDERBACK  0x15
#define CMD_UNKNOWN1    0x37
#define CMD_UNKNOWN2    0x58

int wheelPin=A1; // steering wheel resistance reading pin

int i=0;
int prevButton=0;
boolean flag = false;  //The flag that the previously held button is repeated
unsigned long time;// variable for storing time

void setup() {                 
  pinMode(wheelPin, INPUT);

  delay(100);
  Serial.begin(9600); 
  Serial.println("Startup");
}


// this function returns pressed button code or 0 if all buttons are released
int getWheelButton() 
{ 
  // read resistance value from the steering wheel buttons
  int r=getAvgValue();
  //Serial.println(r);
 
  // below values are for OPEL steering wheel controls and 470 Ohms known resistor
  if (r>=140 && r<=180) return(VOL_DOWN);   //   85 Ohm (0.77V) 166 is a typical value
  if (r>=240 && r<=300) return(VOL_UP);     //  170 Ohm (1.33V) 267
  if (r>=340 && r<=400) return(ON_OFF);     //  290 Ohm (1.91V) 345-355
  if (r>=430 && r<=510) return(TR_NEXT);    //  470 Ohm (2.5V)  440
  if (r>=550 && r<=650) return(TR_PREV);    //  800 Ohm (3.15V) 576
  if (r>=670 && r<=800) return(MODE);       // 1500 Ohm (3.81V) 676
  //if (r>=900) return(0); // Error, probably broken wiring
  return (0);
}

#define NUM_READS    50

int getAvgValue()
{
  long sum = 0;
  int succCounter = 0; int lastRead = 0;
  for(int i=0; i<NUM_READS; i++) {
    int curr = analogRead(wheelPin);
    if (i>0) {
      int diff = abs(lastRead - curr);
      // over 25% difference should be excluded
      if (diff * 1000 / lastRead > 25) {
        continue;
      }
    } 
    sum += curr;
    lastRead = curr;
    succCounter++;
  }
  return sum / succCounter;
}

void loop() {
  int currButton=getWheelButton(); // get current pressed button code
  if (currButton!=prevButton) { // if it has changed since last reading
    time = millis(); 
    flag = false;
    prevButton=currButton; 

    // send command to JVC car stereo
    switch(currButton) {
      case VOL_UP:      
        SendCommand(CMD_VOLUP);   
        Serial.println("VOL_UP");
        break;  

      case VOL_DOWN:
        SendCommand(CMD_VOLDOWN);   
        Serial.println("VOL_DOWN");
        break;

      case TR_NEXT:
        SendCommand(CMD_TRACKFORW);  
        Serial.println("TR_NEXT"); 
        break;

      case TR_PREV: 
        SendCommand(CMD_TRACKBACK);   
        Serial.println("TR_PREV"); 
        break;

      case MODE:
        SendCommand(CMD_FOLDERFORW);  
        Serial.println("MODE");            
        break;

      case ON_OFF:
        SendCommand(CMD_MUTE);   
        Serial.println("MUTE"); 
        break;
    }
  } else {
    if (millis() >= (time + HOLDTIME) && !flag) {
      switch(currButton) {
        case ON_OFF:
          SendCommand(CMD_ON_OFF);   
          Serial.println("hold, ON_OFF"); 
          break;

        case TR_NEXT:
          SendCommand(CMD_FOLDERFORW);  
          Serial.println("hold, FOLDER_NEXT"); 
          delay(800);
          break;

        case TR_PREV: 
          SendCommand(CMD_FOLDERBACK);   
          Serial.println("hold, FOLDER_PREV"); 
          delay(800);
          break;

        case VOL_UP:      
          SendCommand(CMD_VOLUP);   
          Serial.println("hold, VOL_UP");
          delay(150);
          break;

        case VOL_DOWN:
          SendCommand(CMD_VOLDOWN);   
          Serial.println("hold, VOL_DOWN");
          delay(150);
          break;
          
        case MODE:
          SendCommand(CMD_SOURCE);   
          Serial.println("hold, change source");
          time = millis();  
          flag = true; // Prevent any further repeat for this
          break;
      }
    }
  }

  delay(5);
}


// Send a value (7 bits, LSB is sent first, value can be an address or command)
void SendValue(unsigned char value) {
  unsigned char i, tmp = 1;
  for (i = 0; i < sizeof(value) * 8 - 1; i++) {
    if (value & tmp)  // Do a bitwise AND on the value and tmp
      SendOne();
    else
      SendZero();
    tmp = tmp << 1; // Bitshift left by 1
  }
}
 
// Send a command to the radio, including the header, start bit, address and stop bits
void SendCommand(unsigned char value) {
  unsigned char i;
  Preamble();                         // Send signals to precede a command to the radio
  for (i = 0; i < 3; i++) {           // Repeat address, command and stop bits three times so radio will pick them up properly
    SendValue(ADDRESS);               // Send the address
    SendValue((unsigned char)value);  // Send the command
    Postamble();                      // Send signals to follow a command to the radio
  }
}
 
// Signals to transmit a '0' bit
void SendZero() {
  digitalWrite(OUTPUTPIN, HIGH);      // Output HIGH for 1 pulse width
  digitalWrite(LEDPIN, HIGH);         // Turn on on-board LED
  delayMicroseconds(PULSEWIDTH);
  digitalWrite(OUTPUTPIN, LOW);       // Output LOW for 1 pulse width
  digitalWrite(LEDPIN, LOW);          // Turn off on-board LED
  delayMicroseconds(PULSEWIDTH);
}
 
// Signals to transmit a '1' bit
void SendOne() {
  digitalWrite(OUTPUTPIN, HIGH);      // Output HIGH for 1 pulse width
  digitalWrite(LEDPIN, HIGH);         // Turn on on-board LED
  delayMicroseconds(PULSEWIDTH);
  digitalWrite(OUTPUTPIN, LOW);       // Output LOW for 3 pulse widths
  digitalWrite(LEDPIN, LOW);          // Turn off on-board LED
  delayMicroseconds(PULSEWIDTH * 3);
}
 
// Signals to precede a command to the radio
void Preamble() {
  // HEADER: always LOW (1 pulse width), HIGH (16 pulse widths), LOW (8 pulse widths)
  digitalWrite(OUTPUTPIN, LOW);       // Make sure output is LOW for 1 pulse width, so the header starts with a rising edge
  digitalWrite(LEDPIN, LOW);          // Turn off on-board LED
  delayMicroseconds(PULSEWIDTH * 1);
  digitalWrite(OUTPUTPIN, HIGH);      // Start of header, output HIGH for 16 pulse widths
  digitalWrite(LEDPIN, HIGH);         // Turn on on-board LED
  delayMicroseconds(PULSEWIDTH * 16);
  digitalWrite(OUTPUTPIN, LOW);       // Second part of header, output LOW 8 pulse widths
  digitalWrite(LEDPIN, LOW);          // Turn off on-board LED
  delayMicroseconds(PULSEWIDTH * 8);
  
  // START BIT: always 1
  SendOne();
}
 
// Signals to follow a command to the radio
void Postamble() {
  // STOP BITS: always 1
  SendOne();
  SendOne();
}