#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FS.h>
#include <iostream>
//#include <SPIFFS.h> //S2

// ----- Дисплей  -----
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);
#define SDA_PIN 4
#define SCL_PIN 5
//#define SDA_PIN 33 //S2
//#define SCL_PIN 35 //S2

// ----- Энкодер -----
#define delta_CW 2 //5
#define EN_SW_PIN 2
#define EN_CW_PIN 13
#define EN_CCW_PIN 12

#define R1_PIN 0
#define R2_PIN 16
#define R3_PIN 14
#define R4_PIN 15

#define MIN 55 //1000
#define MAX 300 //2800

// ----- Меню -----
String menuItems[] = {"Boot Time", "Up 2",  "Up 1", "Down 1",  "Down 2", "Info", "Exit"};
int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);
int counter, menu_select = 0;
int menu_start = 0, menu_end = 3;
bool menu = false;
uint16_t Param1 = 130;
uint16_t Param2 = 120;
uint16_t Param3 = 110;
uint16_t Param4 = 100;

int submenu = 0;
const char* confFile = "def_t.txt";                         // config file

bool Timer_Blink = false;
uint16_t Timer1 = 0;
uint16_t Timer1_nom = 1;

IRAM_ATTR void isrA() {
    (digitalRead(EN_CCW_PIN) == 1) ? counter--: counter++;
}

IRAM_ATTR void isrB() {
    (digitalRead(EN_CW_PIN) == 1) ? counter++: counter--;
}

void ICACHE_RAM_ATTR onTimerISR(){
    Timer_Blink = !Timer_Blink;
    timer1_write(5000000);//10us
    if (Timer1 < Timer1_nom * 60)
    {
        Timer1++;
    }
}

// The setup() function runs once each time the micro-controller starts
void setup() {
        
    timer1_attachInterrupt(onTimerISR);
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    timer1_write(5000000); //100000 us

    //Serial1.begin(9600, SERIAL_8N1, 37, 39); //S2
    Serial.begin(9600);
    //pinMode(1, FUNCTION_3);    // Превращаем пин TX в GPIO
    //pinMode(3, FUNCTION_3);    // Превращаем пин RX в GPIO

    pinMode(EN_SW_PIN, INPUT_PULLUP);
    pinMode(EN_CW_PIN, INPUT_PULLUP);
    pinMode(EN_CCW_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EN_CW_PIN), isrA, RISING);
    attachInterrupt(digitalPinToInterrupt(EN_CCW_PIN), isrB, RISING);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    Wire.begin(SDA_PIN, SCL_PIN);
    display.setCursor(0, 0);
    display.print("Display-OK");
    display.display();
  
    pinMode(R1_PIN, OUTPUT);
    digitalWrite(R1_PIN, LOW);
    pinMode(R2_PIN, OUTPUT);
    digitalWrite(R2_PIN, LOW);
    pinMode(R3_PIN, OUTPUT);
    digitalWrite(R3_PIN, LOW);
    pinMode(R4_PIN, OUTPUT);
    digitalWrite(R4_PIN, LOW);

    if (!SPIFFS.begin()) {                                  // init SPIFFS
        display.setCursor(0, 8);
        display.print("FS-Err");
        display.display();
        delay(500);
        //return;
    } else {
        display.setCursor(0, 8);
        display.print("FS-OK");
        display.display();
        delay(500);
    }

    if (SPIFFS.exists(confFile)) {                          // read config
        display.setCursor(0, 16);
        display.print("File-OK");
        display.display();
        delay(500);

        File f = SPIFFS.open(confFile, "r");
        Param1 = (f.read() << 8) + f.read();
        Param2 = (f.read() << 8) + f.read();
        Param3 = (f.read() << 8) + f.read();
        Param4 = (f.read() << 8) + f.read();
        Timer1_nom = (f.read() << 8) + f.read();
        f.close();
        Param1 < MIN ? Param1 = MIN : Param1 > MAX ? Param1 = MAX : Param1 = Param1;
        Param2 < MIN ? Param2 = MIN : Param2 > MAX ? Param2 = MAX : Param2 = Param2;
        Param3 < MIN ? Param3 = MIN : Param3 > MAX ? Param3 = MAX : Param3 = Param3;
        Param4 < MIN ? Param4 = MIN : Param4 > MAX ? Param4 = MAX : Param4 = Param4;
        Timer1_nom < 0 ? Timer1_nom = 0 : Timer1_nom > 15 ? Timer1_nom = 15 : Timer1_nom = Timer1_nom;
    }
    else
    {
        display.setCursor(0, 16);
        display.print("File-Err");
        display.display();
        delay(500);
        ConfigFileUpdate();                                 // create default config, if doesn't exist
    }
}

