#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>
#include <Servo.h>

//*********** Used PINs ***************
#define PIN_ENABLE      8 //Mode
#define PIN_FC_X        9 
#define PIN_FC_Y        10
#define PIN_FC_Z        11
#define PIN_DIN_AUX     9 
#define PIN_TOOL        10
#define PIN_UP_DOWN     11
#define PIN_ROT_BUT_D0  13
#define PIN_ROT_BUT_D1  12

#define PIN_SERVO  14 //A3
#define PIN_HOLD  17  //A0

#define PIN_STEP_X  2
#define PIN_DIR_X   5
#define PIN_STEP_Y  3
#define PIN_DIR_Y   6
#define PIN_STEP_Z  4
#define PIN_DIR_Z   7

//*********** Joystick center values and zero threshold ***************
#define JOY_X_CENTER 499
#define JOY_Y_CENTER 486
#define JOY_ZERO_THRESHOLD 20

//***** Rotary button increment of motor steps ***************
#define ROT_SWITCH_STEP_INCR 1

//******* Periods in ms *****************
#define LCD_PRINT_PERIOD 100.0
#define JOY_READ_PERIOD 10.0
#define LCD_SERIAL_PERIOD 50.0

//******* Stepper multipliers ****************
#define X_STEPPER_MULT 4
#define Y_STEPPER_MULT 4
#define Z_STEPPER_MULT 4

//***** Speeds and accelerations in steps **************
#define X_HOMING_SPEED (1000 * X_STEPPER_MULT)
#define X_MAX_SPEED_STP_SEC (1000 * X_STEPPER_MULT)
#define X_CTRL_SPEED_STP_SEC (X_MAX_SPEED_STP_SEC/2)
#define Y_HOMING_SPEED (1000 * Y_STEPPER_MULT)
#define Y_MAX_SPEED_STP_SEC (1000 * Y_STEPPER_MULT)
#define Y_CTRL_SPEED_STP_SEC (Y_MAX_SPEED_STP_SEC/2)
#define Z_HOMING_SPEED (2000 * Z_STEPPER_MULT)
#define Z_MAX_SPEED_STP_SEC (2000 * Z_STEPPER_MULT)
#define Z_CTRL_SPEED_STP_SEC (Z_MAX_SPEED_STP_SEC/2)

#define X_ACCEL_STP_SEC2 (50 * X_STEPPER_MULT)
#define Y_ACCEL_STP_SEC2 (50 * Y_STEPPER_MULT)
#define Z_ACCEL_STP_SEC2 (50 * Z_STEPPER_MULT)

#define JOY_GAIN 5

#define HOMING_MODE 0
#define CONTROL_MODE 1

#define X_MAX_STEPS (6600 * X_STEPPER_MULT)
#define Y_MAX_STEPS (4415 * Y_STEPPER_MULT)
#define Z_MAX_STEPS (4000 * Z_STEPPER_MULT)
#define TOOL_MIN_VAL 0
#define TOOL_MAX_VAL 180

//*********** Calibration ****************
#define X_STEPS_TO_MM_COEF 0.010625
#define Y_STEPS_TO_MM_COEF 0.010645
#define Z_STEPS_TO_MM_COEF 0.0025

//*********** Serial config ***************
#define SERIAL_BAUDRATE 115200

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// Define some steppers and the pins the will use
AccelStepper stepperX(AccelStepper::FULL2WIRE, PIN_STEP_X, PIN_DIR_X);
AccelStepper stepperY(AccelStepper::FULL2WIRE, PIN_STEP_Y, PIN_DIR_Y);
AccelStepper stepperZ(AccelStepper::FULL2WIRE, PIN_STEP_Z, PIN_DIR_Z);
Servo toolServo;  

int x_ref = 0, y_ref = 0, z_ref = 0, tool_ref = 0;
int x_offset = 0, y_offset = 0, z_offset = 0, tool_offset = 0;
int prev_x_ref = 0, prev_y_ref = -1000, prev_z_ref = 1000, prev_tool_ref = -1000;
int sel_axle = 0, prev_sel_axle = -1;
int x_joy, y_joy;
int x_spd = X_CTRL_SPEED_STP_SEC, y_spd = Y_CTRL_SPEED_STP_SEC, z_spd = Z_CTRL_SPEED_STP_SEC;
int rotary_butt_incr = 0;

