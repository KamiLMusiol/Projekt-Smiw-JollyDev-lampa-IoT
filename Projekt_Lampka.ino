/*
 * Projekt: Inteligentna Lampka Jolly
 * wersja finalna lekko statyczna
 */

#define BLYNK_TEMPLATE_ID "TMPL4FqwsYOxC" //unikalny numer identyfikacyjny  projektu w chmurze Blynk
#define BLYNK_TEMPLATE_NAME "Inteligentna Lampa" // nazwa projektu z panelu blynk
#define BLYNK_AUTH_TOKEN "e4P3GACkRTwgszLbP-d8-qbTfGs72ytG" // klucz dostepu do plytku BLYKA (tej konkretnej)

#define BLYNK_PRINT Serial // ta komenda powowduje ze mozemy podgladnac co blynk robi

#include <SPI.h> // Biblioteka potrzebna dla modułu Wi-Fi i Blynka // potrzebne do wysylania powiadomien
// Ładujemy WiFi z Jolly - to wczyta wszystkie WL_CONNECTED itp.
#include <WiFi.h> //dla ESP8285 pozwala atmefze z nim "rozmawiac" i zmeinia urzadzenie na iot

/// fragment ktory jest tlumczem miedzy blinkiem a plytka w blynk jest WL_NO_SHIELD - blad wifi nie dziala, jolly dev WL_NO_WIFI_MODULE_COMM - brak komuniakcji z module
#ifndef WL_NO_SHIELD //czy wiemy (jolly) co to jest WL_NO_SHIELD
  #define WL_NO_SHIELD WL_NO_WIFI_MODULE_COMM// jesli nie to jest to samo co WL_NO_WIFI_MODULE_COMM - to juz jolly rozumie - mikro rozumie nowe instrukcje starych juz nie
#endif //koniec zasad


#include <BlynkSimpleWifi.h> //instrukcje zawierajace lacznie plytki z blinkiem przeez wifi
#include <Wire.h> //bibliotea do obslugi i^2c
#include <BH1750.h> //biblo dla czujnika swiatla

/* dane do wifi */
char ssid[] = "Orange_Swiatlowod_1C80"; //nazwa
char pass[] = "2QKN3NEHADCW"; //haslo

// Piny takie jak na shcemacie prezentacji podglad
const int PIR_PIN = 2;
const int MOSFET_PIN = 9;
const int POT_PIN = A0;

// Ustawienia Lampki
// Definicja progów (w luksach)
const int LUX_CIEMNO = 50;    // Poniżej tego: bardzo ciemno -> świecimy na 100% // pobiedzy to 50%
const int LUX_JASNO = 300;    // Powyżej tego: Dzień -> nie świecimy wcale
const unsigned long LIGHT_DURATION = 10000; //czas swiecenia po wykryciu rychu i jezeli nie wykryje ruchu to 10 sekund

BH1750 lightMeter; //reprezencja czujnika
unsigned long lastMotionTime = 0; //int zamaly loong ok uzywany do liczenia czasy do 10 sekund ??
bool isLightOn = false; // swieci 1 nie swieci 0
bool notificationSent = false; // blokada antyspamowa - zdejmowana dopiero gdy czujnik przestaje wykrywac ruch inaczej spam

int storedPWM = 0;//
void setup() {
  Serial.begin(115200); //rozpoczecie komunikacji usb komputer dla jolly
  Wire.begin(); // uruchominei magistrali i^2c dla BH1750

  Serial.println(F("Start Jolly")); // z pamieci flash wypisujemy (jezeli wpielismy to do kompa w celu testow) info ze jolly sie odalil
  
  // inicjacja wifi kod z przykladu!!!
  Serial.println(F("Start wifi"));
  WiFi.reset(); //twardy reset modulu siecowego
  WiFi.init(AP_STA_MODE); //start modulu wifi AP_STA_MODE - Access Point + Station sta to station - czyli laczy sie pobiera to wifi,  Access Point moze te wif rozglaszac nie korzystam

  // Sprawdzamy czy moduł żyje ( definicja z Jolly)
  if (WiFi.status() == WL_NO_WIFI_MODULE_COMM) { // czy wifi nie odpowiada
    Serial.println(F("Wifi nie dziala"));
    while(true); 
  }

  Serial.print(F("Laczenie z: "));
  Serial.println(ssid); //podglad czy dobrze wpisalismy nazwe

 
  WiFi.begin(ssid, pass);  // laczenie reczne

  while (WiFi.status() != WL_CONNECTED) { //dopoki nie polaczy
    delay(500);
    Serial.print(".");
  }
  
  Serial.println(F("\nWiFi Polaczone!"));
  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP()); //czy do dprego

  //  czesc blynkowska 
  // Mówimy Blynkowi:połączono"
  Blynk.config(BLYNK_AUTH_TOKEN);   //laczenie z blinkiem - nie koniecznie w tej milisekundzie jesli sie nie uda przejdzie dalej
  
  // Inicjalizacja czujników
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) { //wlacza czujnik i sprawdza czy sie udalo
    Serial.println(F("Czujnik swiatla OK"));
  } else {
    Serial.println(F("Blad czujnika swiatla"));
  }

  pinMode(PIR_PIN, INPUT); //ustawienie ze pin pira to input
  pinMode(MOSFET_PIN, OUTPUT); //ustawienie ze pin pira to output 
}

