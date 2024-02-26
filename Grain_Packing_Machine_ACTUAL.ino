/*      CHANGES BETWEEN TEST CODE AND ACTUAL CODE

1. CHANGE DURATIONS FROM MIN:SEC TO HOUR:MIN
    void CompressionLIVE
    int duration_hour = (compress_timer_duration / 60000); // Duration in Hours
    int raw_duration_minute = (compress_timer_duration / 1000); // Duration in minutes

2. PUT ACTUAL VALUE FOR one_bar_grams = 27260
    float One_bar_grams = 500; // 27260g ( 27.26kg)
    
3. ADJUST retracting_time AND removing_time to 20 and 10s RESPECTIVELY
4.
5.

         TO DO


                          
                          1. Calibrate 5kg loadcell & insert value in int calFactor_filling = 53;            n
2.
3.

//Code storage
-This code is hosted in laelmukeni github account
*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <MUIU8g2.h>
#include <SimpleRotary.h>
#include <HX711_ADC.h> // LOADCELL Library
#include <stdlib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// SimpleRotary Library
// https://github.com/mprograms/SimpleRotary
// Pin A, Pin B, Button Pin
SimpleRotary rotary(2, 3, 4);

#define LimitSwitch_Max A0
#define LimitSwitch_Min A1

#define jack_up_btn 23
#define jack_down_btn 22
#define LA_up_btn 25
#define LA_down_btn 24
#define reset_loadcell_btn 6

#define jack_up_signal 50
#define jack_down_signal 51
#define LA_up_signal 52
#define LA_down_signal 53
#define buzzer 7
#define Jack_Down_RPWM 8
#define Jack_Up_LPWM 9

HX711_ADC LoadCell_Filling(48, 49); // DT, SCK
HX711_ADC LoadCell_Compress_1(26, 27);
HX711_ADC LoadCell_Compress_2(28, 29);
HX711_ADC LoadCell_Compress_3(30, 31);
HX711_ADC LoadCell_Compress_4(32, 33);
HX711_ADC LoadCell_Compress_5(34, 35);
HX711_ADC LoadCell_Compress_6(36, 37);
HX711_ADC LoadCell_Compress_7(38, 39);
HX711_ADC LoadCell_Compress_8(40, 41);

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* CS=*/ 10, /* reset=*/ 8);

void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

MUIU8G2 mui;

uint8_t filling_boolean = 1;

/*
  global variables which form the communication gateway between the user interface and the rest of the code
*/

uint8_t pressure_value = 0;
uint8_t cure_hourvalue = 0;
uint8_t cure_minutevalue = 0;
uint8_t core_hourvalue = 0;
uint8_t core_minutevalue = 0;

///////////////////////////////////////////////////////

unsigned long lastTime = 0;
unsigned long lastTime_CompressionLIVE = 0;
unsigned long lastTime_Test_Loadcells = 0;
float pressure = 0;

float preset_pressure = pressure; // (pressure - 1);

float upper_pressure_limit = 0.1;
//int upper_dummy_limit = 15;

int calFactor_filling = 473; // Subject to change with the 5kg load cell
int calFactor_compression1 = 53; // Subject to change for each loadcell (50kg loadcell)
int calFactor_compression2 = 53;
int calFactor_compression3 = 53;
int calFactor_compression4 = 53;
int calFactor_compression5 = 53;
int calFactor_compression6 = 53;
int calFactor_compression7 = 53;
int calFactor_compression8 = 53;

int reset_loadcell_filling_val = 0;
float reset_loadcell_compress_val = 0;
float reset_loadcell_test_val = 0;

int loadcell_Filling_val = 0;
int loadcell_Compress1_val = 0;
int loadcell_Compress2_val = 0;
int loadcell_Compress3_val = 0;
int loadcell_Compress4_val = 0;
int loadcell_Compress5_val = 0;
int loadcell_Compress6_val = 0;
int loadcell_Compress7_val = 0;
int loadcell_Compress8_val = 0;

int loadcell_val_Filling = 0;
int loadcell_val_Filling_Preset = 512;
//int OD = 64; // 64mm
//int ID = 25; // 25mm
//int H = 102; // 102mm
//int loadcell_val_Filling_Preset = (0.0014459 * (sq(OD)-sq(ID)) * H);
float loadcell_val_Filling_perc = 0;

float One_bar_grams = 27789; // 27789g = ( 27.789kg) = 272.61N / 0.0027261m^2 = 100,000N/m^2 = 1 bar

float mould1_pressure = 0;
float mould2_pressure = 0;
float mould3_pressure = 0;
float mould4_pressure = 0;

// mould1_pressure = ( (loadcell_Compress1_val+loadcell_Compress2_val) / One_bar_grams) + 1 - reset_loadcell_compress_val;

String filling_mass_FullStr = "No";
String Compression_display_string = "";

