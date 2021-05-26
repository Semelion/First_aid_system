#include <Adafruit_GFX.h>        //OLED библиотека
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "MAX30105.h"           //MAX3010x библиотека
#include "heartRate.h"          //Алгоритм расчета сердечного ритма

MAX30105 particleSensor;

const byte RATE_SIZE = 2; //Увеличьте это для большего усреднения. 4 это хорошо.
byte rates[RATE_SIZE]; //Массив ЧСС
byte rateSpot = 0;
long lastBeat = 0; //Время, когда произошел последний удар
float beatsPerMinute;
int beatAvg;

#define SCREEN_WIDTH 128 // Ширина OLED-дисплея, в пикселях
#define SCREEN_HEIGHT 32 // Высота OLED-дисплея в пикселях
#define OLED_RESET    -1 // Сбросить pin # (или -1, если используется общий сброс Arduino)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Объявление отображаемого имени (дисплей)

static const unsigned char PROGMEM logo2_bmp[] =
{ 0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E, 0x02, 0x10, 0x0C, 0x03, 0x10,              //Logo2 и Logo3 - две картинки bmp, которые отображаются на OLED при вызове
0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40, 0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,
0x02, 0x08, 0xB8, 0x04, 0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,
0x00, 0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00,  };

static const unsigned char PROGMEM logo3_bmp[] =
{ 0x01, 0xF0, 0x0F, 0x80, 0x06, 0x1C, 0x38, 0x60, 0x18, 0x06, 0x60, 0x18, 0x10, 0x01, 0x80, 0x08,
0x20, 0x01, 0x80, 0x04, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x08, 0x03,
0x80, 0x00, 0x08, 0x01, 0x80, 0x00, 0x18, 0x01, 0x80, 0x00, 0x1C, 0x01, 0x80, 0x00, 0x14, 0x00,
0x80, 0x00, 0x14, 0x00, 0x80, 0x00, 0x14, 0x00, 0x40, 0x10, 0x12, 0x00, 0x40, 0x10, 0x12, 0x00,
0x7E, 0x1F, 0x23, 0xFE, 0x03, 0x31, 0xA0, 0x04, 0x01, 0xA0, 0xA0, 0x0C, 0x00, 0xA0, 0xA0, 0x08,
0x00, 0x60, 0xE0, 0x10, 0x00, 0x20, 0x60, 0x20, 0x06, 0x00, 0x40, 0x60, 0x03, 0x00, 0x40, 0xC0,
0x01, 0x80, 0x01, 0x80, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0C, 0x00,
0x00, 0x08, 0x10, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00  };

void setup() {  
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Запустите дисплей OLED
  display.display();
  delay(1000);
  Serial.println("ss");
  // Инициализировать датчик
  particleSensor.begin(Wire, I2C_SPEED_FAST); //Использовать порт I2C по умолчанию, скорость 400 кГц
  particleSensor.setup(); //Настройте датчик с настройками по умолчанию
  particleSensor.setPulseAmplitudeRed(0x0A); //Установите красный светодиод на низкий уровень, чтобы указать, что датчик работает
}

void loop() {
 long irValue = particleSensor.getIR();    //Считывая значение ИК, мы узнаем, есть ли палец на датчике или нет
                                           //Также обнаружение сердцебиения
if(irValue > 7000){                                           //Если обнаружен палец
  Serial.println("ddd");
    display.clearDisplay();                                   //Очистить дисплей
    display.drawBitmap(5, 5, logo2_bmp, 24, 21, WHITE);       //Нарисуйте первое изображение BMP (маленькое сердце)
    display.setTextSize(2);                                   //Рядом с ним отображается средний BPM, вы можете отобразить BPM, если хотите
    display.setTextColor(WHITE); 
    display.setCursor(50,0);                
    display.println("BPM");             
    display.setCursor(50,18);                
    display.println(beatAvg); 
    Serial.println(beatAvg);
    display.display();
    
  if (checkForBeat(irValue) == true)                        //Если сердцебиение обнаружено
  {
    display.clearDisplay();                                //Очистить дисплей
    display.drawBitmap(0, 0, logo3_bmp, 32, 32, WHITE);    //Нарисуйте вторую картинку (большее сердце)
    display.setTextSize(2);                                //И все равно отображает средний BPM
    display.setTextColor(WHITE);             
    display.setCursor(50,0);                
    display.println("BPM");             
    display.setCursor(50,18);                
    display.println(beatAvg); 
    Serial.println(beatAvg);
    display.display();
    tone(3,1000);                                        //И запустить зуммер на 100 мс, вы можете уменьшить, если будет лучше
    delay(100);
    noTone(3);                                          //Отключите зуммер, чтобы получить эффект «бип»
    //Мы почувствовали ритм!
    long delta = millis() - lastBeat;                   //Измерьте продолжительность между двумя ударами
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);           //Расчет BPM

    if (beatsPerMinute < 255 && beatsPerMinute > 20)               //Чтобы вычислить среднее значение, мы храним некоторые значения (4), а затем делаем некоторые математические вычисления для вычисления среднего значения.
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Сохранить это в массиве
      rateSpot %= RATE_SIZE; 

      //Принять среднее значение
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

}
  if (irValue < 7000){       //Если палец не обнаружен, он информирует пользователя и устанавливает среднее значение BPM в 0, или оно будет сохранено для следующего измерения.
     beatAvg=0;
     display.clearDisplay();
     display.setTextSize(1);                    
     display.setTextColor(WHITE);             
     display.setCursor(30,5);                
     display.println("Please Place "); 
     display.setCursor(30,15);
     display.println("your finger ");  
     Serial.println("palec");
     display.display();
     noTone(3);
     }

}
