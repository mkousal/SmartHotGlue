#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "SSD1306.h"
#include "PID_v1.h"
#include "ClickEncoder.h"
#include <defines.hpp>
#include <images.hpp>

SSD1306 disp(0x3C, PIN_SDA, PIN_SCL);
ClickEncoder *encoder;

u_long volatile prevMil = 0;
u_long volatile prevMilFPS = 0;
u_long volatile frames, fps = 0;
const long interval = 500;
const long second = 1000;
bool volatile blink, click, printGlue, removeGlue = 0;
double Kp = 2, Ki = 0.2, Kd = 0.5;
double realTemp, setTemp, outVal;
int volatile value = 0;
int volatile menuItem, menuSubItem = 0;
bool volatile activeItem = 0;
int volatile movSpeed = 127;
int volatile motorState = 0;

PID heatPID(&realTemp, &outVal, &setTemp, Kp, Ki, Kd, DIRECT);

void motorLoop(){
    if (motorState == 0){        //brake
        // ledcWrite(1, 255);
        digitalWrite(MOT_1A, 1);
        digitalWrite(MOT_1B, 1);
        // ledcWrite(2, 255);
        digitalWrite(MOT_2A, 1);
        digitalWrite(MOT_2B, 1);
    }

    if (motorState == 1){        //go forward
        // ledcWrite(1, movSpeed);
        digitalWrite(MOT_1A, 1);
        digitalWrite(MOT_1B, 0);
        // ledcWrite(2, movSpeed);
        digitalWrite(MOT_2A, 1);
        digitalWrite(MOT_2B, 0);
    }

    if (motorState == -1){       //go backward
        // ledcWrite(1, movSpeed);
        digitalWrite(MOT_1A, 0);
        digitalWrite(MOT_1B, 1);
        // ledcWrite(2, movSpeed);
        digitalWrite(MOT_2A, 0);
        digitalWrite(MOT_2B, 1);
    }
}

void menuLoop(){
    encoder->service();
    value += encoder->getValue();

    if (menuItem == 1 && activeItem == 1 && menuSubItem == 1 && click == 1)
        digitalWrite(SHUTDOWN, 0);

    if (click == 1){
    activeItem = !activeItem;
    while (digitalRead(ENC_SW) == 0){
        delay(1);
        }
    }


    if (value == 1 && activeItem == 0){
        ++menuItem;
        if (menuItem == 3)
            menuItem = 0;
    }
    
    if (value == -1 && activeItem ==0){
        --menuItem;
        if (menuItem == -1)
            menuItem = 2;
    }

    if (activeItem == 1){
        switch (menuItem){
            case 0:                             //set temperature control
                if (value == 1)
                    setTemp = setTemp + 5;
                if (value == -1)
                    setTemp = setTemp - 5;
                if (setTemp == -5)
                    setTemp = 0;
                if (setTemp == 305)
                    setTemp = 300;
                break;

            case 1:                             //shutdown
                if (value == 1){
                    ++menuSubItem;
                    if (menuSubItem == 2)
                        menuSubItem = 0;
                }
                if (value == -1){
                    --menuSubItem;
                    if (menuSubItem == -1)
                        menuSubItem = 1;
                }
                break;

            case 2:                             //remove glue
                if (value == 1){
                    ++menuSubItem;
                    if (menuSubItem == 2)
                        menuSubItem = 0;
                }
                if (value == -1){
                    --menuSubItem;
                    if (menuSubItem == -1)
                        menuSubItem = 1;
                }
                break;
        }
    }
    value = 0;
}

int readVal (int CHANNEL, int CS){
    int msb, lsb = 0;
    int commandBytes = B10100000;
    if (CHANNEL == 1)
        commandBytes = B11100000;
    digitalWrite(CS, LOW);
    SPI.transfer(B00000001);
    msb = SPI.transfer(commandBytes);
    msb = msb & B00001111;
    lsb = SPI.transfer(0x00);
    digitalWrite(CS, HIGH);
    return ((int) msb) << 8 | lsb;
}

void calcBlink(){
    u_long curMil = millis();
    if (curMil - prevMil >= interval){
        prevMil = curMil;
        blink = !blink;
    }
}

void calcFPS(){
    u_long curMilFPS = millis();
    ++frames;
    if (curMilFPS - prevMilFPS >= second){
        prevMilFPS = curMilFPS;
        fps = frames;
        frames = 0;
    }
}

int calcADC (int CHANNEL, int CS){
    int sum = 0;
    for (uint8_t i = 0; i != 100; ++i){
        sum += readVal(0, CS_MCP3202); 
    }
    return sum/100;
}

double measTemp() {
  double temp_adc = calcADC(ADC_CHANNEL_HEAT, CS_MCP3202);
  double temperature = (defined_a+sqrt((defined_b)-(defined_c*(R0-(temp_adc*Uref)/defined_e))))/defined_d;
  temperature = round(temperature);
return temperature;    
}

