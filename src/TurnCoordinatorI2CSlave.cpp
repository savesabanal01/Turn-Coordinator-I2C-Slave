#include <Arduino.h>
#include "I2C_slave.h"
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include <tc_main_gauge.h>
#include <tc_plane.h>
#include <slip_ball.h>
#include <slip_center_line.h>
#include <instrument_bezel.h>
#include <logo.h>
#define USE_HSPI_PORT
#define DEG2RAD 0.0174533
#define I2C_MOBIFLIGHT_ADDR 0x26
#define PANEL_COLOR 0x7BEE

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

TFT_eSprite TCmainSpr = TFT_eSprite(&tft);    // Main Sprite for Turn Coordinator
TFT_eSprite TCPlaneSpr = TFT_eSprite(&tft);   // Sprite for Turn Coordinator Airplane
TFT_eSprite slipBallSpr = TFT_eSprite(&tft);  // Sprite for slip ball
TFT_eSprite slipCtrLnSpr = TFT_eSprite(&tft); // Sprite for slip center line

float turnAngle = 0;                     // angle of the turn angle from the simulator Initial Value
double slipDegree = 0;                   // angle of the slip from the simulator
float instrumentBrightnessRatio = 1;     // instrument brightness ratio from sim
int instrumentBrightness = 255;            // instrument brightness based on ratio. Value between 0 - 255
float prevInstrumentBrightnessRatio = 0; // previous value of instrument brightness. If no change do not set instrument brightness to avoid flickers
float ballXPos = 0;                      // X position of the ball
float ballYPos = 0;                      // YPosition of the ball
float startTime = 0;
float endTime = 0;
float fps = 0;

int16_t messageID = 0;            // will be set to the messageID coming from the connector
char message[MAX_LENGTH_MESSAGE]; // contains the message which belongs to the messageID
I2C_slave i2c_slave;

// Functions declrations
float scaleValue(float x, float in_min, float in_max, float out_min, float out_max);
void setTurnAngle(float angle);
void setInstrumentBrightnessRatio(float ratio);
void setSlipDegree(double value);
void checkI2CMesage();

void setup()
{
  Serial.begin(115200);

  // init I2C busses
  i2c_slave.init(I2C_MOBIFLIGHT_ADDR, 400000);
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); // built in LED on arduino board will turn on and off with the status of the beacon light
  digitalWrite(LED_BUILTIN, HIGH);

  // delay(1000); // wait for serial monitor to open
  digitalWrite(LED_BUILTIN, LOW);

  i2c_slave.init(I2C_MOBIFLIGHT_ADDR, 400000);
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); // built in LED on arduino board will turn on and off with the status of the beacon light
  digitalWrite(LED_BUILTIN, HIGH);

  // delay(1000); // wait for serial monitor to open
  digitalWrite(LED_BUILTIN, LOW);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(PANEL_COLOR);
  tft.setPivot(320, 160);
  tft.setSwapBytes(true);
  tft.pushImage(160, 80, 160, 160, logo);
  delay(3000);
  tft.fillScreen(PANEL_COLOR);
  tft.fillCircle(240, 160, 160, TFT_BLACK);

  tft.setSwapBytes(true);
  tft.pushImage(80, 0, 320, 320, instrument_bezel, TFT_BLACK);

}