uint16_t port, portF = 0;
uint16_t buffer [5] = {0, 0, 0, 0, 0};
byte start_index = 0;

void loop()
{

    if (Serial.available()) {
        portF = uint(Serial.parseFloat() * 10);
        if (portF != 0)
        {
            if (start_index <= 5) {start_index++;}
            buffer[0] = buffer[1];
            buffer[1] = buffer[2];
            buffer[2] = buffer[3];
            buffer[3] = buffer[4];
            buffer[4] = portF;
            if (start_index >= 5) {
                port = (buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4]) / 5;
//                Serial.println(port);
            }
        }
    }

    Encoder();

    Display();

    if (Timer1 >= Timer1_nom * 60) {
        if ((port >= 55) and (port <= 655)) {
            if (Param1 <= port) {digitalWrite(R1_PIN, HIGH);} else {digitalWrite(R1_PIN, LOW);}
            if (Param2 <= port) {digitalWrite(R2_PIN, HIGH);} else {digitalWrite(R2_PIN, LOW);}
            if (Param3 >= port) {digitalWrite(R3_PIN, HIGH);} else {digitalWrite(R3_PIN, LOW);}
            if (Param4 >= port) {digitalWrite(R4_PIN, HIGH);} else {digitalWrite(R4_PIN, LOW);}
        } else {
            digitalWrite(R1_PIN, LOW);
            digitalWrite(R2_PIN, LOW);
            digitalWrite(R3_PIN, LOW);
            digitalWrite(R4_PIN, LOW);
        }
    } else {
        digitalWrite(R1_PIN, LOW);
        digitalWrite(R2_PIN, LOW);
        digitalWrite(R3_PIN, LOW);
        digitalWrite(R4_PIN, LOW);
    }

    if (!digitalRead(EN_SW_PIN)) {
        Encoder_button();
    }
}

void Encoder () {
    if (menu)
    {
        if (counter - menu_select > delta_CW) {
            if (menu_select < menuSize - 1){
                menu_select++;
                if (menu_select > menu_end)
                {
                    menu_start = menu_select - 3;
                    menu_end = menu_select;
                }
            }
            counter = menu_select;
        }
        if (counter - menu_select < -delta_CW) {
            if (menu_select > 0){
                menu_select--;
                if (menu_select < menu_start)
                {
                    menu_start = menu_select;
                    menu_end = menu_select + 3;
                }
            }
            counter = menu_select;
        }
    }
    
    if (submenu == 1)
    {
        int temp = menu_select;
        if (counter - menu_select > delta_CW) {
                if (Param1 < MAX) Param1++;
                menu_select++;
            }
        if (counter - menu_select < -delta_CW) {
                if (Param1 > MIN) Param1--;
                menu_select--;
            }
        menu_select = temp;
        counter = menu_select;
    }
    if (submenu == 2)
    {
        int temp = menu_select;
        if (counter - menu_select > delta_CW) {
                if (Param2 < MAX) Param2++;
                menu_select++;
            }
        if (counter - menu_select < -delta_CW) {
                if (Param2 > MIN) Param2--;
                menu_select--;
            }
        menu_select = temp;
        counter = menu_select;
    }
    if (submenu == 3)
    {
        int temp = menu_select;
        if (counter - menu_select > delta_CW) {
                if (Param3 < MAX) Param3++;
                menu_select++;
            }
        if (counter - menu_select < -delta_CW) {
                if (Param3 > MIN) Param3--;
                menu_select--;
            }
        menu_select = temp;
        counter = menu_select;
    }
    if (submenu == 4)
    {
        int temp = menu_select;
        if (counter - menu_select > delta_CW) {
                if (Param4 < MAX) Param4++;
                menu_select++;
            }
        if (counter - menu_select < -delta_CW) {
                if (Param4 > MIN) Param4--;
                menu_select--;
            }
        menu_select = temp;
        counter = menu_select;
    }
    if (submenu == 6)
    {
        int temp = menu_select;
        if (counter - menu_select > delta_CW) {
                if (Timer1_nom < 15) Timer1_nom++;
                menu_select++;
            }
        if (counter - menu_select < -delta_CW) {
                if (Timer1_nom > 0) Timer1_nom--;
                menu_select--;
            }
        menu_select = temp;
        counter = menu_select;
    }
}