unsigned long last_tstamp_lcd = 0;
unsigned long last_tstamp_joy = 0;
unsigned long last_tstamp_ctrl = 0;
unsigned long last_tstamp_serial = 0;

int mode = HOMING_MODE;

bool x_homed = false, y_homed = false, z_homed = false;

char lcd_buffer[20] = "                    ";

bool joy_ctr = false;

bool x_ref_reached = false;
bool y_ref_reached = false;
bool z_ref_reached = false;
bool ref_reached = false;

void setup()
{
  //********* Pin config ***********
  pinMode(PIN_ENABLE, INPUT); 
  pinMode(PIN_UP_DOWN, INPUT_PULLUP); 
  pinMode(PIN_TOOL, INPUT_PULLUP); 
  pinMode(PIN_DIN_AUX, INPUT_PULLUP);
  pinMode(PIN_ROT_BUT_D0, INPUT_PULLUP); 
  pinMode(PIN_ROT_BUT_D1, INPUT_PULLUP);
  pinMode(PIN_HOLD, OUTPUT);

  //******* Time stamp initialization ***********
  last_tstamp_lcd = millis();
  last_tstamp_joy = last_tstamp_lcd;
  last_tstamp_ctrl = last_tstamp_lcd;
  last_tstamp_serial = last_tstamp_lcd;

  //****** LCD initialization *************
  lcd.init();                      
  lcd.backlight();

  //********* Stepper motors initialization *****************
  stepperX.setMaxSpeed(X_MAX_SPEED_STP_SEC);
  stepperY.setMaxSpeed(Y_MAX_SPEED_STP_SEC);
  stepperZ.setMaxSpeed(Z_MAX_SPEED_STP_SEC);

  stepperX.setAcceleration(X_ACCEL_STP_SEC2);
  stepperY.setAcceleration(Y_ACCEL_STP_SEC2);
  stepperZ.setAcceleration(Z_ACCEL_STP_SEC2);

  stepperX.setCurrentPosition(0);
  stepperY.setCurrentPosition(0);
  stepperZ.setCurrentPosition(0);
  
  x_homed = false;
  y_homed = false;
  z_homed = false;

  toolServo.attach(PIN_SERVO);

  toolServo.write((TOOL_MAX_VAL - TOOL_MIN_VAL)/2);
     
  //attachInterrupt(digitalPinToInterrupt(PIN_ROT_BUT_D0), rotaryButtonInterruptCallback, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(PIN_ROT_BUT_D1), rotaryButtonInterruptCallback, CHANGE);

  // Open serial communications and wait for port to open:
  Serial.begin(SERIAL_BAUDRATE);
}