void loop()
{

  checkI2CMesage();

  startTime = millis();
  TCmainSpr.createSprite(320, 320);
  TCmainSpr.setSwapBytes(true);
  TCmainSpr.fillSprite(TFT_BLACK);
  TCmainSpr.pushImage(0, 0, 320, 320, tc_main_gauge);
  TCmainSpr.setPivot(160, 160);

  slipBallSpr.createSprite(slip_ball_width, slip_ball_height);
  slipBallSpr.setSwapBytes(false);
  slipBallSpr.fillSprite(TFT_BLACK);
  slipBallSpr.pushImage(0, 0, slip_ball_width, slip_ball_height, slip_ball);

  ballXPos = scaleValue(slipDegree, 8, -8, 81, 209);
  ballYPos = cos((scaleValue(slipDegree / 2, 8, -8, -116, -244) - (-180)) * DEG_TO_RAD) * 36 + 153; // Approximation based on trial and error
  slipBallSpr.pushToSprite(&TCmainSpr, ballXPos, ballYPos, TFT_BLACK);
  slipBallSpr.deleteSprite();

  slipCtrLnSpr.createSprite(slip_center_line_width, slip_center_line_height);
  slipCtrLnSpr.setSwapBytes(false);
  slipCtrLnSpr.fillSprite(TFT_BLACK);
  slipCtrLnSpr.pushImage(0, 0, slip_center_line_width, slip_center_line_height, slip_center_line);
  slipCtrLnSpr.pushToSprite(&TCmainSpr, 144, 190, TFT_BLACK);
  slipCtrLnSpr.deleteSprite();

  TCPlaneSpr.createSprite(tc_plane_width, tc_plane_height);
  TCPlaneSpr.fillSprite(TFT_BLACK);
  TCPlaneSpr.pushImage(0, 0, tc_plane_width, tc_plane_height, tc_plane);
  TCPlaneSpr.setSwapBytes(true);
  TCPlaneSpr.setPivot(tc_plane_width / 2, 36);
  TCPlaneSpr.pushRotated(&TCmainSpr, turnAngle, TFT_BLACK);
  TCPlaneSpr.deleteSprite();

  TCmainSpr.pushSprite(80, 0, TFT_BLACK);
  TCmainSpr.deleteSprite();

  // Check if messages are coming in
  endTime = millis();
  fps = 1000 / (endTime - startTime);
  if (prevInstrumentBrightnessRatio != instrumentBrightnessRatio) // there is a change in brighness, execute code
  {
    analogWrite(TFT_BL, instrumentBrightness);
    prevInstrumentBrightnessRatio = instrumentBrightnessRatio;
  }

  // Serial.println(fps);
}

void checkI2CMesage()
{
  if (i2c_slave.message_available())
  {
    messageID = i2c_slave.getMessage(message);
    switch (messageID)
    {
    case -1:
      // received messageID is 0
      // data is a string in message[] and 0x00 terminated
      // do something with your received data
      // Serial.print("MessageID is -1 and Payload is: "); Serial.println(message);
      // tbd

    case 2:
      // received messageID is 0
      // data is a string in message[] and 0x00 terminated
      // do something with your received data
      // Serial.print("MessageID is 0 and Payload is: "); Serial.println(message);
      setTurnAngle(atof(message));
      break;

    case 3:
      // received messageID is 1
      // data is a string in message[] and 0x00 terminated
      // do something with your received data
      // Serial.print("MessageID is 1 and Payload is: "); Serial.println(message);
      // message[4] = '\0'; // need to truncate the message because it is causing instability with the atof function
      setSlipDegree(atof(message));
      break;

    case 4:
      // received messageID is 1
      // data is a string in message[] and 0x00 terminated
      // do something with your received data
      // Serial.print("MessageID is 1 and Payload is: "); Serial.println(message);
      setInstrumentBrightnessRatio(atof(message));
      break;

      // case 2:
      //   // received messageID is 2
      //   // data is a string in message[] and 0x00 terminated
      //   // do something with your received data
      //   Serial.print("MessageID is 2 and Payload is: "); Serial.println(message);
      //   break;

    default:
      break;
    }
  }
}

void setTurnAngle(float angle)
{
  turnAngle = angle;
}

void setInstrumentBrightnessRatio(float ratio)
{
  instrumentBrightnessRatio = ratio;
  instrumentBrightness = round(scaleValue(instrumentBrightnessRatio, 0.15, 1, 0, 255));
}

void setSlipDegree(double value)
{
  slipDegree = value;
}

float scaleValue(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