int constant_compens = 6;
int cure_hourvalue_compensate = 0; String cure_hourvalue_compensate_str = "";
int cure_minutevalue_compensate = 0; String cure_minutevalue_compensate_str = "";
//int core_hourvalue_compensate = 0; String core_hourvalue_compensate_str = "";
// The above is not needed since core houre value will never reach past 10 hours, so it has been hard coded and is static.
int core_minutevalue_compensate = 0; String core_minutevalue_compensate_str = "";

int cure_hour_live_compensate = 0; String cure_hour_live_compensate_str = "";
int cure_minute_live_compensate = 0; String cure_minute_live_compensate_str = "";
// The above will be used also for the core since its the same time lapse

// For the compression period
unsigned long compress_timer_start = 0;
unsigned long compress_timer_duration = 0;

// FOR THE STRINGS TO CHANGE VALUE AFTE CERTAIN PERIODS
unsigned long removing_time = 8000; // LINEAR ACTUATOR - 10000
unsigned long retracting_time = 5000; // JACK - 15000
unsigned long reference_time1 = 0;
unsigned long reference_time2 = 0;

// JACK AND LA OVERRIDE BOOLEAN
bool enable_overrride = true;

// JACK DOWN MANUAL PWM

int T_ON = 500; // 500ms (0.5s)

unsigned long duration_ON = 0;
unsigned long duration_OFF = 0;
unsigned long buzzertime_ON = 0;
unsigned long buzzertime_OFF = 0;

bool bool_ready = false;
bool bool_jack_down = false;
bool bool_jack_up = false;

uint8_t mui_hrule(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    u8g2.drawHLine(0, mui_get_y(ui), u8g2.getDisplayWidth());
  }
  return 0;
}

////////////////////////////////////////////////////////////
//              DETAILED FORMS

//             1. FILLING SECTION
uint8_t FillingSection_display(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    u8g2_uint_t x = mui_get_x(ui);
    u8g2_uint_t y = mui_get_y(ui);

    // Reset loadcell value to ignore empty weight
    if( digitalRead(reset_loadcell_btn) == HIGH){
      reset_loadcell_filling_val = loadcell_Filling_val;
    }
    
    u8g2.setCursor(x+5, y);
    u8g2.print("Mass     :");
    u8g2.setCursor(x+60, y); // 
    u8g2.print(loadcell_Filling_val-reset_loadcell_filling_val); // loadcell_val_Filling  loadcell_Compress1_val
    u8g2.setCursor(x+83, y);
    u8g2.print("/");
    u8g2.setCursor(x+90, y);
    u8g2.print(loadcell_val_Filling_Preset); // Variable -> int filling_mass = i; // (Weight sensor)
    u8g2.setCursor(x+110, y);
    u8g2.print("g");

    float preset_value = loadcell_val_Filling_Preset;
    loadcell_val_Filling_perc = ( (loadcell_Filling_val-reset_loadcell_filling_val) / (preset_value/100) ); // loadcell_val_Filling_Preset
    
    u8g2.setCursor(x+5, y+12);
    u8g2.print("Percent :");
    u8g2.setCursor(x+60, y+12);
    u8g2.print(loadcell_val_Filling_perc); // filling_mass/604.092  //  loadcell_val_Filling_perc
    u8g2.setCursor(x+95, y+12);
    u8g2.print("%");

    if( (loadcell_Filling_val-reset_loadcell_filling_val) > loadcell_val_Filling_Preset){
      filling_mass_FullStr = "Yes";
    }else{
      filling_mass_FullStr = "No";
    }   
    u8g2.setCursor(x+5, y+24);
    u8g2.print("Full?     :");
    u8g2.setCursor(x+50, y+24);
    u8g2.print(filling_mass_FullStr); // Variable -> String filling_mass_FullStr = "Yes";
   }   
  return 0;
}


//             2. COMPRESSION SECTION
uint8_t CompressionSection_Page(mui_t *ui, uint8_t msg) {
  compress_timer_start = millis();
  compress_timer_duration = 0;
  duration_ON = millis();
  enable_overrride = true;
  bool_jack_down = false;
  bool_jack_up = false;
  digitalWrite(buzzer, LOW);
}