void loop()
{
  switch(mode){
    case HOMING_MODE:
    {
       //Wait until no simultaneous combination of PIN_UP_DOWN and PIN_DIN_AUX buttons are pressed
       while ((!digitalRead(PIN_UP_DOWN)) || (!digitalRead(PIN_DIN_AUX)));

       //Display "homing under process" message
       lcd.setCursor(0,0);
       lcd.print("********************");
       lcd.setCursor(0,1);
       lcd.print("*      HOMING      *");
       lcd.setCursor(0,2);
       lcd.print("*  Please wait ... *");
       lcd.setCursor(0,3);
       lcd.print("********************");

       //Home X axle
       while(digitalRead( PIN_FC_X )){
          stepperX.setSpeed(-X_HOMING_SPEED);
          stepperX.runSpeed();
       }
       while(!digitalRead( PIN_FC_X )){
          stepperX.setSpeed(X_HOMING_SPEED);
          stepperX.runSpeed();
       }
       lcd.setCursor(0,2);
       lcd.print("*  X axle homed    *");

       stepperX.setCurrentPosition(0);  

       //Home Y axle
       while(digitalRead( PIN_FC_Y )){
          stepperY.setSpeed(Y_HOMING_SPEED);
          stepperY.runSpeed();
       }
       while(!digitalRead( PIN_FC_Y )){
          stepperY.setSpeed(-Y_HOMING_SPEED);
          stepperY.runSpeed();
       }
       lcd.setCursor(0,2);
       lcd.print("*  Y axle homed    *");

       stepperY.setCurrentPosition(Y_MAX_STEPS);

       //Home Z axle
       while (digitalRead( PIN_FC_Z )){
          stepperZ.setSpeed(Z_HOMING_SPEED);
          stepperZ.runSpeed();
       }
       while(!digitalRead( PIN_FC_Z )){
          stepperZ.setSpeed(-Z_HOMING_SPEED);
          stepperZ.runSpeed();
       }
       lcd.setCursor(0,2);
       lcd.print("*  Z axle homed    *");

       stepperZ.setCurrentPosition(0);        

       //Initialization status
       x_ref = X_MAX_STEPS/2;  
       prev_x_ref = -1000;
       y_ref = Y_MAX_STEPS/2;
       prev_y_ref = -1000;
       z_ref = 0;
       prev_z_ref = -1000;
       tool_ref = (TOOL_MAX_VAL - TOOL_MIN_VAL)/2;
       prev_tool_ref = -1000;
     
       sel_axle = 0; prev_sel_axle = -1;
       mode = CONTROL_MODE;
       joy_ctr = false;

       //Display message while going to the initialization position
       lcd.setCursor(6,0);
       lcd.print("   ***********");
       lcd.setCursor(6,1);
       lcd.print("   * GOING TO ");
       lcd.setCursor(6,2);
       lcd.print("   * INITIAL  ");
       lcd.setCursor(9,3);
       lcd.print("* POSITION ");

    }
    break;
    case CONTROL_MODE: 
    {
      //Determine rotation button status
      rotary_butt_incr = getIncrRotBut();

      //Measure time since last analog joystick reading
      unsigned long clocktime = millis();//Current time
      float delta_time = float(clocktime) - float(last_tstamp_joy);

      if (fabs(delta_time) > JOY_READ_PERIOD)//Perform analog joystick reading periodically
      {
        //Read joystick values
        x_joy = analogRead(A1);
        y_joy = analogRead(A2);

        //Joystic value with respect to center position
        int delta_x_joy = (x_joy - JOY_X_CENTER);
        int delta_y_joy = (y_joy - JOY_Y_CENTER);

        if ( delta_x_joy > JOY_ZERO_THRESHOLD){
          sel_axle = 0;
          rotary_butt_incr = 0;
          x_ref = X_MAX_STEPS;
          x_spd = JOY_GAIN * abs(delta_x_joy);
          joy_ctr = true;
        }else if (delta_x_joy < -JOY_ZERO_THRESHOLD){
          sel_axle = 0;    
          rotary_butt_incr = 0;
          x_ref = 0;
          x_spd = JOY_GAIN * abs(delta_x_joy);
          joy_ctr = true;
        }else if ( delta_y_joy > JOY_ZERO_THRESHOLD){
          if (digitalRead(PIN_UP_DOWN)){
            sel_axle = 1;
            y_ref = Y_MAX_STEPS;            
            y_spd = JOY_GAIN * abs(delta_y_joy);
          }else{
            sel_axle = 2;
            z_ref = 0;                    
            z_spd = JOY_GAIN * abs(delta_y_joy);    
          }
          rotary_butt_incr = 0;
          joy_ctr = true;
        }else if (delta_y_joy < -JOY_ZERO_THRESHOLD){
          if (digitalRead(PIN_UP_DOWN)){
            sel_axle = 1;    
            y_ref = 0;
            y_spd = JOY_GAIN * abs(delta_y_joy);
          }else{
            sel_axle = 2;
            z_ref = -Z_MAX_STEPS;                        
            z_spd = JOY_GAIN * abs(delta_y_joy);    
          }
            
          rotary_butt_incr = 0;
          joy_ctr = true;
        }else if (joy_ctr){//When comming back to the center position of the joystick stop by setting the reference as current position
           x_ref = stepperX.currentPosition();
           x_spd = X_CTRL_SPEED_STP_SEC;
           y_ref = stepperY.currentPosition();
           y_spd = Y_CTRL_SPEED_STP_SEC;
           z_ref = stepperZ.currentPosition();
           z_spd = Z_CTRL_SPEED_STP_SEC;
           joy_ctr = false;//Do it only once
        }

        last_tstamp_joy = clocktime;  
      }

      //Apply the computed position increment to the selected axle
      switch(sel_axle){
        case 0:
          x_ref +=(X_STEPPER_MULT * rotary_butt_incr); rotary_butt_incr = 0;
          if (x_ref < 0) x_ref = 0;
          else if (x_ref > X_MAX_STEPS) x_ref = X_MAX_STEPS;
          break;
        case 1:
          y_ref +=(Y_STEPPER_MULT * rotary_butt_incr); rotary_butt_incr = 0;
          if (y_ref < 0) y_ref = 0;
          else if (y_ref > Y_MAX_STEPS) y_ref = Y_MAX_STEPS;
          break;
        case 2:
          z_ref +=(Z_STEPPER_MULT * rotary_butt_incr); rotary_butt_incr = 0;
          if (z_ref > 0) z_ref = 0;
          else if (z_ref < -Z_MAX_STEPS) z_ref = -Z_MAX_STEPS;
          break;
        case 3:
          tool_ref +=rotary_butt_incr; rotary_butt_incr = 0;
          if (tool_ref < TOOL_MIN_VAL) tool_ref = TOOL_MIN_VAL;
          else if (tool_ref > TOOL_MAX_VAL) tool_ref = TOOL_MAX_VAL;
          break;
      }

      //Move X axle towards the reference at max speed
      int curr_pos = stepperX.currentPosition();
      if ( curr_pos < x_ref ){
          stepperX.setSpeed(x_spd);
          stepperX.runSpeed();
          x_ref_reached = false;
      }else if (curr_pos > x_ref){
          stepperX.setSpeed(-x_spd);
          stepperX.runSpeed(); 
          x_ref_reached = false;   
      }else x_ref_reached = true;
  
      //Move Y axle towards the reference at max speed
      curr_pos = stepperY.currentPosition();
      if ( curr_pos < y_ref ){
          stepperY.setSpeed(y_spd);
          stepperY.runSpeed();
          y_ref_reached = false;
      }else if (curr_pos > y_ref){
          stepperY.setSpeed(-y_spd);
          stepperY.runSpeed();    
          y_ref_reached = false;
      }else  y_ref_reached = true;

      //Move Z axle towards the reference at max speed
      curr_pos = stepperZ.currentPosition();
      if ( curr_pos < z_ref ){
          stepperZ.setSpeed(z_spd);
          stepperZ.runSpeed();
          z_ref_reached = false;
      }else if (curr_pos > z_ref){
          stepperZ.setSpeed(-z_spd);
          stepperZ.runSpeed();    
          z_ref_reached = false;
      }else  z_ref_reached = true;

      toolServo.write(tool_ref);

      if ((!digitalRead(PIN_UP_DOWN)) && (!digitalRead(PIN_DIN_AUX))){//If pushing simultaneously up-down button and din_aux then enter in "homing mode"
        mode = HOMING_MODE;
      }else if (!digitalRead(PIN_UP_DOWN)){//Only up-down will select Y axle 
        sel_axle = 2;
      }else if (!digitalRead(PIN_TOOL)){//tool button will select tool axle
        sel_axle = 3;
      }else if (!digitalRead(PIN_DIN_AUX)){
        x_offset = stepperX.currentPosition();
        y_offset = stepperY.currentPosition();
        z_offset = stepperZ.currentPosition();
      }

      //Print axle selection only if it changes
      if (prev_sel_axle != sel_axle){
        lcd.setCursor(0,0);
        if (sel_axle == 0){
          lcd.print("[*]X: ");
        }else{
          lcd.print("[ ]X: ");    
        }
        lcd.setCursor(0,1);
        if (sel_axle == 1){
          lcd.print("[*]Y: ");
        }else{
          lcd.print("[ ]Y: ");    
        }
        lcd.setCursor(0,2);
        if (sel_axle == 2){
          lcd.print("[*]Z: ");
        }else{
          lcd.print("[ ]Z: ");    
        }
        lcd.setCursor(0,3);
        if (sel_axle == 3){
          lcd.print("[*]Tool: ");
        }else{
          lcd.print("[ ]Tool: ");    
        }      
      }
      prev_sel_axle = sel_axle;

      //Periodically if reference reached display current status. (We don't print while moving as it can produce perturbations in the kinematics
      ref_reached = x_ref_reached && y_ref_reached && z_ref_reached;


      float x_dist = X_STEPS_TO_MM_COEF * float(stepperX.currentPosition() - x_offset);
      float y_dist = Y_STEPS_TO_MM_COEF * float(stepperY.currentPosition() - y_offset);
      float z_dist = Z_STEPS_TO_MM_COEF * float(stepperZ.currentPosition() - z_offset);

      delta_time = float(clocktime) - float(last_tstamp_serial);
      if (fabs(delta_time) > LCD_SERIAL_PERIOD)
      {
        Serial.print("$");
        Serial.print(x_dist);
        Serial.print(",");
        Serial.print(y_dist);
        Serial.print(",");
        Serial.print(z_dist);
        Serial.print(",");
        Serial.print(tool_ref);
        Serial.println("#");

        last_tstamp_serial = clocktime;
      }

      delta_time = float(clocktime) - float(last_tstamp_lcd);

      if ((fabs(delta_time) > LCD_PRINT_PERIOD) && ref_reached)
      {
        char floatStr[15];
        
        //Print axis values
        dtostrf(x_dist, 14, 2, floatStr);  
        sprintf(lcd_buffer,"%s",floatStr);
        lcd.setCursor(6,0);
        lcd.print(lcd_buffer);
        
        dtostrf(y_dist, 14, 2, floatStr);  
        sprintf(lcd_buffer,"%s",floatStr);
        lcd.setCursor(6,1);
        lcd.print(lcd_buffer);
 
        dtostrf(z_dist, 14, 2, floatStr);  
        sprintf(lcd_buffer,"%s",floatStr);
        lcd.setCursor(6,2);
        lcd.print(lcd_buffer);

        sprintf(lcd_buffer,"%11d",tool_ref);
        lcd.setCursor(9,3);
        lcd.print(lcd_buffer);

        last_tstamp_lcd = clocktime;  
      } 
    }
    break;
  }
}

