#include <Wire.h>                                     // I2C 통신을 쉽게 사용할 수 있도록 돕는 함수
#include <LiquidCrystal.h>                          // LCD를 사용하기 위한 LiquidCrystal 라이브러리
#include "Adafruit_MPR121.h"                          // MPR121 터치센서 라이브러리
Adafruit_MPR121 cap = Adafruit_MPR121();         
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);                // RS:2번 핀,  EN:3번 핀, D4:4번 핀, D5:5번 핀, D6:6번 핀, D7:7번 핀

//Function prototype
void PlaySounds(byte type, byte velocity);            // 다양한 악기의 소리를 출력하는 함수(Serial Communication)
void lcd_print(int play_Mode);                        // 현재 사용되고있는 악기의 Mode를 LCD에 출력하는 함수
void PianoSounds(byte touch_num, byte velocity);      // 피아노의 소리를 출력하는 함수(Serial Communication)
void lcd_print_piano(int play_Mode);                  // 현재 사용되고 있는 피아노의 옥타브를 LCD에 출력하는 함수

//버튼 터치여부를 컨트롤 해주기 위한 변수
uint16_t Last_Touched = 0;
uint16_t Current_Touched = 0;

//음계 번호
int Piano[3][12] = {                                   // 피아노 옥타브만을 연주하기 위한 배열
  { 48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67 } , // 첫번째 옥타브 (도,레,미,파,솔,라,시,도,레,미,파,솔)
  { 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79 } , // 두번째 옥타브 (도,레,미,파,솔,라,시,도,레,미,파,솔)
  { 72, 74, 76, 77, 79, 81, 83, 84, 86, 88, 89, 91 }   // 세번째 옥타브 (도,레,미,파,솔,라,시,도,레,미,파,솔)
};
int Note[4][12] = {                                    // 4가지 sound type을 연주하기 위한 배열
  {55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74 },   // 바이올린 음계
  {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 59, 60},    // 일렉기타 음계
  {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 59, 60},    // 트럼펫 음계
  {49, 51, 44, 41, 43, 47, 48, 45, 50, 38, 40, 35 }    // 드럼 음계
};

//악기번호
int Piano_type = 1;                         // 악기번호 1 : 피아노
int sound_type[4] = {41, 28, 57, 0};        // 악기번호 41:바이올린, 28:일렉기타, 57:트럼펫, 0:드럼(악기의 채널을 바꿔 사용하기 때문에 번호의미 없음)

int type = 0;     // Sound type을 변경하기 위해 배열에서 사용할 변수 ( loop 에서 ++ 해주며 0~3까지 사용할 예정)
int octave = 0;   // Piano octave를 변경하기 위해 배열에서 사용할 변수 ( loop 에서 ++해주며 0~2까지 사용할 예정)

int pin_btn_1 = 8; //버튼 핀번호 설정
int pin_btn_2 = 9; //버튼 핀번호 설정
int active = 0;    // 1번버튼과 2번 버튼을 동시에 사용하지 않기 위해서, 현재 실행중인 버튼을 확인하기 위한 변수

//debouncing에 사용할 변수 선언
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers


void setup() {
  Serial.begin(19200);            // 시리얼 통신 보드레이트 설정 -  ' MIDI<->Serial Bridge'프로그램의 설정에서도 똑같이 19200으로 맞춰줘야 함.
  pinMode(pin_btn_1, INPUT);       // 1번 버튼 (피아노 옥타브변경용 버튼)
  pinMode(pin_btn_2, INPUT);       // 2번 버튼 (악기 변경용 버튼)
  lcd.begin(16, 2);                // LCD
  lcd.print("Team of E N E  ");    // "Team of E N E" - LCD 첫째줄에 출력
  lcd.setCursor(0, 1);             // LCD 커서 변경(2번째줄로)
  lcd.print("Electric Piano");     // "Electric Piano" - LCD 둘째줄에 출력

  if (!cap.begin(0x5A)) {           // MPR121(터치센서) 연결 확인을 위한 조건
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  else
    Serial.println("MPR121 found!");

  Serial.write(0xc0);             // 실행초기 악기 설정 ( 0xc0 을 Serial.Write 해주면 다음에 나오는 숫자로 악기를 변경해 주겠다는 의미입니다. )
  Serial.write(Piano_type);       // 악기번호 전송 : 피아노 번호(1)를 보내줌.
}



void loop() {
  // 1번 버튼 디바운스 코드
  int reading = digitalRead(pin_btn_1);
  if ( reading  != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState)
    {
      buttonState = reading;
      if (buttonState == HIGH)                              // 버튼이 눌러졌다면
      {
        active = 1;                                         // 1번 버튼 활성화 ( active 변수가 1이면 1번버튼 유지)
        type++;                                             // 다음 악기(Sound type)로 변경
        lcd_print(type);                                    // Function call : 변경된 악기 번호 전송 & 출력
        if ( type == 4)                                     // 준비된 악기의 수보다 많은 변수가 된다면.
        {
          type = 0;                                         // 첫번째 악기로 되돌아감
          lcd_print(type);                                  // Function call : 첫번째 악기 번호 전송 & 출력
        }
      }
    }
  }//end if
  lastButtonState = reading;
  //end 1st_button


  // 2번 버튼 디바운스 코드
  int octave_reading = digitalRead(pin_btn_2);
  if ( octave_reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (octave_reading != buttonState) {
      buttonState = octave_reading;
      if (buttonState == HIGH) {                // 버튼이 눌러졌다면
        active = 2 ;                            // 2번 버튼 활성화 ( active 변수가 2이면 2번버튼 유지)
        octave++;                               // 다음 옥타브(Octave up)로 변경
        lcd_print_piano(octave);                // Function call : 변경된 옥타브 번호 전송 & 출력

        if ( octave == 3) {                     // 준비된 옥타브의 수보다 많은 변수가 된다면.
          octave = 0;                           // 첫번째 옥타브로 되돌아감.
          lcd_print_piano(octave);              // Function call : 첫번째 옥타브 번호 전송 & 출력
        }
      }
    }
  }
  lastButtonState =  octave_reading;
  // end 2nd_button



  if ( active == 1 )                                                      // active == 1 : 첫 번째 버튼의 타입 유지 (다양한 악기)
  {
    Current_Touched = cap.touched();                                    // 터치를 읽어와 Current_Touched 변수에 저장
    for (uint8_t i = 0; i < 12; i++) {                                  // 0~11 총 12개의 변수(건반)
      if ((Current_Touched & _BV(i)) && !(Last_Touched & _BV(i)) ) {    // 어떤 건반에서 신호가 들어오는지 읽어드림
        PlaySounds(i, 127);                                             // Function Call : i번째 터치센서 읽어옴. Velocity(치는속도, 즉 볼륨 보내줌)
      }
      if (!(Current_Touched & _BV(i)) && (Last_Touched & _BV(i)) ) {    // 어떤 건반에서 신호가 들어오는지 읽어드림
        PlaySounds(i, 0);                                               // Function Call : i번째 터치센서 읽어옴. Velocity(치는속도, 즉 볼륨 보내줌)
      }
    }
    Last_Touched = Current_Touched;                                     // 터치 상태 저장, ON / OFF를 구분해주기 위함.
  }

  else if ( active == 2 )                      // active == 2 : 두 번째 버튼의 타입 유지 (피아노_3개의 옥타브를 가짐)
  {
    Current_Touched = cap.touched();                                      // 터치를 읽어와 Current_Touched 변수에 저장
    for (uint8_t i = 0; i < 12; i++) {                                    // 0~11 총 12개의 변수(건반)
      if ((Current_Touched & _BV(i)) && !(Last_Touched & _BV(i)) ) {      // 어떤 건반에서 신호가 들어오는지 읽어드림
        PianoSounds(i, 127);                                              // Function Call : i번째 터치센서 읽어옴. Velocity(치는속도, 즉 볼륨 보내줌)
      }
      if (!(Current_Touched & _BV(i)) && (Last_Touched & _BV(i)) ) {      // 어떤 건반에서 신호가 들어오는지 읽어드림
        PianoSounds(i, 0);                                                // Function Call : i번째 터치센서 읽어옴. Velocity(치는속도, 즉 볼륨 보내줌)
      }
    }
    Last_Touched = Current_Touched;                                     // 터치 상태 저장, ON / OFF를 구분해주기 위함.
  }


} // end loop