//             2B. COMPRESSION SECTION - CONFIRM SETTINGS
uint8_t CompressionSection_confirm(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    u8g2_uint_t x = mui_get_x(ui);
    u8g2_uint_t y = mui_get_y(ui);

    // VARIABLES FOR THE SELECTED PRESSURE, CURE AND CORE TIME FOR THE 4 MOULDS
    float val = pressure_value; pressure = val / 10; // To store value as float, so that we can divide and have decimal
    
    u8g2.setCursor(x+5, y);
    u8g2.print("Pressure  :");
    u8g2.setCursor(x+60, y);
    u8g2.print(pressure);
    u8g2.setCursor(x+82, y);
    u8g2.print("Bars");

    u8g2.setCursor(x+5, y+12);
    u8g2.print("Cure Time:");
    u8g2.setCursor(x+60, y+12);
    u8g2.print(cure_hourvalue);
    u8g2.setCursor(x+75, y+12);
    u8g2.print("Hrs");
    u8g2.setCursor(x+95, y+12);
    u8g2.print(cure_minutevalue);
    u8g2.setCursor(x+110, y+12);
    u8g2.print("min");

    u8g2.setCursor(x+5, y+24);
    u8g2.print("Core Time:");
    u8g2.setCursor(x+60, y+24);
    u8g2.print(core_hourvalue);
    u8g2.setCursor(x+75, y+24);
    u8g2.print("Hrs");
    u8g2.setCursor(x+95, y+24);
    u8g2.print(core_minutevalue);
    u8g2.setCursor(x+110, y+24);
    u8g2.print("min");
  }
  return 0;
}

