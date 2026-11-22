#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2); 

const int T_PIN1 = 2; 
const int T_PIN2 = 4; 
const int T_PIN3 = 8; 
const int T_PIN4 = 7; 

const int buzzer = A3; 
const int redLed = 5;
const int greenLed = 6;
const int blueLed = 3;

String inputPassword = "";
const String correctPassword = "1234"; 
const String dogruKart = "23 FE DE 16";
const String dogruCip  = "D3 AC 40 16";

int hataliGirisSayaci = 0;
bool kilitAcikMi = false; 
bool sistemBlokeMi = false; 

void setup() {
  Serial.begin(9600); 
  SPI.begin();           
  rfid.PCD_Init();       
  
  lcd.init();
  lcd.backlight();
  ekraniTemizleVeHazirEt(); 

  pinMode(T_PIN1, INPUT);
  pinMode(T_PIN2, INPUT);
  pinMode(T_PIN3, INPUT);
  pinMode(T_PIN4, INPUT);
  
  pinMode(buzzer, OUTPUT); 
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);

  digitalWrite(redLed, HIGH); 
  digitalWrite(greenLed, LOW);
  digitalWrite(blueLed, LOW);
}

void loop() {
  // 1. TELEFONDAN VEYA PROCESSING'DEN GELEN VERİLERİ DİNLE
  if (Serial.available() > 0) {
    String gelenMetin = Serial.readStringUntil('\n');
    gelenMetin.trim();
    
    if (gelenMetin.length() > 0) {
      char ilkKarakter = gelenMetin.charAt(0);
      
      // ARAYÜZDEN BASSBYPASS VEYA MOBİL TETİKLEME
      if (gelenMetin == "1") { 
        if (sistemBlokeMi) {
          sistemBlokeMi = false; 
          hataliGirisSayaci = 0;
        }
        onayVerSifre(); 
      } 
      else if (gelenMetin == "0") { 
        if (kilitAcikMi) {
          sistemiGeriKilitle();
        }
      }
      // PROCESSING KLAVYEDEN GELEN ŞİFRE KONTROLÜ (Örn: P1234)
      else if (ilkKarakter == 'P' && !sistemBlokeMi) {
        String p_sifre = gelenMetin.substring(1); // Baştaki P harfini atıp şifreyi alıyoruz kanka
        if (p_sifre == correctPassword) {
          onayVerSifre(); // Doğru: Yeşil yanar, ses çıkar, geri kilitlenir
        } else {
          hataVerBasic(); // Yanlış: Devrede LCD değişir, buzzer öter, 3 kere de kilitlenir!
        }
      }
    }
  }

  if (sistemBlokeMi) {
    return; 
  }

  // 2. DOKUNMATİK SENSÖR KANALLARINI DİNLE
  if (!kilitAcikMi) {
    if (digitalRead(T_PIN1) == HIGH) { handleKey("1"); while(digitalRead(T_PIN1) == HIGH); }
    if (digitalRead(T_PIN2) == HIGH) { handleKey("2"); while(digitalRead(T_PIN2) == HIGH); }
    if (digitalRead(T_PIN3) == HIGH) { handleKey("3"); while(digitalRead(T_PIN3) == HIGH); }
    if (digitalRead(T_PIN4) == HIGH) { handleKey("4"); while(digitalRead(T_PIN4) == HIGH); }
  }

  // 3. RFID MODÜLÜNÜ DİNLE
  if (!kilitAcikMi) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String okunanID = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        okunanID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        okunanID += String(rfid.uid.uidByte[i], HEX);
        if (i < rfid.uid.size - 1) okunanID += " ";
      }
      okunanID.toUpperCase();

      if (okunanID == dogruKart || okunanID == dogruCip) {
        onayVerKart(); 
      } else {
        hataVerBasic(); 
      }
      rfid.PICC_HaltA();
    }
  }
}

