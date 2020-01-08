//Sensor Variables
int EDAPin = A2;
int PulsePin = A0;         // Pulse Sensor purple wire connected to analog pin 0
int pressurePin = A1;
int buttonPin = 12;

//Output variables
int EDA = 0;              // Raw EDA measurement
int IBI = 600;            // Time interval between heart beats
int pressure = 0;         // Raw Pressure Sensor Measurement
int rawPulseSensor = 0;   // Raw reading from the pulse sensor pin (can be used for plotting the pulse)
int testLED = 11;


int IBISign = 1;           // used to change the sign of the IBI, to indicate a new value (in case two succeeding values are the same)
boolean Pulse = false;     // "True" when a heartbeat is detected. (can be used for for blinking an LED everytime a heartbeat is detected) 
bool NewPulse = false;

//Serial variables
bool Connected = false;
const unsigned int serialOutputInterval = 10; // Output Frequency = 1000 / serialOutputInterval = 1000 / 10 = 100Hz
unsigned long serialLastOutput = 0;
const char StartFlag = '#';
const String Delimiter = "\t";

// PulseChecker variables
int Threshold = 525;                      // used to find instant moment of heart beat
int Peak = 0;                             // used to find peak in pulse wave
int Trough = 1023;                        // used to find trough in pulse wave
int Signal;                               // holds the incoming raw data
unsigned long lastBeatTime = 0;           // Store last millis a pulse was detected
unsigned long TimeoutInterval = 1500UL;
bool Resetting = true;

void setup() {
  Serial.begin(115200);
  

//Example input/output pins
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(2, INPUT);

  analogReference(EXTERNAL);

  Serial.print("#");
  Serial.println("----- LOG BEGIN EDA-IBI-SERIAL (sep=tab, col=6, label=EDAIBISerial) -----");
  Serial.print("#");
  Serial.println("Millis\tEDA\tIBI\tRawPulse\tPressure\tButton");
  digitalWrite(testLED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(testLED, LOW);    // turn the LED off by making the voltage LOW
}

void loop() {

  ReadSensors(); //Have the Arduino read it's sensors etc.

  //SerialInput(); //Check if Unity has sent anything

  SerialOutput(); //Check if it is time to send data to Unity

  digitalWrite(13,Pulse); // Light up the LED on Pin 13 when a pulse is detected
}

void ReadSensors()
{
  EDA = analogRead(EDAPin); //Read the raw 

  pressure = analogRead(pressurePin);

  checkPulseSensor();
}

/* SERIALINPUT DISABLED FOR NOW
const int inputCount = 4; //This must match the amount of bytes you send from Unity!
byte inputBuffer[inputCount];
void SerialInput()
{  
  if(Serial.available() > 0){ //check if there is some data from Unity
     Serial.readBytes(inputBuffer, inputCount); //read the data
     //Use the data for something
     digitalWrite(13, inputBuffer[0]);

     analogWrite(3, inputBuffer[1]);
     analogWrite(5, inputBuffer[2]);
     analogWrite(6, inputBuffer[3]);


     //You could for example use the data for playing patterns
     //e.g. first value indicates which player and second value indicates which pattern to play
  }

     //Currently no checks for desync (no start/end flags or package size checks)
     //This should be implemented to make the imp. more robust
}
*/


void SerialOutput() {
  //Time to output new data?
  if(millis() - serialLastOutput < serialOutputInterval)
    return;
  serialLastOutput = millis();


  //Write data package to Unity
  Serial.write(StartFlag);    //Flag to indicate start of data package
  Serial.print(millis());     //Write the current "time"
  Serial.print(Delimiter);    //Delimiter used to split values
  Serial.print(EDA);          //Write a value
  Serial.print(Delimiter);    //Write delimiter

  if(NewPulse){
    Serial.print(IBI);        //Only print IBI if a new pulse has been detected
    NewPulse = false;
  } else {
    Serial.print(0);          //else print 0
  }
  
  Serial.print(Delimiter);
  Serial.print(rawPulseSensor);
  Serial.print(Delimiter);
  Serial.print(pressure);
  Serial.print(Delimiter);
  Serial.print(digitalRead(buttonPin));
  Serial.println();           // Write endflag '\n' to indicate end of package

  //For debugging. Comment the lines above and uncomment one of these.
  //Serial.println(analogRead(EDAPin));
  //Serial.println(analogRead(PulsePin));
}

void checkPulseSensor(){
  Signal = analogRead(PulsePin);                  // Read the Pulse Sensor
  rawPulseSensor = Signal;
  unsigned long N = millis() - lastBeatTime;                // Calculate time since we last had a beat

  if (Signal < Threshold && Pulse == true && !Resetting) {      // When the values are going below the threshold, the beat is over
    Pulse = false;                                              // reset the Pulse flag so we are ready for another pulse
    Threshold = Trough + (Peak - Trough) * 0.75;                
    Peak = 0;                                                   // reset for next pulse
    Trough = 1023;                                              // reset for next pulse
    return;
  }
  
  //  Find the trough and the peak (aka. min and max) of the pulse wave (they are used to adjust threshold)
  if (Signal > Peak) {
    Peak = Signal;                           // keep track of highest point in pulse wave   
  }                                          
  if (Signal < Trough) {                     
    Trough = Signal;                         // keep track of lowest point in pulse wave
  }
    
  // Look for a heart beat
  if (N > 400) {                                            // avoid heart rates higher than 60000/400 = 150 bpm. Decrease the value to detect higher bpm
    if ( (Signal > Threshold) && !Pulse && !Resetting) {    // Signal surges up in value every time there is a pulse
      Pulse = true;                                         // set the Pulse flag when we think there is a pulse
      IBI = N;                                              // Set inter-beat interval
      lastBeatTime = millis();                              // keep track of time for next pulse
      NewPulse = true;                                      // Signal that a new pulse has been detected
      TimeoutInterval = 1500UL;                             // Reset the timeout variable
      return;
    }
  }



  if (N > TimeoutInterval) {                          // if 2.5 seconds go by without a beat, reset values and wait for a clear beat
    if(Resetting){
     Threshold = Trough + (Peak - Trough) * 0.75;
     Resetting = false;
    } else {
      Resetting = true;
      Threshold = 512;                       // set threshhold to default value
      Peak = 0;                            // set P default
      Trough = 1023;                          // set T default
      Pulse = false;
    }

    TimeoutInterval += 1500UL;
    
    //TimeoutInterval += 2500;
    //lastBeatTime = millis();               // bring the lastBeatTime up to date (would probably be better to just leave it be, and filter any large values (>2500) from the data)
  }
}