void Display() {
    display.clearDisplay();
    if (menu)
    {
        for (int i = menu_start; i <= menu_end; i++) {
            if (i == menu_select) {
                display.fillRect(0, (i - menu_start) * 12, 64, 12, WHITE); // выделение
                display.setTextColor(BLACK);
            } else {
                display.setTextColor(WHITE);
            }
            display.setCursor(5, (i - menu_start) * 12 + 2);
            display.print(menuItems[i]);
        }
    } else {
        switch (submenu) {
            case 0:
                if (Param1 <=  port) {
                    display.fillRect(0, 0, 64, 8, WHITE);
                    display.setTextColor(BLACK);
                } else {
                    display.setTextColor(WHITE);
                }
                display.setCursor(0, 0);
                display.printf("Up2:  %.2f", double(Param1)/100);

                if (Param2 <=  port) {
                    display.fillRect(0, 9, 64, 8, WHITE);
                    display.setTextColor(BLACK);
                } else {
                    display.setTextColor(WHITE);
                }
                display.setCursor(0, 9);
                display.printf("Up1:  %.2f", double(Param2)/100);

                if (Param3 >=  port) {
                    display.fillRect(0, 18, 64, 8, WHITE);
                    display.setTextColor(BLACK);
                } else {
                    display.setTextColor(WHITE);
                }
                display.setCursor(0, 18);
                display.printf("Do1:  %.2f", double(Param3)/100);

                if (Param4 >=  port) {
                    display.fillRect(0, 27, 64, 8, WHITE);
                    display.setTextColor(BLACK);
                } else {
                    display.setTextColor(WHITE);
                }
                display.setCursor(0, 27);
                display.printf("Do2:  %.2f", double(Param4)/100);

                display.setTextColor(WHITE);
                display.setCursor(0, 38);
                display.printf("Lam:  %.2f", double(port)/100);
                break;
            case 1:
                display.setTextColor(BLACK);
                display.fillRect(0, 0, 64, 12, WHITE);
                display.setCursor(5, 2);
                display.print(menuItems[submenu]);
                display.setTextColor(WHITE);
                display.setCursor(5, 12 + 2);
                display.printf("val: %.2f", double(Param1)/100);
                break;
            case 2:
                display.setTextColor(BLACK);
                display.fillRect(0, 0, 64, 12, WHITE);
                display.setCursor(5, 2);
                display.print(menuItems[submenu]);
                display.setTextColor(WHITE);
                display.setCursor(5, 12 + 2);
                display.printf("val: %.2f", double(Param2)/100);
                break;
            case 3:
                display.setTextColor(BLACK);
                display.fillRect(0, 0, 64, 12, WHITE);
                display.setCursor(5, 2);
                display.print(menuItems[submenu]);
                display.setTextColor(WHITE);
                display.setCursor(5, 12 + 2);
                display.printf("val: %.2f", double(Param3)/100);
                break;
            case 4:
                display.setTextColor(BLACK);
                display.fillRect(0, 0, 64, 12, WHITE);
                display.setCursor(5, 2);
                display.print(menuItems[submenu]);
                display.setTextColor(WHITE);
                display.setCursor(5, 12 + 2);
                display.printf("val: %.2f", double(Param4)/100);
                break;
            case 5:
                display.setTextColor(BLACK);
                display.fillRect(0, 0, 64, 12, WHITE);
                display.setCursor(5, 2);
                display.print(menuItems[submenu]);
                display.setTextColor(WHITE);
                display.setCursor(5, 12 + 2);
                display.print("v: 1.0.7");
                display.setCursor(5, 12 * 2 + 2);
                display.print("08/01/26");
                break;                
            case 6:
                display.setTextColor(BLACK);
                display.fillRect(0, 0, 64, 12, WHITE);
                display.setCursor(5, 2);
                display.print("Boot Time");
                display.setTextColor(WHITE);
                display.setCursor(5, 12 + 2);
                display.printf("min: %d", Timer1_nom);
                break;                

        }
    }
    if (Timer1 < Timer1_nom * 60) {
        if (Timer_Blink)
        {
            display.drawLine(0, 47, int(float (64) / (Timer1_nom * 60) * Timer1), 47, WHITE);
        } else
        {
            display.drawLine(0, 46, int(float (64) / (Timer1_nom * 60) * Timer1), 46, WHITE);
        }
    } else
    {
        if (Timer_Blink)
        {
            display.drawLine(0, 47, 64, 47, WHITE);
        } else
        {
            display.drawLine(0, 46, 64, 46, WHITE);
            display.drawLine(0, 47, 64, 47, WHITE);
        }
    }
    display.display();   
}