//*********** Function to get the increment with sign of the rotary button ************

void rotaryButtonInterruptCallback(){
  rotary_butt_incr = getIncrRotBut();
}

int getIncrRotBut(void){
  int incr = 0;

  static bool prev_rot_d0=false;
  static bool prev_rot_d1=false;
  bool pin_rot_d0, pin_rot_d1;

  pin_rot_d0 = bool(digitalRead(PIN_ROT_BUT_D0));
  pin_rot_d1 = bool(digitalRead(PIN_ROT_BUT_D1));
  
  if ((!pin_rot_d0)&&(!pin_rot_d1)) 
  {
        if((prev_rot_d0)&&(!prev_rot_d1))
          incr=-ROT_SWITCH_STEP_INCR;
        else if (((!prev_rot_d0)&&(prev_rot_d1)))
          incr=ROT_SWITCH_STEP_INCR;
  }
  else if ((!pin_rot_d0)&&(pin_rot_d1))
  {
        if((!prev_rot_d0)&&(!prev_rot_d1))
          incr=-ROT_SWITCH_STEP_INCR;
        else if ((prev_rot_d0)&&(prev_rot_d1))
          incr=ROT_SWITCH_STEP_INCR;        
  }
  else if ((pin_rot_d0)&&(pin_rot_d1))
  {
        if((!prev_rot_d0)&&(prev_rot_d1))
          incr=-ROT_SWITCH_STEP_INCR;
        else if ((prev_rot_d0)&&(!prev_rot_d1))
          incr=ROT_SWITCH_STEP_INCR;        
  }
  else if ((pin_rot_d0)&&(!pin_rot_d1)) 
  {
        if((prev_rot_d0)&&(prev_rot_d1))
          incr=-ROT_SWITCH_STEP_INCR;
        else if ((!prev_rot_d0)&&(!prev_rot_d1))
          incr=ROT_SWITCH_STEP_INCR;        
  }
        
  prev_rot_d0=pin_rot_d0;
  prev_rot_d1=pin_rot_d1;
    
  return incr;
}
