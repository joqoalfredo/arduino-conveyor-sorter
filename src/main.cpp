#include <ESP32Servo.h>

  //Conveyor
  constexpr uint8_t ENABLE_PIN = 14; // Green LED
  constexpr uint8_t IN1_PIN = 27; // Blue LED - Forward
  constexpr uint8_t IN2_PIN = 26; // Red LED - Reverse
  uint8_t conveyorSpeed = 100; // Arbitrary value

  //Servo
  constexpr uint8_t DATA_SERVO = 25;
  uint8_t servoSpeed = 100; // Arbitrary value
  Servo kickerServo; // Instantiate the servo object

  //Button
  constexpr uint8_t BUTTON_PIN = 32;

  constexpr uint16_t SERVO_SWEEP_DELAY = 500; //500ms for servo to sweep
  uint32_t rejectStartTimer = 0; // Time when state changed to reject

  //FSM
  enum class SystemState{
    IDLE,
    CONVEYING,
    MEASURING,
    APPROVE,
    REJECT
  };
  SystemState currentState = SystemState::IDLE;

  uint16_t approvedItems = 0; // Up to 60k something items. Enough for simulation and prototype.



class Conveyor{
  private:
  uint8_t enPin;
  uint8_t fwdPin;
  uint8_t revPin;

  public:
  // Constructor
  Conveyor(uint8_t enable, uint8_t in1, uint8_t in2){
    enPin = enable;
    fwdPin = in1;
    revPin = in2;
  }

  // Hardware initialization
  void begin() {
      pinMode(enPin, OUTPUT);
      pinMode(fwdPin, OUTPUT);
      pinMode(revPin, OUTPUT);
      stop(); 
    }

  // Functions
  void moveForward(uint8_t speed){
    analogWrite(enPin, speed);
    digitalWrite(fwdPin, HIGH);
    digitalWrite(revPin, LOW);
  }

  void stop(){
    analogWrite(enPin, 0);
    digitalWrite(fwdPin, LOW);
    digitalWrite(revPin, LOW);
  }
};

// Instantiating
Conveyor myConveyor(ENABLE_PIN, IN1_PIN, IN2_PIN);



void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  kickerServo.setPeriodHertz(50);

  kickerServo.attach(DATA_SERVO, 500, 2400); 
  kickerServo.write(0); 

  myConveyor.begin(); 

  pinMode(BUTTON_PIN, INPUT_PULLUP);
}



void loop() {
  uint32_t currentTime = millis();

  switch (currentState){
    case SystemState::IDLE:
      Serial.println("State: IDLE");
      Serial.println("State changed to: CONVEYING");
      currentState = SystemState::CONVEYING; // Auto start.
      break;

    //case SystemState::CONVEYING:
      //myConveyor.moveForward(conveyorSpeed);
      //if(/*sensor detects part*/ true){
        //currentState = SystemState::MEASURING;
      //}
      //break; // Case commented for testing purposes
    
    case SystemState::CONVEYING:    // Testing case
      // State actions
      myConveyor.moveForward(conveyorSpeed);
      
      // We stay in CONVEYING forever, UNTIL the button is pressed
      if(digitalRead(BUTTON_PIN) == LOW){ 
        // Entry actions for REJECT
        rejectStartTimer = currentTime;
        myConveyor.stop();
        kickerServo.write(90); // Rotate servo to 90°
        Serial.println("State changed to: REJECT");
        currentState = SystemState::REJECT;
      }
      break;

    case SystemState::MEASURING:
      Serial.println("State: MEASURING");
      if(digitalRead(BUTTON_PIN) == LOW){ // Button simulating part out of tolerance
        rejectStartTimer = currentTime;
        currentState = SystemState::REJECT;
      } else{
        currentState = SystemState::APPROVE;
      }
      break;

    case SystemState::APPROVE:
      Serial.println("State: APPROVE");
      approvedItems++;
      Serial.print("Approved Items: ");
      Serial.println(approvedItems);
      currentState = SystemState::CONVEYING;
      break;

    case SystemState::REJECT:
      if(currentTime - rejectStartTimer >= 500){
        // Entry actions for CONVEYING
        kickerServo.write(0); // Rotate servo back to 0°
        Serial.println("State changed to: CONVEYING");
        currentState = SystemState::CONVEYING;
      }
      break;

  }

  delay(10); // this speeds up the simulation
}