void handleKey(String key) {
  inputPassword += key;
  tone(buzzer, 350, 12); 
  digitalWrite(blueLed, HIGH); delay(70); digitalWrite(blueLed, LOW);

  lcd.setCursor(0, 1);
  String yildizlar = "Password: ";
  for(int i=0; i < inputPassword.length(); i++) yildizlar += "*";
  lcd.print(yildizlar);

  if (inputPassword.length() == 4) {
    if (inputPassword == correctPassword) {
      onayVerSifre(); 
    } else {
      hataVerBasic(); 
    }
    inputPassword = ""; 
  }
}

void onayVerSifre() {
  hataliGirisSayaci = 0; 
  kilitAcikMi = true;
  Serial.println("1"); // Processing ve Telefon bu sinyalle Başarılı ekranına geçer
  
  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, HIGH); 
  digitalWrite(blueLed, LOW);
  
  lcd.clear();
  lcd.setCursor(1, 0); lcd.print("Access Granted");
  lcd.setCursor(2, 1); lcd.print("Keypad Unlock");
  
  tone(buzzer, 523, 100); delay(120); 
  tone(buzzer, 784, 100); delay(120); 
  tone(buzzer, 1046, 120); delay(140); 
  tone(buzzer, 1318, 250); delay(3000); 
  
  sistemiGeriKilitle(); 
}

void onayVerKart() {
  hataliGirisSayaci = 0; 
  kilitAcikMi = true;
  Serial.println("1"); 
  
  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, LOW);
  digitalWrite(blueLed, HIGH); 
  
  lcd.clear();
  lcd.setCursor(1, 0); lcd.print("Access Granted");
  lcd.setCursor(3, 1); lcd.print("RFID Unlock");
  
  tone(buzzer, 523, 100); delay(120); 
  tone(buzzer, 1046, 120); delay(140); 
  tone(buzzer, 1318, 250); delay(3000); 
  
  sistemiGeriKilitle(); 
}

void sistemiGeriKilitle() {
  kilitAcikMi = false;
  Serial.println("0"); // Sistem kilitlendi durumunu fırlatır
  
  digitalWrite(greenLed, LOW);
  digitalWrite(blueLed, LOW);
  digitalWrite(redLed, HIGH); 
  
  ekraniTemizleVeHazirEt();
}

void hataVerBasic() {
  hataliGirisSayaci++; 
  Serial.println("2"); // Processing ve Telefona başarısız sinyali gider
  
  if (hataliGirisSayaci >= 3) {
    sistemBlokeMi = true; 
    Serial.println("ALARM"); // Siber kilitlenme sinyali fırlatılır!
    
    lcd.clear();
    lcd.setCursor(1, 0); lcd.print("System Blocked");
    lcd.setCursor(2, 1); lcd.print("!! NO ENTRY !!");

    for (int i = 0; i < 8; i++) {
      digitalWrite(redLed, HIGH); digitalWrite(blueLed, LOW);
      tone(buzzer, 1400, 250); delay(250);
      digitalWrite(redLed, LOW); digitalWrite(blueLed, HIGH);
      tone(buzzer, 950, 250); delay(250);
    }
    digitalWrite(blueLed, LOW);
    digitalWrite(redLed, HIGH); 
    
    lcd.clear();
    lcd.setCursor(1, 0); lcd.print("System Blocked");
    lcd.setCursor(0, 1); lcd.print("Use App to Reset");
    
  } else {
    lcd.clear();
    lcd.setCursor(1, 0); lcd.print("Access Denied");
    lcd.setCursor(3, 1); lcd.print("Try Again");

    tone(buzzer, 220, 120); delay(160);
    tone(buzzer, 220, 120); delay(160);
    tone(buzzer, 220, 250); delay(270);
    
    delay(400); 
    ekraniTemizleVeHazirEt();
  }
}

void ekraniTemizleVeHazirEt() {
  lcd.clear();
  lcd.setCursor(2, 0); lcd.print("System Ready");
  lcd.setCursor(0, 1); lcd.print("Scan ID or Pass");
}