//             2C. COMPRESSION LIVE
uint8_t CompressionSection_LIVE(mui_t *ui, uint8_t msg) {
  enable_overrride = false; // Disable override
  
  if( (millis() - lastTime_CompressionLIVE) >= 200){

    preset_pressure = pressure; // To capture the pressure set by user ( Eg. 3 bars)

   // TIME DURATIONS
    
    if(mould1_pressure > preset_pressure){ // For testing -> loadcell_Filling_val > loadcell_val_Filling_Preset
      compress_timer_duration = millis() - compress_timer_start;
    }else{
      compress_timer_start = millis() - compress_timer_duration;
    }
 
    int duration_hour = (compress_timer_duration / 3600000); // Duration in Hours
    int raw_duration_minute = (compress_timer_duration / 60000); // Duration in minutes
    int duration_minute = raw_duration_minute - 60 * duration_hour; // Corrected duration minutes

    // PRINTING POSITION AUTOCORRECT - LIVE  //////////////////////////////////////////////////////////////////////////
    if(duration_hour < 10){cure_hour_live_compensate = constant_compens; cure_hour_live_compensate_str = "0";}
    else{cure_hour_live_compensate = 0; cure_hour_live_compensate_str = "";}

    if(duration_minute < 10){cure_minute_live_compensate = constant_compens; cure_minute_live_compensate_str = "0";}
    else{cure_minute_live_compensate = 0; cure_minute_live_compensate_str = "";}

    // PRINTING POSITION AUTOCORRECT - PRESET  ///////////////////////////////////////////////////////////////////////

    if(cure_hourvalue >= 10){cure_hourvalue_compensate = 0; cure_hourvalue_compensate_str = "";}
    else{cure_hourvalue_compensate = constant_compens; cure_hourvalue_compensate_str = "0";} // if(cure_hourvalue < 10)
    
    if(cure_minutevalue < 10){cure_minutevalue_compensate = constant_compens; cure_minutevalue_compensate_str = "0";}
    else{cure_minutevalue_compensate = 0; cure_minutevalue_compensate_str = "";}

    if(core_minutevalue < 10){core_minutevalue_compensate = constant_compens; core_minutevalue_compensate_str = "0";}
    else{core_minutevalue_compensate = 0; core_minutevalue_compensate_str = "";}


    //////////////////////////////////    FUNCTIONS WITHIN COMPRESS_LIVE FUNCTION     ////////////////////////////////////////

    //    reset_loadcell_compress_val
    // Reset loadcell value to ignore empty weight
    if( digitalRead(reset_loadcell_btn) == HIGH){
      reset_loadcell_compress_val = mould1_pressure - 1; // Mould 1 pressure will be used to level the others
    }
    
    mould1_pressure = ( (loadcell_Compress1_val+loadcell_Compress2_val) / One_bar_grams) + 1 - reset_loadcell_compress_val;
    mould2_pressure = ( (loadcell_Compress3_val+loadcell_Compress4_val) / One_bar_grams) + 1 - reset_loadcell_compress_val;
    mould3_pressure = ( (loadcell_Compress5_val+loadcell_Compress6_val) / One_bar_grams) + 1 - reset_loadcell_compress_val;
    mould4_pressure = ( (loadcell_Compress7_val+loadcell_Compress8_val) / One_bar_grams) + 1 - reset_loadcell_compress_val;

    // Implementing a PWM speed using the pressure difference between the preset pressure and current pressure
    // Using a maximum PWM feed of 100 out of 255 AND assuming a maximum pressure difference of 3-1=2 bars, we get 100 = (2) * 50, thus the 50 below
    //                                                                                                             255 = 2 * x
    // OTHERWISE, 50 can be used as a constant PWM feed, with various tests
    int pressure_difference_PWM_speed = sqrt(sq(preset_pressure-mould1_pressure)) * 50;
    
    if( (mould1_pressure < preset_pressure) && bool_ready == false ){
      analogWrite(Jack_Down_RPWM, pressure_difference_PWM_speed);
      analogWrite(Jack_Up_LPWM, 0);
    }
    else if(mould1_pressure > (preset_pressure + upper_pressure_limit) ){ ///////   upper_pressure_limit    ///////////////
      analogWrite(Jack_Down_RPWM, 0);
      analogWrite(Jack_Up_LPWM, pressure_difference_PWM_speed);
    }
    else{
      analogWrite(Jack_Down_RPWM, 0);
      analogWrite(Jack_Up_LPWM, 0);
    }

    //  REMOVE CORES 
    if( (duration_hour >= core_hourvalue && duration_minute >= core_minutevalue) || (duration_hour > core_hourvalue) ){
      digitalWrite(LA_down_signal, HIGH);

      Compression_display_string = "Removing";
      if( (millis()-reference_time1) >= removing_time){ // removing_time
        Compression_display_string = "Removed";
      }
    }else{
      digitalWrite(LA_down_signal, LOW);
      Compression_display_string = "";
      reference_time1 = millis();
    }
    
    // RETRACT JACK
    if( (duration_hour >= cure_hourvalue && duration_minute >= cure_minutevalue) || (duration_hour > cure_hourvalue) ){
//      digitalWrite(Jack_Up_LPWM, HIGH);
//      COMMENTED BECAUSE THE JACK HAD SOME FAULT AS OF 22/2/2024
//      LET THE USER TAKE THE JACK UP MANUALLY TO A DESIRED HEIGHT
      
      Compression_display_string = "Retracting";
      if( (millis()-reference_time2) >= retracting_time){ // retracting_time
        Compression_display_string = "Ready";
        bool_ready = true;

        // Make buzzer beep
        if( (millis() - buzzertime_ON) <= T_ON){
          digitalWrite(buzzer, HIGH);
          buzzertime_OFF = millis();
        }else if( (millis() - buzzertime_OFF) <= T_ON){
          digitalWrite(buzzer, LOW);
        }else{
          buzzertime_ON = millis();
        }
        
      }
    }else{
      reference_time2 = millis();
      bool_ready = false;
    } 

    ////////////////////////////////////////////    DISPLAY     ////////////////////////////////////////////////
    
    if ( msg == MUIF_MSG_DRAW ) {
      u8g2_uint_t x = mui_get_x(ui);
      u8g2_uint_t y = mui_get_y(ui);
      
  
      // ROW 1
      u8g2.setCursor(x+3, y);
      u8g2.print("P1:");
      u8g2.setCursor(x+23, y);
      u8g2.print(mould1_pressure ); // mould1_pressure  loadcell_Compress1_val
      u8g2.setCursor(x+45, y);
      u8g2.print("| Cu:");
      u8g2.setCursor(x+70, y);
      u8g2.print(cure_hour_live_compensate_str);
      u8g2.setCursor(x+70+cure_hour_live_compensate, y);
      u8g2.print(duration_hour);
      u8g2.setCursor(x+83, y);
      u8g2.print(cure_minute_live_compensate_str);
      u8g2.setCursor(x+83+cure_minute_live_compensate, y);
      u8g2.print(duration_minute);
      
      u8g2.setCursor(x+97, y);
      u8g2.print("/");
      u8g2.setCursor(x+102, y);
      u8g2.print(cure_hourvalue_compensate_str);
      u8g2.setCursor(x+102+cure_hourvalue_compensate, y);
      u8g2.print(cure_hourvalue);
      u8g2.setCursor(x+116, y);
      u8g2.print(cure_minutevalue_compensate_str);
      u8g2.setCursor(x+116+cure_minutevalue_compensate, y);
      u8g2.print(cure_minutevalue);
  
      // ROW 2
      u8g2.setCursor(x+3, y+12);
      u8g2.print("P2:");
      u8g2.setCursor(x+23, y+12);
      u8g2.print(mould2_pressure);
      u8g2.setCursor(x+45, y+12);
      u8g2.print("| Co:");
      u8g2.setCursor(x+70, y+12);
      u8g2.print(cure_hour_live_compensate_str);
      u8g2.setCursor(x+70+cure_hour_live_compensate, y+12);
      u8g2.print(duration_hour);
      u8g2.setCursor(x+83, y+12);
      u8g2.print(cure_minute_live_compensate_str);
      u8g2.setCursor(x+83+cure_minute_live_compensate, y+12);
      u8g2.print(duration_minute);
      
      u8g2.setCursor(x+97, y+12);
      u8g2.print("/");
      u8g2.setCursor(x+102, y+12);
      u8g2.print("0");
      u8g2.setCursor(x+108, y+12);
      u8g2.print(core_hourvalue);
      u8g2.setCursor(x+116, y+12);
      u8g2.print(core_minutevalue_compensate_str);
      u8g2.setCursor(x+116+core_minutevalue_compensate, y+12);
      u8g2.print(core_minutevalue);
      
      // ROW 3
      u8g2.setCursor(x+3, y+24);
      u8g2.print("P3:");
      u8g2.setCursor(x+23, y+24);
      u8g2.print(mould3_pressure);
      u8g2.setCursor(x+45, y+24);
      u8g2.print("|");
      u8g2.setCursor(x+52, y+24);
      u8g2.print(Compression_display_string);
      u8g2.setCursor(x+101, y+24);
      u8g2.print("(Hrs)");
  
      // ROW 4
      u8g2.setCursor(x+3, y+36);
      u8g2.print("P4:");
      u8g2.setCursor(x+23, y+36);
      u8g2.print(mould4_pressure);
      u8g2.setCursor(x+45, y+36);
      u8g2.print("|");  
    }
    return 0;
    
    lastTime_CompressionLIVE = millis();
  }    
}

