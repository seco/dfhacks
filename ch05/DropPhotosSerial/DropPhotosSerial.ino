/* Digital Fotografie Hacks
 * DropPhotos: 
 * Tropfen Fotografie mit dem Foto Shield
 * v0.1, 2014-01-26, Peter Recktenwald 
*/ 
/*-----( Import needed libraries )-----*/
#include <SoftwareSerial.h>   // needed by SerialCommand
// https://github.com/scogswell/ArduinoSerialCommand
#include <SerialCommand.h>
#include <DFH_OptoCam.h>
#include <DFH_OptoFlash.h>

/*-----( Declare Constants )-----*/
#define focusPin    8 
#define shutterPin  9
#define flashPin    7
#define valvePin    5

enum eCommand {
  cmdNONE,
  cmdCANCEL,
  cmdDROPPHOTOS
};

enum eMode {
  modNONE,
  modRUNNING
};

/*-----( Declare objects )-----*/
SerialCommand SCmd;   // The demo SerialCommand object
DFH_OptoCam myCam(focusPin, shutterPin);
DFH_OptoFlash myFlash(flashPin);

/*-----( Declare Variables )-----*/

long repeatCount = 1;
int delayTime = 300;
int delayIncr = 0;
int Command = cmdNONE;
int valveParam[10];
int timeParam[10];
long cnt, dly, dlyofs;
int mode = modNONE;

// Drop Photos command Handler 
void processDropPhotosCmd()
{
  int aNumber;
  char *arg;

  Serial.println("We're in processDropPhotosCmd");
  arg = SCmd.next();
  if (arg != NULL)
  {
    aNumber = atoi(arg);  // Converts a char string to an integer
    delayTime = aNumber;
  }
  arg = SCmd.next();
  if (arg != NULL)
  {
    aNumber = atoi(arg);  // Converts a char string to an integer
    delayIncr = aNumber;
  }
  arg = SCmd.next();
  if (arg != NULL)
  {
    aNumber = atoi(arg);  // Converts a char string to an integer
    repeatCount = aNumber;
  }
  if ((repeatCount > 0) && (delayTime > 0))
  {
    Command = cmdDROPPHOTOS;
    Serial.print("Delay Time: ");
    Serial.println(delayTime);
    Serial.print("Repeat Count: ");
    Serial.println(repeatCount);
  }
  else
    Serial.println("invalid arguments");
}

// Valve Config command handler
void processValveCmd()    
{
  int aNumber, paramIdx;  
  char *arg; 

  Serial.println("We're in processValveCmd"); 
  paramIdx = 0;
  do {
    arg = SCmd.next();
    if (arg != NULL) 
    {
      aNumber=atoi(arg);    // Converts a char string to an integer
      Serial.print("Valve"); 
      if ((paramIdx & 1) == 0)
        Serial.print(" on: "); 
      else
        Serial.print(" off: "); 
      Serial.println(aNumber); 
      valveParam[paramIdx++] = aNumber;
    }    
  } while(arg != NULL);
  valveParam[paramIdx++] = 0;    
}

// Cancel command Handler
void processCancelCmd()
{
  Serial.println("We're in processCancelCmd");
  Command = cmdCANCEL;
}

// default handler
void unrecognized()
{
  Serial.println("error: unknown command!");
  printHelp();
}

// print help screen
void printHelp(void)
{
  Serial.println("available commands:");
  Serial.println("Drop Photos: 'DP <delaytime> <delayincr> <repeatcount>'");
  Serial.println("Valve Config: 'VC <on1> <off1> <on2> <off2> <on3> <off3>'");
  Serial.println("Cancel: 'C'");
}
  
void setup() {
  // set up
  pinMode(valvePin, OUTPUT);
  digitalWrite(valvePin, LOW);

  SCmd.addCommand("DP", processDropPhotosCmd); // Handler for Drop Photos command
  SCmd.addCommand("VC", processValveCmd);      // Handler for Valve Config command
  SCmd.addCommand("C", processCancelCmd);      // Handler for Cancel command
  SCmd.addDefaultHandler(unrecognized);        // default handler

  Serial.begin(57600);  // Used to type in characters
  Serial.println("Drop Photos v0.1");
  printHelp();
  valveParam[0]= 20;
  valveParam[1]= 20;
  valveParam[2]= 20;
}

void launchDrop()
{
  int idx, val;
  
  for(idx=0; idx < 6; idx++)
  {
    val = valveParam[idx];
    if (val && ((idx & 1) == 0) )
    {
      digitalWrite(valvePin, HIGH);
      delay(val);
      digitalWrite(valvePin, LOW);
    }
    else if (val)
    {
      digitalWrite(valvePin, LOW);
      delay(val);
    }
    else
      break;
  }
}

void loop() 
{

  SCmd.readSerial();     // process serial commands
  
  switch(Command)
  {
    case cmdDROPPHOTOS:   // received timelapse command
      cnt = repeatCount;
      dly = delayTime;
      dlyofs = delayIncr;
      mode = modRUNNING;
      Command = cmdNONE;
      Serial.println("running..");
    break;
    case cmdCANCEL:     // received cancel command
      mode = modNONE;
      Command = cmdNONE;
      Serial.println("operation canceled..");
    break;
    default:
    break;
  }
    
  if (mode == modRUNNING)
  {
    if (cnt)
    {
      launchDrop();
      myCam.launchFocusShutter();
      delay(dly);
      myFlash.fire();
      myCam.releaseFocusShutter();
      Serial.print("delay ");
      Serial.print(dly, DEC);
      Serial.print(" shoot ");
      Serial.println(cnt, DEC);
      dly += dlyofs;
      cnt--;
      delay(5000);
    }
    else
    {
      mode = modNONE;
      Serial.println("done..");
    }
  }
}