void PlaySounds(byte touch_num, byte velocity) {         // @touch_num : 터치된 번호,  @velocity : 치는속도(볼륨)
  Serial.write(0xc0);                                   // 0xc0 : 악기 타입을 입력하겠다는 신호.
  Serial.write(sound_type[type]);                       // 악기 타입 입력(sound_type 배열에 악기번호가 저장되어 있음, 버튼을 통해서 type이 ++ 되면 악기 번호가 바뀜.
  if (type == 3) {                                       // 타악기(드럼)의 경우 악기 번호가 아니라 채널을 바꿔줘야 하기에, type==3일경우 sound_type이 의미가 없음. 채널을 바꿔 악기를 변경함.
    Serial.write(0x90 | 9);                             // 9번 채널로 악기 변경
  }
  else {                                                 // 타악기가 아닌 그 외의 경우.
    Serial.write(0x90);                                 // 0x90 : 음계를 입력하겠다는 신호.
  }

  switch (touch_num) {                                  // 터치에 입력된 값(몇번째 센서가 터치가 됐는지)에 따라 Serial 통신으로 음계번호 전달.
    case 0: Serial.write(Note[type][0]); break;
    case 1: Serial.write(Note[type][1]); break;
    case 2: Serial.write(Note[type][2]); break;
    case 3: Serial.write(Note[type][3]); break;
    case 4: Serial.write(Note[type][4]); break;
    case 5: Serial.write(Note[type][5]); break;
    case 6: Serial.write(Note[type][6]); break;
    case 7: Serial.write(Note[type][7]); break;
    case 8: Serial.write(Note[type][8]); break;
    case 9: Serial.write(Note[type][9]); break;
    case 10: Serial.write(Note[type][10]); break;
    case 11: Serial.write(Note[type][11]); break;
  }
  Serial.write(velocity);                             // Velocity (127이 파라미터로 들어온 경우 NOTE ON , 0이 파라미터로 들어온 경우 NOTE OFF) 볼륨의 크기로 터치의 ON/OFF를 컨트롤한다.
}




void PianoSounds(byte touch_num, byte velocity) {
  Serial.write(0xc0);                                 // 0xc0 : 악기 타입을 입력하겠다는 신호.
  Serial.write(Piano_type);                           // 악기 타입 입력(Piano_type은 1이고 옥타브만을 변경해줄 것이기 때문에 버튼에서는 옥타브 변경 변수만 ++시키고, 악기를 바꾸기 위해서는 배열이 필요하지 않음.

  Serial.write(0x90);                                 // 0x90 : 음계를 입력하겠다는 신호.
  switch (touch_num) {                                 // 터치에 입력된 값(몇번째 센서가 터치가 됐는지)에 따라 Serial 통신으로 음계번호 전달.
    case 0: Serial.write(Piano[octave][0]); break;
    case 1: Serial.write(Piano[octave][1]); break;
    case 2: Serial.write(Piano[octave][2]); break;
    case 3: Serial.write(Piano[octave][3]); break;
    case 4: Serial.write(Piano[octave][4]); break;
    case 5: Serial.write(Piano[octave][5]); break;
    case 6: Serial.write(Piano[octave][6]); break;
    case 7: Serial.write(Piano[octave][7]); break;
    case 8: Serial.write(Piano[octave][8]); break;
    case 9: Serial.write(Piano[octave][9]); break;
    case 10: Serial.write(Piano[octave][10]); break;
    case 11: Serial.write(Piano[octave][11]); break;
  }
  Serial.write(velocity);                             // Velocity (127이 파라미터로 들어온 경우 NOTE ON , 0이 파라미터로 들어온 경우 NOTE OFF) 볼륨의 크기로 터치의 ON/OFF를 컨트롤한다.
}


                                                      // 다양한 악기의 모드를 LCD에 출력해주기 위한 함수
void lcd_print(int play_Mode) {                       // parameter -> play_Mode : 현재 악기의 타입 번호 
  lcd.clear();                                        // LCD 모니터 초기화
  lcd.print("Mode: ");                                // MODE를 알려주기 위함
  lcd.setCursor(0, 1);                                // 글자의 위치를 바꿔주기 위함

  switch (play_Mode) {                                // 각각의 play_Mode마다 해당하는 컨셉이 LCD에 출력 되도록.
    case 0: lcd.print("Grand Piano"); break;          // 0 : 피아노
    case 1: lcd.print("Violin"); break;               // 1 : 바이올린
    case 2: lcd.print("Electic Guitar"); break;       // 2 : 일렉기타
    case 3: lcd.print("Enjoy Drum"); break;           // 3 : 드럼
  }
  delay(1000);                                        // 1초간 Delay
}

                                                     // 현재 Piano의 Octave를 LCD에 출력해주기 위한 함수  
void lcd_print_piano(int play_Mode) {                // parameter -> play_Mode : 현재 악기의 타입 번호 
  lcd.clear();                                       // LCD 모니터 초기화
  lcd.print("Piano Octave: ");                       // Piano octave를 알려주기 위함
  lcd.setCursor(0, 1);                               // 글자의 위치를 바꿔주기 위함

  switch (play_Mode) {                               // 각각의 play_Mode마다 해당하는 Octave가 LCD에 출력 되도록.
    case 0: lcd.print("1 LEVEL"); break;             // 옥타브 1
    case 1: lcd.print("2 LEVEL"); break;             // 옥타브 2
    case 2: lcd.print("3 LEVEL"); break;             // 옥타브 3
  }
  delay(1000);                                       // 1초간 Delay
}