void loop() {

  // Obsługa sieci
  Blynk.run(); //odbiera (tu nie) i wysyla do blinksa oraz mowi serwerowi ze projekt zyje

  // Odczyty
  float lux = lightMeter.readLightLevel(); //odczytuje ilosc luxow z bh
  int motion = digitalRead(PIR_PIN); //odczytuje czy byl ruch 1 - jest anpiecei 0 nie ma napiecia
  int potValue = analogRead(POT_PIN); // odczytuje wartosc z potencjometru  analogowo - 0 - 1023 przetwornik 10 bitowy 

  // Logika jasności LED
  int ledBrightness = map(potValue, 0, 1023, 0, 255); // tlumacz mateamtyczny skaluje, wejscie ADC jest 10 bitowe, wyjcie PWM jest 8 bitowe 

  // Logika wykrywania ruchu
  if (motion == HIGH) { //jezeli byl ruch
    lastMotionTime = millis(); // liczy od wlaczenia programu milisekundy zegar systemowy
    
    if (notificationSent == false) { //jezeli wczesniej nie wyslano wysylamy
       Serial.println(F("\nruch wysylanie info do blynka"));
       Blynk.logEvent("movement_alert", "Wykryto ruch!"); //wysyla powiadomienie movement_alert - kod zdarzenia Klucz, "Wykryto ruch!" - opis useless
       notificationSent = true; //upewniamy sie ze wiecej nie wyslemy blynk ma limity na liczbe wyslanych wiadomosci dlatego takie zabezpieczenie
    }
  } else {
    notificationSent = false; //przestalismy wykrywac ruch to false - i tak w blynku jest zabezpieczenie ze tylko co 1 minute mozemy dostac wiadomosc
  }

  // Logika świecenia
 
  
  // Sprawdzamy czy w ogóle powinien świecić 
  bool timeCondition = (millis() - lastMotionTime < LIGHT_DURATION); // czyli czas teraz od odpalenia urzadzenia -  czas od wykrycia ruchu czy jst < od 10 sekund
  
  int finalPWM = 0; // Zmienna na finalną moc LED

  if (timeCondition ) { // czy wykryto ruch w ciągu ostatnich 10 sekund  // to nie i czy lampka wylaczona - zabezpieczenie przed wykyrwaniem z czujnika swiatla z lampki
      

      if (lux < LUX_CIEMNO) { // czy jest bardzo ciemno to 100% z potencjometru
       
          finalPWM = ledBrightness; 
          isLightOn = true;
          
      } else if (lux >= LUX_CIEMNO && lux < LUX_JASNO) { // czy jest troche ciemno ale nie najciemniej  (50 - 300 lux) to 50 % potencjometra
         
          finalPWM = ledBrightness / 2; 
          isLightOn = true;
          
      } else { //jasno bardzo to nie wlaczamy
         
          finalPWM = 0;
          isLightOn = false;
      }
      
  } else { //brak ruchu od 10 sekund zgas
      
      finalPWM = 0;
      isLightOn = false;
  }

  // po zebraniu danych 
  analogWrite(MOSFET_PIN, finalPWM); // uzycie technki PWM zapisuje do zmiennej finalPWM z mosfet Pinu,  MOSFET_PIN -  to jest to 0 - 255
}