///   3. TEST LOADCELLS   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t Test_Loadcells(mui_t *ui, uint8_t msg) {

  if ( msg == MUIF_MSG_DRAW ) {
    u8g2_uint_t x = mui_get_x(ui);
    u8g2_uint_t y = mui_get_y(ui);

    // Reset loadcell values to ignore empty weights
    if( digitalRead(reset_loadcell_btn) == HIGH){
      reset_loadcell_test_val = loadcell_Compress1_val;
    }

    
    // ROW 1  ////////////////////////////////////////
    u8g2.setCursor(x+5, y);
    u8g2.print("M1:");
    u8g2.setCursor(x+30, y);
    u8g2.print((loadcell_Compress1_val-reset_loadcell_test_val)/1000);
    u8g2.setCursor(x+55, y);
    u8g2.print(",");
    u8g2.setCursor(x+65, y);
    u8g2.print((loadcell_Compress2_val-reset_loadcell_test_val)/1000);

    // ROW 2  ////////////////////////////////////////
    u8g2.setCursor(x+5, y+12);
    u8g2.print("M2:");
    u8g2.setCursor(x+30, y+12);
    u8g2.print((loadcell_Compress3_val-reset_loadcell_test_val)/1000);
    u8g2.setCursor(x+55, y+12);
    u8g2.print(",");
    u8g2.setCursor(x+65, y+12);
    u8g2.print((loadcell_Compress4_val-reset_loadcell_test_val)/1000);

     // ROW 3  ////////////////////////////////////////
    u8g2.setCursor(x+5, y+24);
    u8g2.print("M3:");
    u8g2.setCursor(x+30, y+24);
    u8g2.print((loadcell_Compress5_val-reset_loadcell_test_val)/1000);
    u8g2.setCursor(x+55, y+24);
    u8g2.print(",");
    u8g2.setCursor(x+65, y+24);
    u8g2.print((loadcell_Compress6_val-reset_loadcell_test_val)/1000);

    // ROW 4  ////////////////////////////////////////
    u8g2.setCursor(x+5, y+36);
    u8g2.print("M4:");
    u8g2.setCursor(x+30, y+36);
    u8g2.print((loadcell_Compress7_val-reset_loadcell_test_val)/1000);
    u8g2.setCursor(x+55, y+36);
    u8g2.print(",");
    u8g2.setCursor(x+65, y+36);
    u8g2.print((loadcell_Compress8_val-reset_loadcell_test_val)/1000);

  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
muif_t muif_list[] = // The Widget Functions

{
  MUIF_U8G2_FONT_STYLE(0, u8g2_font_helvR08_tr),        /* Regular font */
  MUIF_U8G2_FONT_STYLE(1, u8g2_font_helvB08_tr),        /* bold font */

//  FillingSection_display
  
  MUIF_RO("HR", mui_hrule),   // Underline
  MUIF_U8G2_LABEL(),
  MUIF_RO("GP", mui_u8g2_goto_data),              // Will show data
  MUIF_BUTTON("GC", mui_u8g2_goto_form_w1_pi),    // Will lead to a form
  
  MUIF_U8G2_U8_MIN_MAX("PA", &pressure_value, 15, 30, mui_u8g2_u8_min_max_wm_mud_pi),  // PA = Pressure for Mould A. Use 2 LETTER words. These are NOT ACCEPTED: 3 LETTER WORDS, NUMBERS, SOME SYMBOLS.
  MUIF_U8G2_U8_MIN_MAX("QA", &cure_hourvalue, 0, 48, mui_u8g2_u8_min_max_wm_mud_pi),   //  QA = Cure Hours for Mould A    (Alphabets Q,R,S,T for Mould A, downwards)
  MUIF_U8G2_U8_MIN_MAX("RA", &cure_minutevalue, 0, 59, mui_u8g2_u8_min_max_wm_mud_pi), //  RA = Cure Minutes for Mould A
  MUIF_U8G2_U8_MIN_MAX("SA", &core_hourvalue, 0, 2, mui_u8g2_u8_min_max_wm_mud_pi),    //  SA = Core Hours for Mould A
  MUIF_U8G2_U8_MIN_MAX("TA", &core_minutevalue, 0, 59, mui_u8g2_u8_min_max_wm_mud_pi), //  TA = Core Minutes for Mould A

  /* register custom function to show the data */
  MUIF_RO("SF", FillingSection_display),      // Show my data
  MUIF_RO("SC", CompressionSection_Page),
  MUIF_RO("SH", CompressionSection_confirm),
  MUIF_RO("SL", CompressionSection_LIVE),
  MUIF_RO("ST", Test_Loadcells),

  /* a button for the menu... */
  MUIF_BUTTON("GO", mui_u8g2_btn_goto_wm_fi)  // Ok Button Widget
};


fds_t fds_data[] = // The Layouts

  // --- FORMS ---
  // FORM 1 -  HOME
  // FORM 2 -  FILLING SECTION
  // FORM 3 -  COMPRESSION SECTION
  // FORM 4 -  ENTER SETTINGS
  // FORM 5 -  CONFIRM SETTINGS
  // FORM 6 -  --COMPRESS--
  
  // MAIN PAGE LAYOUT
  MUI_FORM(1) // (1)
  MUI_STYLE(1) // Bold Text Font
  MUI_LABEL(50, 8, "Home") // MUI_LABEL - The Title
  MUI_STYLE(0) // Regular Text Font
  MUI_XY("HR", 0, 11) // The Underline
  MUI_DATA("GP",    // MUI_DATA - The List of items
           MUI_2 "1. Filling Section|"
           MUI_3 "2. Compression Section|"
           MUI_7 "3. Test Loadcells"
          )
  MUI_XYA("GC", 5, 24, 0) // Positions of the List Items
  MUI_XYA("GC", 5, 36, 1)
  MUI_XYA("GC", 5, 60, 2)

  // 1. FILLING SECTION ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  MUI_FORM(2)    // Form to be generated when MUI_xx from the MUI_DATA above is selected
  MUI_STYLE(1)    // Bold Text Font
  MUI_LABEL(5, 8, "1. Filling Section")   // Title
  MUI_XY("HR", 0, 11)  // Underline
  MUI_STYLE(0)    // Regular Text Font
  MUI_XY("SF", 0, 23)   // SF (Show Filling data): Display (show) the data/text
  MUI_XYAT("GO", 114, 60, 1, " Ok ")

  // 3. TEST LOADCELLS ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  MUI_FORM(7)    // Form to be generated when MUI_xx from the MUI_DATA above is selected
  MUI_STYLE(1)    // Bold Text Font
  MUI_LABEL(5, 8, "3. Test Loadcells (Kgs)")   // Title
  MUI_XY("HR", 0, 11)  // Underline
  MUI_STYLE(0)    // Regular Text Font
  MUI_XY("ST", 0, 23)   // Display (show) the data/text
  MUI_XYAT("GO", 105, 60, 1, "Back")


  // 2. COMPRESSION SECTION ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  MUI_FORM(3) // (1)
  MUI_STYLE(1) // Bold Text Font
  MUI_LABEL(5, 8, "2. Compression Section") // MUI_LABEL - The Title
  MUI_STYLE(0) // Regular Text Font
  MUI_XY("HR", 0, 11) // The Underline
  MUI_XY("SC", 0, 23)
  MUI_DATA("GP",    // MUI_DATA - The List of items
           MUI_4 "A. Enter Settings|"
           MUI_5 "B. Confirm Settings|"
           MUI_6 "  >>>>  Compress  <<<<"
          )
  MUI_XYA("GC", 5, 24, 0) // Positions of the List Items
  MUI_XYA("GC", 5, 36, 1)
  MUI_XYA("GC", 5, 48, 2)

  MUI_XYAT("GO", 110, 60, 1, "Back")

  
  // 2A. ENTER SETTINGS ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  MUI_FORM(4)    // Form to be generated when MUI_xx from the MUI_DATA above is selected
  MUI_STYLE(1)    // Bold Text Font
  MUI_LABEL(5, 8, "2A. Enter Settings")   // Title
  MUI_XY("HR", 0, 11) // Underline
  MUI_STYLE(0)    // Regular Text Font
  MUI_LABEL(5, 23, "Pressure  :")  // Tags
  MUI_LABEL(5, 36, "Cure Time:")  //
  MUI_LABEL(5, 49, "Core Time:") //

  MUI_XY("PA", 60, 23)        // (Type of selection: Number), (X-pstn), (Y-pstn)
  MUI_LABEL(75, 23, "x10^-1 Bar")

  MUI_XY("QA", 60, 36)//60,36
  MUI_LABEL(75, 36, "Hrs")
  MUI_XY("RA", 95, 36)
  MUI_LABEL(110, 36, "min")

  MUI_XY("SA", 60, 49)
  MUI_LABEL(70, 49, "Hrs")
  MUI_XY("TA", 92, 49)
  MUI_LABEL(105, 49, "min")

  MUI_XYAT("GO", 114, 60, 3, " Ok ")

 
  // 2B. CONFIRM SETTINGS ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  MUI_FORM(5)    // Form to be generated when MUI_xx from the MUI_DATA above is selected
  MUI_STYLE(1)    // Bold Text Font
  MUI_LABEL(5, 8, "2B. Confirm Settings")   // Title
  MUI_XY("HR", 0, 11)  // Underline
  MUI_STYLE(0)    // Regular Text Font
  MUI_XY("SH", 0, 23)   // Display (show) the data/text
  MUI_XYAT("GO", 114, 60, 3, " Ok ")

  // 2C. COMPRESS LIVE ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  MUI_FORM(6)    // Form to be generated when MUI_xx from the MUI_DATA above is selected
  MUI_STYLE(1)    // Bold Text Font
  MUI_LABEL(5, 8, "2C. Compression LIVE")   // Title
  MUI_XY("HR", 0, 11)  // Underline
  MUI_STYLE(0)    // Regular Text Font
  MUI_XY("SL", 0, 23)   // Display (show) the data/text
  MUI_XYAT("GO", 105, 60, 3, "Terminate")
  ;

void setup(void) { 
  Serial.begin(9600);
  u8g2.begin();
  mui.begin(u8g2, fds_data, muif_list, sizeof(muif_list) / sizeof(muif_t));
  mui.gotoForm(/* form_id= */ 1, /* initial_cursor_position= */ 0);
 
  pinMode(jack_up_btn, INPUT);
  pinMode(jack_down_btn, INPUT);
  pinMode(LA_up_btn, INPUT);
  pinMode(LA_down_btn, INPUT);
  pinMode(reset_loadcell_btn, INPUT);
  pinMode(LimitSwitch_Max, INPUT);
  pinMode(LimitSwitch_Min, INPUT);
  
  pinMode(jack_up_signal, OUTPUT); // No longer used non-PWM Pins. Previously used with old remote motor driver
  pinMode(jack_down_signal, OUTPUT);
  pinMode(LA_up_signal, OUTPUT);
  pinMode(LA_down_signal, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(Jack_Down_RPWM, OUTPUT); // New Pins for Jack Up and Down with PWM functionality
  pinMode(Jack_Up_LPWM, OUTPUT);

  digitalWrite(Jack_Up_LPWM, LOW);
  digitalWrite(Jack_Down_RPWM, LOW);

  LoadCell_Filling.begin(); // start connection to HX711
  LoadCell_Filling.start(1); // load cells gets 1 ms time to stabilize
  LoadCell_Filling.setCalFactor(calFactor_filling); //Set calibration factor specific to loadcell

  LoadCell_Compress_1.begin();LoadCell_Compress_1.start(1);LoadCell_Compress_1.setCalFactor(calFactor_compression1);
  LoadCell_Compress_2.begin();LoadCell_Compress_2.start(1);LoadCell_Compress_2.setCalFactor(calFactor_compression2);
  LoadCell_Compress_3.begin();LoadCell_Compress_3.start(1);LoadCell_Compress_3.setCalFactor(calFactor_compression3);
  LoadCell_Compress_4.begin();LoadCell_Compress_4.start(1);LoadCell_Compress_4.setCalFactor(calFactor_compression4);
  LoadCell_Compress_5.begin();LoadCell_Compress_5.start(1);LoadCell_Compress_5.setCalFactor(calFactor_compression5);
  LoadCell_Compress_6.begin();LoadCell_Compress_6.start(1);LoadCell_Compress_6.setCalFactor(calFactor_compression6);
  LoadCell_Compress_7.begin();LoadCell_Compress_7.start(1);LoadCell_Compress_7.setCalFactor(calFactor_compression7);
  LoadCell_Compress_8.begin();LoadCell_Compress_8.start(1);LoadCell_Compress_8.setCalFactor(calFactor_compression8);
 
}

uint8_t is_redraw = 1;
uint8_t rotary_event = 0; // 0 = not turning, 1 = CW, 2 = CCW
uint8_t push_event = 0; // 0 = not pushed, 1 = pushed

void detect_events(void) {
  uint8_t tmp;

  // 0 = not pushed, 1 = pushed
  tmp = rotary.push();
  if ( tmp != 0 )         // only assign the push event, never clear the event here
    push_event = tmp;

  // 0 = not turning, 1 = CW, 2 = CCW
  tmp = rotary.rotate();
  if ( tmp != 0 )       // only assign the rotation event, never clear the event here
    rotary_event = tmp;
}

void handle_events(void) {
  // 0 = not pushed, 1 = pushed
  if ( push_event == 1 ) {
    mui.sendSelect();
    is_redraw = 1;
    push_event = 0;
  }

  // 0 = not turning, 1 = CW, 2 = CCW
  if ( rotary_event == 1 ) {
    mui.nextField();
    is_redraw = 1;
    rotary_event = 0;
  }

  if ( rotary_event == 2 ) {
    mui.prevField();
    is_redraw = 1;
    rotary_event = 0;
  }
}

//     LOADCELLS FUNCTION
int LoadCells(){
  LoadCell_Filling.update(); // retrieves data from the load cell
  LoadCell_Compress_1.update();
  LoadCell_Compress_2.update();
  LoadCell_Compress_3.update();
  LoadCell_Compress_4.update();
  LoadCell_Compress_5.update();
  LoadCell_Compress_6.update();
  LoadCell_Compress_7.update();
  LoadCell_Compress_8.update();
  
  loadcell_Filling_val = LoadCell_Filling.getData(); // get output value
  loadcell_Compress1_val = LoadCell_Compress_1.getData();
  loadcell_Compress2_val = LoadCell_Compress_2.getData();
  loadcell_Compress3_val = LoadCell_Compress_3.getData();
  loadcell_Compress4_val = LoadCell_Compress_4.getData();
  loadcell_Compress5_val = LoadCell_Compress_5.getData();
  loadcell_Compress6_val = LoadCell_Compress_6.getData();
  loadcell_Compress7_val = LoadCell_Compress_7.getData();
  loadcell_Compress8_val = LoadCell_Compress_8.getData();
}


void loop(void) {

  // Additional lines of Code incorporating BTS7960 Motor Driver
  // WHILE loops to stop the jack using its NC Limit Switches being read at the analog inputs A0 and A1

  while(analogRead(LimitSwitch_Max) > 512){
//    analogWrite(Jack_Down_RPWM, 0);
    digitalWrite(Jack_Down_RPWM, LOW);
//    digitalWrite(Jack_Up_LPWM, HIGH);
  }
  while(analogRead(LimitSwitch_Min) > 512){
//    analogWrite(Jack_Up_LPWM, 0);
    digitalWrite(Jack_Up_LPWM, LOW);
//    digitalWrite(Jack_Down_RPWM, HIGH);
  }
  
  if( (digitalRead(jack_up_btn) == HIGH) && enable_overrride==true){digitalWrite(Jack_Up_LPWM, HIGH);
  }else{digitalWrite(Jack_Up_LPWM, LOW);}

  if( (digitalRead(jack_down_btn) == HIGH) && enable_overrride==true){digitalWrite(Jack_Down_RPWM, HIGH);
  }else{digitalWrite(Jack_Down_RPWM, LOW);}

  if( (digitalRead(LA_up_btn) == HIGH) && enable_overrride==true){digitalWrite(LA_up_signal, HIGH);
  }else{digitalWrite(LA_up_signal, LOW);}

  if( (digitalRead(LA_down_btn) == HIGH) && enable_overrride==true){digitalWrite(LA_down_signal, HIGH);
  }else{digitalWrite(LA_down_signal, LOW);}

//  if(bool_jack_down == true){
//    if( (millis() - duration_ON) <= T_ON){
//      digitalWrite(jack_down_signal, HIGH);
//      duration_OFF = millis();
//    }else if( (millis() - duration_OFF) <= T_ON){
//      digitalWrite(jack_down_signal, LOW);
//    }else{
//      duration_ON = millis();
//    }
//  }
//  if(bool_jack_up == true){
//    if( (millis() - duration_ON) <= T_ON){
//      digitalWrite(jack_up_signal, HIGH);
//      duration_OFF = millis();
//    }else if( (millis() - duration_OFF) <= T_ON){
//      digitalWrite(jack_up_signal, LOW);
//    }else{
//      duration_ON = millis();
//    }
//  }
//}

  
  /* check whether the menu is active */
  if ( mui.isFormActive() ) {

    /* update the display content, if the redraw flag is set */
    if ( is_redraw ) {
      u8g2.firstPage();
      do {
        detect_events();
        mui.draw();
        detect_events();
      } while ( u8g2.nextPage() );
      is_redraw = 0;                    /* clear the redraw flag */
    }
    detect_events();
    handle_events();

    /* update the stop watch timer */
    if ( filling_boolean != 0 ) {
      LoadCells();
      is_redraw = 1;
    }

  }
  else {
    /* the menu should never become inactive, but if so, then restart the menu system */
    mui.gotoForm(/* form_id= */ 1, /* initial_cursor_position= */ 0);
  }
}