void Encoder_button() {
    if (!menu and submenu==0) {
        menu = true;
        counter = 0;
        menu_select = 0;
        counter = 0;
        menu_start = 0;
        menu_end = 3;
    }
    else {
        switch (submenu) {
            case 0:
                switch (menu_select) {
                    case 0:
                        menu = false;
                        submenu = 6;
                        break;
                    case 1:
                        menu = false;
                        submenu = 1;
                        break;
                    case 2:
                        menu = false;
                        submenu = 2;
                        break;                            
                    case 3:
                        submenu = 3;
                        menu = false;
                        break;
                    case 4:
                        submenu = 4;
                        menu = false;
                        break;
                    case 5:
                        submenu = 5;
                        menu = false;
                        break;
                    case 6:
                        submenu = 0;
                        menu = false;
                        break;
                }
                break;
            case 1:
                ConfigFileUpdate();
                submenu = 0;
                menu = true;
                break;
            case 2:
                ConfigFileUpdate();
                submenu = 0;
                menu = true;
                break;
            case 3:
                ConfigFileUpdate();
                submenu = 0;
                menu = true;
                break;
            case 4:
                ConfigFileUpdate();
                submenu = 0;
                menu = true;
                break;
            case 5:
                submenu = 0;
                menu = true;
                break;
            case 6:
                ConfigFileUpdate();
                submenu = 0;
                menu = true;
                break;
        }
    }
    while (!digitalRead(EN_SW_PIN)) ;
}

void ConfigFileUpdate() {
    File f = SPIFFS.open(confFile, "w");
    f.write(Param1 >> 8);
    f.write(Param1 & 0xFF);
    f.write(Param2 >> 8);
    f.write(Param2 & 0xFF);
    f.write(Param3 >> 8);
    f.write(Param3 & 0xFF);
    f.write(Param4 >> 8);
    f.write(Param4 & 0xFF);
    f.write(Timer1_nom >> 8);
    f.write(Timer1_nom & 0xFF);
    f.close();
}