void showData(){
    calcBlink();
    calcFPS();
    disp.clear();
    int rTemp = realTemp;
    int sTemp = setTemp;

    disp.setFont(ArialMT_Plain_24);

    if (blink == 1 && (setTemp - realTemp) > 0){
        disp.drawXbm(112, 8, 16, 16, heat_inv);
    }
    if ((blink == 0 && (setTemp - realTemp) > 0) || realTemp > 50){   
        disp.drawXbm(112, 8, 16, 16, heat);
    }

    if (menuItem == 0 && activeItem == 1 && blink == 0){
    }
    else{
        disp.setTextAlignment(TEXT_ALIGN_RIGHT);
        disp.drawString(68, 22, String(sTemp));
        disp.setTextAlignment(TEXT_ALIGN_LEFT);
        disp.drawString(68, 22, "°C");
    }

    if (menuItem == 0 && activeItem == 0)
        disp.drawXbm(0, 24, 24, 24, temp_set_inv);
    else
        disp.drawXbm(0, 24, 24, 24, temp_set);

    if (menuItem == 1 && activeItem == 0)
        disp.drawXbm(4, 49, 16, 16, off_inv);
    else
        disp.drawXbm(4, 49, 16, 16, off);

    if (menuItem == 2 && blink == 0){
    }
    else {
        disp.fillCircle(44, 58, 3);
    }

    disp.setTextAlignment(TEXT_ALIGN_RIGHT);
    disp.setFont(ArialMT_Plain_10);
    disp.drawString(127, 52, "Vysunout lepidlo");
 
    disp.drawXbm(0, 0, 24, 24, temp_24);

    disp.setFont(ArialMT_Plain_24);
    disp.setTextAlignment(TEXT_ALIGN_RIGHT);
    disp.drawString(68, 0, String(rTemp));
    disp.setTextAlignment(TEXT_ALIGN_LEFT);
    disp.drawString(68, 0, "°C");

    disp.setFont(ArialMT_Plain_10);
    disp.setTextAlignment(TEXT_ALIGN_RIGHT);
    disp.drawString(128, 30, String(fps));
    disp.drawString(128, 40, String(outVal));

    if (menuItem == 1 && activeItem == 1){
        disp.clear();
        disp.setFont(ArialMT_Plain_24);
        disp.setTextAlignment(TEXT_ALIGN_CENTER);
        disp.drawString(64, 0, "Vypnout?");
        if (menuSubItem == 0)
            disp.drawXbm(22, 32, 32, 24, no_inv);
        else
            disp.drawXbm(22, 32, 32, 24, no);
        if (menuSubItem == 1)
            disp.drawXbm(75, 32, 32, 24, yes_inv);
        else
            disp.drawXbm(75, 32, 32, 24, yes);
    }

    if (menuItem == 2 && activeItem == 1){
        disp.clear();
        disp.setFont(ArialMT_Plain_24);
        disp.setTextAlignment(TEXT_ALIGN_CENTER);
        disp.drawString(64, 0, "Vysunout?");
        if (menuSubItem == 0)
            disp.drawXbm(22, 32, 32, 24, no_inv);
        else
            disp.drawXbm(22, 32, 32, 24, no);
        if (menuSubItem == 1)
            disp.drawXbm(75, 32, 32, 24, yes_inv);
        else
            disp.drawXbm(75, 32, 32, 24, yes);
    }

    disp.display();
}

void readButtons(){
    click = !digitalRead(ENC_SW);
    printGlue = !digitalRead(MOV_SW);

    if (printGlue == 0 && removeGlue == 0)
        motorState = 0;
    if (printGlue == 1)
        motorState = 1;
    if (removeGlue == 1)
        motorState = -1;
}

void initMCU(){
    pinMode(CS_MCP3202, OUTPUT);
    pinMode(ENC_A, INPUT);
    pinMode(ENC_B, INPUT);
    pinMode(MOV_SW, INPUT);
    pinMode(ENC_SW, INPUT);
    pinMode(SHUTDOWN, OUTPUT);
    digitalWrite(SHUTDOWN, 1);

    pinMode(MOT_1A, OUTPUT);
    pinMode(MOT_1B, OUTPUT);
    pinMode(MOT_2A, OUTPUT);
    pinMode(MOT_2B, OUTPUT);

    ledcSetup(0, 20000, 8);
    ledcAttachPin(HEAT_PWM, 0);
    // ledcSetup(1, 20000, 8);
    // ledcAttachPin(MOT_1A, 1);
    // ledcSetup(2, 20000, 8);
    // ledcAttachPin(MOT_2A, 2);
}

void setup() {
    initMCU();
    digitalWrite(CS_MCP3202, HIGH);
    SPI.begin();
    Serial.begin(115200);
    disp.init();
    disp.flipScreenVertically();
    disp.clear();
    heatPID.SetMode(AUTOMATIC);
    encoder = new ClickEncoder(ENC_A, ENC_B, ENC_SW, 4);

}

void loop() {
    encoder->service();
    readButtons();
    realTemp = measTemp();

    heatPID.Compute();
    menuLoop();
    showData();

    ledcWrite(0, outVal);
    motorLoop();
}