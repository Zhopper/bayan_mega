/*
 MIDI Баян (Аккордион, Гармонь, и т.д.)

 Для этой программы нужна плата MIDI Bayan Mega. Можно использовать Arduino Mega 2560, 
 но потребуются доработки.

 Создано 24.02.2017 (Поликарпов Александр)

 Эта программа для свободного распространения.

 Обсуждение платы и программы здесь:
 http://russian-garmon.ru/forum/obsluzhivanie-i-remont/29026-samodelnaya-midi-sistema-dlya-bayana-ili-garmoni
 */
#include <Wire.h>
#include <avr/pgmspace.h>
#include "ASOLED.h"
#include "MIDI.h"

// Выводы периферии, установленной на плате:
// Шина данных 0
#define D0_PC0 (1<<0)
#define D1_PC1 (1<<1)
#define D2_PC2 (1<<2)
#define D3_PC3 (1<<3)
#define D4_PC4 (1<<4)
#define D5_PC5 (1<<5)
#define D6_PC6 (1<<6)
#define D7_PC7 (1<<7)
// Шина адреса 0
#define L0_PA0 (1<<0)
#define L1_PA1 (1<<1)
#define L2_PA2 (1<<2)
#define L3_PA3 (1<<3)
#define L4_PA4 (1<<4)
#define L5_PA5 (1<<5)
#define L6_PA6 (1<<6)
#define L7_PA7 (1<<7)
#define L8_PB0 (1<<0)
#define L9_PB4 (1<<4)
#define L10_PB5 (1<<5)
#define L11_PB6 (1<<6)
#define L12_PB7 (1<<7)
#define L13_PD4 (1<<4)
#define L14_PD5 (1<<5)
#define L15_PD6 (1<<6)
#define L16_PD7 (1<<7)

// Шина данных 1
#define D0_PK0 (1<<0)
#define D1_PK1 (1<<1)
#define D2_PK2 (1<<2)
#define D3_PK3 (1<<3)
#define D4_PK4 (1<<4)
#define D5_PK5 (1<<5)
#define D6_PK6 (1<<6)
#define D7_PK7 (1<<7)
// Шина адреса 1
#define L0_PL0 (1<<0)
#define L1_PL1 (1<<1)
#define L2_PL2 (1<<2)
#define L3_PL3 (1<<3)
#define L4_PL4 (1<<4)
#define L5_PL5 (1<<5)
#define L6_PL6 (1<<6)
#define L7_PL7 (1<<7)
#define L8_PG0 (1<<0)
#define L9_PG1 (1<<1)
#define L10_PG2 (1<<2)
#define L11_PG3 (1<<3)
#define L12_PG4 (1<<4)
#define L13_PG5 (1<<5)
#define L14_PE4 (1<<4)
#define L15_PF6 (1<<6)
#define L16_PF7 (1<<7)

//Светодиоды
#define LED1_PJ5 (1<<5)
#define LED2_PJ6 (1<<6)
#define LED3_PJ7 (1<<7)

// Аналоговые входы
//#define PRESS_ADC0 PF0
//#define POT1_ADC1 PF1
//#define POT2_ADC2 PF2
//#define POT3_ADC3 PF3
//#define POT4_ADC4 PF4
//#define VBAT_ADC5 PF5

// I2C
//SDA_PD1
//SCL_PD0

typedef struct
{
  unsigned char mode;      // Биты 0,1 = Функция (0 Нота, 1 Регистр, 2 Управление), Бит 2 = инверсия, Биты 4,5,6,7 = Канал/ Биты 3,4,5,6,7 = Функция управления (0-Ок, 1-Лево, 2-Право, 3-Верх, 4-низ).
  unsigned char note_in1;  // Нота сжим 1
  unsigned char note_out1; // Нота разжим 1
  unsigned char debounce;  // Антидребезг, мс
} BUTTON;

typedef struct
{
  unsigned int min_value;
  unsigned int max_value;
  unsigned int zero_value;
  char function;
} ANALOG;

#define F_LINEAR 0
#define F_EXPO   1
#define F_LOG    2
#define F_SFUNC  3

#define MAX_BUTTONS 136    // Количество кнопок максимальное
#define MAX_ANALOG  6      // Количество аналоговых входов
#define MIDI_LOCK_CYCLES 4 // Количество циклов опроса кнопок до момента включения MIDI выхода. Сразу после включения выход заблокирован.

#define BTN_MODE_INVERSE (1<<2) // бит2 инверсия кнопки
#define BTN_CURRENT (1<<0) // бит0 = 0 отпущена, 1 нажата. (Текущее значение)
#define BTN_PREV    (1<<1) // бит1 = 0 отпущена, 1 нажата. (Предыдущее значение)
#define BTN_PRESS   (1<<2) // бит2 = 0 ничего,   1 была нажата. (Нажатие)
#define BTN_RELEASE (1<<3) // бит3 = 0 ничего,   1 была нажата. (Отпускание)
#define BTN         (1<<4) // бит0 = 0 отпущена, 1 нажата. (Текущее значение)

#define BTN_FUNC_OK    0x00
#define BTN_FUNC_LEFT  0x10
#define BTN_FUNC_RIGHT 0x20
#define BTN_FUNC_UP    0x30
#define BTN_FUNC_DOWN  0x40

typedef struct
{
  unsigned char max_buttons;
  unsigned char global_debounce;
  BUTTON        buttons_config[MAX_BUTTONS]; // конфигурация кнопок
  ANALOG        analog_config[MAX_ANALOG];   // конфигурация аналоговых входов
} CONTROL;

#define MENU_PARAMETERS 0
#define MENU_SETUP 1

#define MENU_BUTTONS 2
#define MENU_MIDI_IO 3
#define MAX_PARAMETERS 11

typedef struct
{
  char current_line;
  char current_byte;
  char midi_lock;
  unsigned int analog[MAX_ANALOG];
  unsigned char buttons_state[MAX_BUTTONS];  // здесь применяются маски BTN_CURRENT, BTN_PREV, BTN_PRESS, BTN_RELEASE
  unsigned char buttons_debounce_counter[MAX_BUTTONS];
  unsigned char current_menu;
  unsigned char current_parameter;
  unsigned char pressed_timer;
  unsigned char parameters[MAX_PARAMETERS]; //Voice=0,Volume=1,Variation=2,Reverb=3,Chorus=4,Transpose=5,Style=6,Tempo=7,Bank=8,Modulation=9
} DATA;

CONTROL control = {
136, // max_buttons
4,   // global_debounce
{0, 0x24, 0x24, 0, // Кнопка 0
 0, 0x25, 0x25, 0, // Кнопка 1
 0, 0x26, 0x26, 0, // Кнопка 2
 0, 0x27, 0x27, 0, // Кнопка 3
 0, 0x28, 0x28, 0, // Кнопка 4
 0, 0x29, 0x29, 0, // Кнопка 5
 0, 0x2A, 0x2A, 0, // Кнопка 6
 0, 0x2B, 0x2B, 0, // Кнопка 7
 0, 0x2C, 0x2C, 0, // Кнопка 8
 0, 0x2D, 0x2D, 0, // Кнопка 9
 0, 0x2E, 0x2E, 0, // Кнопка 10
 0, 0x2F, 0x2F, 0, // Кнопка 11
 0, 0x30, 0x30, 0, // Кнопка 12
 0, 0x31, 0x31, 0, // Кнопка 13
 0, 0x32, 0x32, 0, // Кнопка 14
 0, 0x33, 0x33, 0, // Кнопка 15
 0, 0x34, 0x34, 0, // Кнопка 16
 0, 0x35, 0x35, 0, // Кнопка 17
 0, 0x36, 0x36, 0, // Кнопка 18
 0, 0x37, 0x37, 0, // Кнопка 19
 0, 0x38, 0x38, 0, // Кнопка 20
 0, 0x39, 0x39, 0, // Кнопка 21
 0, 0x3A, 0x3A, 0, // Кнопка 22
 0, 0x3B, 0x3B, 0, // Кнопка 23
 0, 0x3C, 0x3C, 0, // Кнопка 24
 0, 0x3D, 0x3D, 0, // Кнопка 25
 0, 0x3E, 0x3E, 0, // Кнопка 26
 0, 0x3F, 0x3F, 0, // Кнопка 27
 0, 0x40, 0x40, 0, // Кнопка 28
 0, 0x41, 0x41, 0, // Кнопка 29
 0, 0x42, 0x42, 0, // Кнопка 30
 0, 0x43, 0x43, 0, // Кнопка 31
 0, 0x44, 0x44, 0, // Кнопка 32
 0, 0x45, 0x45, 0, // Кнопка 33
 0, 0x46, 0x46, 0, // Кнопка 34
 0, 0x47, 0x47, 0, // Кнопка 35
 0, 0x48, 0x48, 0, // Кнопка 36
 0, 0x49, 0x49, 0, // Кнопка 37
 0, 0x4A, 0x4A, 0, // Кнопка 38
 0, 0x4B, 0x4B, 0, // Кнопка 39
 0, 0x4C, 0x4C, 0, // Кнопка 40
 0, 0x4C, 0x4C, 0, // Кнопка 41
 0, 0x4C, 0x4C, 0, // Кнопка 42
 0, 0x4C, 0x4C, 0, // Кнопка 43
 0, 0x4C, 0x4C, 0, // Кнопка 44
 0, 0x4C, 0x4C, 0, // Кнопка 45
 0, 0x4C, 0x4C, 0, // Кнопка 46
 0, 0x4C, 0x4C, 0, // Кнопка 47
 0, 0x4C, 0x4C, 0, // Кнопка 48
 0, 0x4C, 0x4C, 0, // Кнопка 49
 0, 0x4C, 0x4C, 0, // Кнопка 50
 0, 0x4C, 0x4C, 0, // Кнопка 51
 0, 0x4C, 0x4C, 0, // Кнопка 52
 0, 0x4C, 0x4C, 0, // Кнопка 53
 0, 0x4C, 0x4C, 0, // Кнопка 54
 0, 0x4C, 0x4C, 0, // Кнопка 55
 0, 0x4C, 0x4C, 0, // Кнопка 56
 0, 0x4C, 0x4C, 0, // Кнопка 57
 0, 0x4C, 0x4C, 0, // Кнопка 58
 0, 0x4C, 0x4C, 0, // Кнопка 59
 0, 0x4C, 0x4C, 0, // Кнопка 60
 0, 0x4C, 0x4C, 0, // Кнопка 61
 0, 0x4C, 0x4C, 0, // Кнопка 62
 0, 0x4C, 0x4C, 0, // Кнопка 63
 0, 0x4C, 0x4C, 0, // Кнопка 64
 0, 0x4C, 0x4C, 0, // Кнопка 65
 0, 0x4C, 0x4C, 0, // Кнопка 66
 0, 0x4C, 0x4C, 0, // Кнопка 67
 0, 0x4C, 0x4C, 0, // Кнопка 68
 0, 0x4C, 0x4C, 0, // Кнопка 69
 0, 0x4C, 0x4C, 0, // Кнопка 70
 0, 0x4C, 0x4C, 0, // Кнопка 71
 0, 0x4C, 0x4C, 0, // Кнопка 72
 0, 0x4C, 0x4C, 0, // Кнопка 73
 0, 0x4C, 0x4C, 0, // Кнопка 74
 0, 0x4C, 0x4C, 0, // Кнопка 75
 0, 0x4C, 0x4C, 0, // Кнопка 76
 0, 0x4C, 0x4C, 0, // Кнопка 77
 0, 0x4C, 0x4C, 0, // Кнопка 78
 0, 0x4C, 0x4C, 0, // Кнопка 79
 0, 0x4C, 0x4C, 0, // Кнопка 80
 0, 0x4C, 0x4C, 0, // Кнопка 81
 0, 0x4C, 0x4C, 0, // Кнопка 82
 0, 0x4C, 0x4C, 0, // Кнопка 83
 0, 0x4C, 0x4C, 0, // Кнопка 84
 0, 0x4C, 0x4C, 0, // Кнопка 85
 0, 0x4C, 0x4C, 0, // Кнопка 86
 0, 0x4C, 0x4C, 0, // Кнопка 87
 0, 0x4C, 0x4C, 0, // Кнопка 88
 0, 0x4C, 0x4C, 0, // Кнопка 89
 0, 0x4C, 0x4C, 0, // Кнопка 90
 0, 0x4C, 0x4C, 0, // Кнопка 91
 0, 0x4C, 0x4C, 0, // Кнопка 92
 0, 0x4C, 0x4C, 0, // Кнопка 93
 0, 0x4C, 0x4C, 0, // Кнопка 94
 0, 0x4C, 0x4C, 0, // Кнопка 95
 0, 0x4C, 0x4C, 0, // Кнопка 96
 0, 0x4C, 0x4C, 0, // Кнопка 97
 0, 0x4C, 0x4C, 0, // Кнопка 98
 0, 0x4C, 0x4C, 0, // Кнопка 99
 0, 0x4C, 0x4C, 0, // Кнопка 100
 0, 0x4C, 0x4C, 0, // Кнопка 101
 0, 0x4C, 0x4C, 0, // Кнопка 102
 0, 0x4C, 0x4C, 0, // Кнопка 103
 0, 0x4C, 0x4C, 0, // Кнопка 104
 0, 0x4C, 0x4C, 0, // Кнопка 105
 0, 0x4C, 0x4C, 0, // Кнопка 106
 0, 0x4C, 0x4C, 0, // Кнопка 107
 0, 0x4C, 0x4C, 0, // Кнопка 108
 0, 0x4C, 0x4C, 0, // Кнопка 109
 0, 0x4C, 0x4C, 0, // Кнопка 110
 0, 0x4C, 0x4C, 0, // Кнопка 111
 0, 0x4C, 0x4C, 0, // Кнопка 112
 0, 0x4C, 0x4C, 0, // Кнопка 113
 0, 0x4C, 0x4C, 0, // Кнопка 114
 0, 0x4C, 0x4C, 0, // Кнопка 115
 0, 0x4C, 0x4C, 0, // Кнопка 116
 0, 0x4C, 0x4C, 0, // Кнопка 117
 0, 0x4C, 0x4C, 0, // Кнопка 118
 0, 0x4C, 0x4C, 0, // Кнопка 119
 0, 0x4C, 0x4C, 0, // Кнопка 120
 0, 0x4C, 0x4C, 0, // Кнопка 121
 0, 0x4C, 0x4C, 0, // Кнопка 122
 0, 0x4C, 0x4C, 0, // Кнопка 123
 0, 0x4C, 0x4C, 0, // Кнопка 124
 0, 0x4C, 0x4C, 0, // Кнопка 125
 0, 0x4C, 0x4C, 0, // Кнопка 126
 0, 0x4C, 0x4C, 0, // Кнопка 127
 /*
 0, 0x3C, 0x3C, 0x00, 0x00, 0x00, 0, // Кнопка 128
 0, 0x3E, 0x3E, 0x00, 0x00, 0x00, 0, // Кнопка 129
 0, 0x40, 0x40, 0x00, 0x00, 0x00, 0, // Кнопка 130
 0, 0x41, 0x41, 0x00, 0x00, 0x00, 0, // Кнопка 131
 0, 0x43, 0x43, 0x00, 0x00, 0x00, 0, // Кнопка 132
*/
 
 0x42, 0x3C, 0x3C, 0, // Кнопка 128 низ
 0x22, 0x3E, 0x3E, 0, // Кнопка 129 право
 0x02, 0x40, 0x40, 0, // Кнопка 130 ок
// 0, 0x40, 0x40, 0, // Кнопка 130

 0x12, 0x41, 0x41, 0, // Кнопка 131 лево
 0x32, 0x43, 0x43, 0, // Кнопка 132 верх

 0, 0x4C, 0x4C, 0, // Кнопка 133
 0, 0x4C, 0x4C, 0, // Кнопка 134
 0, 0x4C, 0x4C, 0, // Кнопка 135
 },
{0,991,512,0,
 0,991,512,0,
 0,991,512,0,
 0,991,512,0,
 0,991,512,0,
 0,991,512,0}
};
DATA common_data;

unsigned char str_parameters[11][12] ={
  {"Program    \0",},
  {"Volume     \0",},
  {"Variation  \0",},
  {"Reverb     \0",},
  {"Chorus     \0",},
  {"Transpose  \0",},
  {"Style      \0",},
  {"Tempo      \0",},
  {"Bank       \0",},
  {"Modulation \0",},
  {"Setup      \0",}  
};

void setup() {
  // Инициализация последовательных портов
  Serial.begin(57600);  // Отладочный порт, подключенный к USB-UART (PE0, PE1)
  Serial1.begin(31250); // Порт MIDI1 (PD2, PD3)
  Serial2.begin(31250); // Порт MIDI2 (PH0, PH1)
  
  common_data.midi_lock = MIDI_LOCK_CYCLES;
  
  // Инициализация линий D0 - D7 шины данных 0 на вход  
  DDRC  &= ~(D0_PC0 | D1_PC1 | D2_PC2 | D3_PC3 | D4_PC4 | D5_PC5 | D6_PC6 | D7_PC7); // Данные с холлов на вход
  PORTC &= ~(D0_PC0 | D1_PC1 | D2_PC2 | D3_PC3 | D4_PC4 | D5_PC5 | D6_PC6 | D7_PC7); // Установить нули
  // Инициализация линий L0 - L16 шины данных 0 на выход
  DDRA  |=  (L0_PA0 | L1_PA1 | L2_PA2 | L3_PA3 | L4_PA4 | L5_PA5 | L6_PA6 | L7_PA7); // На выход для управления холлами
  PORTA &=  ~(L0_PA0 | L1_PA1 | L2_PA2 | L3_PA3 | L4_PA4 | L5_PA5 | L6_PA6 | L7_PA7); // На всех холлах единицы
  DDRB  |=  (L8_PB0 | L9_PB4 | L10_PB5 | L11_PB6 | L12_PB7); // На выход для управления холлами
  PORTB |=  (L8_PB0 | L9_PB4 | L10_PB5 | L11_PB6 | L12_PB7); // На всех холлах единицы
  DDRD  |=  (L13_PD4 | L14_PD5 | L15_PD6 | L16_PD7); // На выход для управления холлами
  PORTD |=  (L13_PD4 | L14_PD5 | L15_PD6 | L16_PD7); // На всех холлах единицы
/*
  // Инициализация линий D0 - D7 шины данных 1 на вход  
  DDRK  &= ~(D0_PK0 | D1_PK1 | D2_PK2 | D3_PK3 | D4_PK4 | D5_PK5 | D6_PK6 | D7_PK7); // Данные с холлов на вход
  PORTK &= ~(D0_PK0 | D1_PK1 | D2_PK2 | D3_PK3 | D4_PK4 | D5_PK5 | D6_PK6 | D7_PK7); // Установить нули
  // Инициализация линий L0 - L16 шины данных 1 на выход
  DDRL  |=  (L0_PL0 | L1_PL1 | L2_PL2 | L3_PL3 | L4_PL4 | L5_PL5 | L6_PL6 | L7_PL7); // На выход для управления холлами
  PORTL |=  (L0_PL0 | L1_PL1 | L2_PL2 | L3_PL3 | L4_PL4 | L5_PL5 | L6_PL6 | L7_PL7); // На всех холлах единицы
  DDRG  |=  (L8_PG0 | L9_PG1 | L10_PG2 | L11_PG3 | L12_PG4 | L13_PG5); // На выход для управления холлами
  PORTG |=  (L8_PG0 | L9_PG1 | L10_PG2 | L11_PG3 | L12_PG4 | L13_PG5); // На всех холлах единицы
  DDRE  |=  (L14_PE4); // На выход для управления холлами
  PORTE |=  (L14_PE4); // На всех холлах единицы
*/
  DDRF  |=  (L15_PF6 | L16_PF7); // На выход для управления холлами
  PORTF |=  (L15_PF6 | L16_PF7); // На всех холлах единицы
  LD.init();  // Инициализация дисплея
  LD.clearDisplay(); // Очистка дисплея
}

char j = 0;

// В цикле происходит опрос клавиш, датчика давления, потенциометров и отправка MIDI команд
void loop() {
  ProcessInput();
  ReadAnalog();
  ReadButtons();
  ProcessButtons();
  ProcessAnalog();
}

////////////////////////////////
// Чтение всех аналоговых входов
////////////////////////////////
void ReadAnalog(void) {
  // Считать значение потенциометров и датчика давления
  for (int ii=0;ii<MAX_ANALOG;ii++) common_data.analog[ii] = analogRead(ii);
}

///////////////
// Опрос кнопок
///////////////
void ReadButtons(void) {
  // При каждом выполнении этой функции опрашивается только одна из линий
  // В каждой линии 8 кнопок
  if (0  == common_data.current_line) PORTA |=  (L0_PA0);         // Выбрана линия L0 порта 0
  if (1  == common_data.current_line) PORTA |=  (L1_PA1);         // Выбрана линия L1 порта 0
  if (2  == common_data.current_line) PORTA |=  (L2_PA2);         // Выбрана линия L2 порта 0
  if (3  == common_data.current_line) PORTA |=  (L3_PA3);         // Выбрана линия L3 порта 0
  if (4  == common_data.current_line) PORTA |=  (L4_PA4);         // Выбрана линия L4 порта 0
  if (5  == common_data.current_line) PORTA |=  (L5_PA5);         // Выбрана линия L5 порта 0
  if (6  == common_data.current_line) PORTA |=  (L6_PA6);         // Выбрана линия L6 порта 0
  if (7  == common_data.current_line) PORTA |=  (L7_PA7);         // Выбрана линия L7 порта 0
  if (8  == common_data.current_line) PORTB |=  (L8_PB0);         // Выбрана линия L8 порта 0
  if (9  == common_data.current_line) PORTB |=  (L9_PB4);         // Выбрана линия L9 порта 0
  if (10 == common_data.current_line) PORTB |= (L10_PB5);         // Выбрана линия L10 порта 0
  if (11 == common_data.current_line) PORTB |= (L11_PB6);         // Выбрана линия L11 порта 0
  if (12 == common_data.current_line) PORTB |= (L12_PB7);         // Выбрана линия L12 порта 0
  if (13 == common_data.current_line) PORTD |= (L13_PD4);         // Выбрана линия L13 порта 0
  if (14 == common_data.current_line) PORTD |= (L14_PD5);         // Выбрана линия L14 порта 0
  if (15 == common_data.current_line) PORTD |= (L15_PD6);         // Выбрана линия L15 порта 0
//  if (16 == common_data.current_line) PORTD |= (L16_PD7);         // Выбрана линия L16 порта 0
  if (16 == common_data.current_line) PORTF &=~(L16_PF7);         // Выбрана линия L16 порта 1

  for (volatile unsigned int a=0;a<500;a++);                      // Задержка на время включения датчиков Холла
  common_data.current_byte = PINC;                                // Считать байт данных из линий D0 - D7 порта 0
  for (int current_bit; current_bit<8; current_bit++)             // Перебрать все биты порта и запомнить состояние кнопок
  {
    unsigned int current_button = common_data.current_line*8 + current_bit; // Вычислить текущий номер кнопки
    unsigned char changed_flag = 0;
    // ЧТЕНИЕ СОСТОЯНИЯ КНОПКИ
    if (common_data.buttons_debounce_counter[current_button]==0) {  // Если таймер антидребезга = 0, разрешено изменение
      if ((common_data.current_byte>>current_bit) & 1) {            // Если текущий бит равен "1"
        if (common_data.buttons_state[current_button] & BTN_CURRENT == 0) // Текущее значение было 0
          changed_flag = 1;                                         // Значит оно изменилось - выставить флаг
        common_data.buttons_state[current_button] |= BTN_CURRENT;   // Текущее состояние выставить в "1"
      } else {                                                      // Если текущий бит равен "0"
        if (common_data.buttons_state[current_button] & BTN_CURRENT != 0) // Текущее значение было 1
          changed_flag = 1;                                         // Значит оно изменилось - выставить флаг 
        common_data.buttons_state[current_button] &=~BTN_CURRENT;   // Текущее состояние выставить в "0"
      }  
    }
    // АНТИДРЕБЕЗГ КНОПОК
    if (changed_flag == 1) {                                        // Если состояние кнопки изменилось
      if (control.buttons_config[current_button].debounce)          // Если для текущей кнопки задано индивидуальное значение антидребезга 
        common_data.buttons_debounce_counter[current_button] = control.buttons_config[current_button].debounce; // Задать индивидуальный таймер антидребезга
      else                                                          // Если не задано
        common_data.buttons_debounce_counter[current_button] = control.global_debounce; // Задать глобальный таймер антидребезга
    }
    if (common_data.buttons_debounce_counter[current_button]) common_data.buttons_debounce_counter[current_button]--; // Уменьшить таймер на 1, если он не ноль.
    // ПЕРЕХОД ИЗ 0 В 1
    if (((common_data.buttons_state[current_button] & BTN_CURRENT) !=0) && (common_data.buttons_state[current_button] & BTN_PREV) ==0) { // Был переход из "0" в "1"
      if ((control.buttons_config[current_button].mode & BTN_MODE_INVERSE) == 0) { // Если в конфигурации кнопки нет инверсии
        common_data.buttons_state[current_button] |= BTN_RELEASE; // Флаг отпускания выставить в "1"
      } else {                                                    // Если есть инверсия
        common_data.buttons_state[current_button] |= BTN_PRESS;   // Флаг нажатия выставить в "1"
      }
    }
    // ПЕРЕХОД ИЗ 1 В 0
    if (((common_data.buttons_state[current_button] & BTN_CURRENT) ==0) && (common_data.buttons_state[current_button] & BTN_PREV) !=0) { // Был переход из "1" в "0"
      if ((control.buttons_config[current_button].mode & BTN_MODE_INVERSE) == 0) { // Если в конфигурации кнопки нет инверсии
        common_data.buttons_state[current_button] |= BTN_PRESS;   // Флаг нажатия выставить в "1"
      } else {                                                    // Если есть инверсия
        common_data.buttons_state[current_button] |= BTN_RELEASE; // Флаг отпускания выставить в "1"
      }
    }
    // ОБНОВИТЬ ПРЕДЫДУЩЕЕ СОСТОЯНИЕ КНОПКИ
    if ((common_data.buttons_state[current_button] & BTN_CURRENT) !=0)  // Если текущий бит равен "1"
      common_data.buttons_state[current_button] |= BTN_PREV;      // Предыдущее состояние выставить в "1"
    else                                                          // Если текущий бит равен "0"
      common_data.buttons_state[current_button] &=~BTN_PREV;      // Предыдущее состояние выставить в "0"
  }
  
  if (0  == common_data.current_line) PORTA &= ~(L0_PA0);         // Выбрана линия L0 порта 0
  if (1  == common_data.current_line) PORTA &= ~(L1_PA1);         // Выбрана линия L1 порта 0
  if (2  == common_data.current_line) PORTA &= ~(L2_PA2);         // Выбрана линия L2 порта 0
  if (3  == common_data.current_line) PORTA &= ~(L3_PA3);         // Выбрана линия L3 порта 0
  if (4  == common_data.current_line) PORTA &= ~(L4_PA4);         // Выбрана линия L4 порта 0
  if (5  == common_data.current_line) PORTA &= ~(L5_PA5);         // Выбрана линия L5 порта 0
  if (6  == common_data.current_line) PORTA &= ~(L6_PA6);         // Выбрана линия L6 порта 0
  if (7  == common_data.current_line) PORTA &= ~(L7_PA7);         // Выбрана линия L7 порта 0
  if (8  == common_data.current_line) PORTB &= ~(L8_PB0);         // Выбрана линия L8 порта 0
  if (9  == common_data.current_line) PORTB &= ~(L9_PB4);         // Выбрана линия L9 порта 0
  if (10 == common_data.current_line) PORTB &=~(L10_PB5);         // Выбрана линия L10 порта 0
  if (11 == common_data.current_line) PORTB &=~(L11_PB6);         // Выбрана линия L11 порта 0
  if (12 == common_data.current_line) PORTB &=~(L12_PB7);         // Выбрана линия L12 порта 0
  if (13 == common_data.current_line) PORTD &=~(L13_PD4);         // Выбрана линия L13 порта 0
  if (14 == common_data.current_line) PORTD &=~(L14_PD5);         // Выбрана линия L14 порта 0
  if (15 == common_data.current_line) PORTD &=~(L15_PD6);         // Выбрана линия L15 порта 0
//  if (16 == common_data.current_line) PORTD |= (L16_PD7);         // Выбрана линия L16 порта 0
  if (16 == common_data.current_line) PORTF |= (L16_PF7);         // Выбрана линия L16 порта 1

  common_data.current_line++;                                     // Следующее значение линии L
  if (common_data.current_line>16) 
  {
    common_data.current_line=0;    // Если дошли до крайней линии, то начать с нулевой.
    if (common_data.midi_lock) common_data.midi_lock--; // Разблокировка MIDI выхода после нескольких опросов всех кнопок.
  }
}

/////////////////////////////////////////////////
void ProcessButtons(void) {
  #define MAX_PRESSED_TIMER_SLOW 120
  #define MAX_PRESSED_TIMER_FAST 10
  #define FAST_REPEAT_DELAY 2
  static unsigned char idle_prev; // Предыдущее значение флага сброса всех нот.
  static unsigned char fast_repeat_counter = FAST_REPEAT_DELAY;
  unsigned char idle=1; // Флаг сброса всех нот, разрешающий сброс.
  // Перебрать все кнопки
  for (unsigned int current_button; current_button<MAX_BUTTONS; current_button++) {
    if (common_data.buttons_state[current_button] & BTN_PRESS) {  // Если кнопка была нажата
      common_data.buttons_state[current_button] &= ~BTN_PRESS;    // Сбросить флаг нажатия
      common_data.buttons_state[current_button] |=  BTN;          // Установить флаг нажатой и удерживаемой кнопки
      Serial.print("pressed ");Serial.println(current_button,DEC);
      if ((control.buttons_config[current_button].mode & 0x03) == 0) { // Если у кнопки режим - нота
        Command3(MIDI_NOTE_ON, control.buttons_config[current_button].note_in1 , 127,3);
      }
      if ((control.buttons_config[current_button].mode & 0x03) == 1) { // Если у кнопки режим - регистр
        // TODO: register
      }
      if ((control.buttons_config[current_button].mode & 0x03) == 2) { // Если у кнопки режим - управление
        ProcessControlButtons(control.buttons_config[current_button].mode & 0xF8); // Передать код кнопки в функцию управления
        common_data.pressed_timer = MAX_PRESSED_TIMER_SLOW;
        fast_repeat_counter = FAST_REPEAT_DELAY;
      }
    }
    if (common_data.buttons_state[current_button] & BTN_RELEASE) {  // Если кнопка была отпущена
      common_data.buttons_state[current_button] &= ~BTN_RELEASE;    // Сбросить флаг отпускания
      common_data.buttons_state[current_button] &= ~BTN;            // Сбросить флаг нажатой и удерживаемой кнопки
      Serial.print("released ");Serial.println(current_button,DEC);
      if ((control.buttons_config[current_button].mode & 0x03) == 0) { // Если у кнопки режим - нота
        Command3(MIDI_NOTE_OFF, control.buttons_config[current_button].note_in1 , 0,3);
      }
      if ((control.buttons_config[current_button].mode & 0x03) == 1) { // Если у кнопки режим - регистр
        // TODO: register
      }
      if ((control.buttons_config[current_button].mode & 0x03) == 2) { // Если у кнопки режим - управление
        // TODO: control
      }
    }
    if (common_data.buttons_state[current_button] & BTN) {  // Если кнопка была нажата и удерживается
      if ((control.buttons_config[current_button].mode & 0x03) == 0) { // Если у кнопки режим - нота
        idle=0;                                               // Запретить сброс всех нот
      }
      if ((control.buttons_config[current_button].mode & 0x03) == 2) { // Если у кнопки режим - управление
        if (common_data.pressed_timer) {
          common_data.pressed_timer--;
        } else {
          if (fast_repeat_counter) {
            fast_repeat_counter--;
            common_data.pressed_timer=MAX_PRESSED_TIMER_SLOW;
          } else {
            common_data.pressed_timer=MAX_PRESSED_TIMER_FAST;
          }
          ProcessControlButtons(control.buttons_config[current_button].mode & 0xF8); // Передать код кнопки в функцию управления
        }
      }
    }
  }
  /*
  if (idle_prev != idle && idle == 1) { // Если сброс всех нот изменился и он разрешён
    for (int channel=0;channel<16;channel++)
      Command3 ((MIDI_CONTROL_CHANGE | channel),0x7B,0,3); // Отправить команду сброса всех нот на все каналы
    Serial.println("cleared ");
  }
  idle_prev = idle; // Обновить предыдущее значение
  */
}

void ProcessAnalog(void) {
  static char prev_vol=0;
  static char prev_pitch=0;
  static char prev_mod =0;
  char vol   = (long int)127*common_data.analog[0]/1024;
  char pitch = (long int)127*common_data.analog[1]/1024;
  char mod   = (long int)127*common_data.analog[2]/1024;

  if (prev_vol!=vol)
      Command3(MIDI_CONTROL_CHANGE, MIDI_CONTROL_EXPRESSION,vol,3);
  if (prev_pitch!=pitch)
      Command3(MIDI_PITCH_WHEEL, 0x07,pitch,3);
//  if (prev_mod!=mod)
//      Command3(MIDI_CONTROL_CHANGE, MIDI_CONTROL_MODULATION_WHEEL, mod, 3);    
  prev_vol=vol;
  prev_pitch=pitch;
  prev_mod=mod;
}

void ProcessControlButtons(char button_code) {
  static char update_flag = 0;

  // МЕНЮ НАСТРОЕК
  if (common_data.current_menu == MENU_SETUP)  {
    if (button_code == BTN_FUNC_LEFT) {
//      if (common_data.parameters[common_data.current_parameter]>0) common_data.parameters[common_data.current_parameter]--;
      update_flag = 1;
    }
    if (button_code == BTN_FUNC_RIGHT) {
//      if (common_data.parameters[common_data.current_parameter]<MIDI_MAX_VALUE) common_data.parameters[common_data.current_parameter]++;
      update_flag = 1;
    }
    if (button_code == BTN_FUNC_UP) {
//      if (common_data.current_parameter>0) common_data.current_parameter--;
      update_flag = 1;
    }
    if (button_code == BTN_FUNC_DOWN) {
//      if (common_data.current_parameter<MAX_PARAMETERS-1) common_data.current_parameter++;
      update_flag = 1;
    }
    if (button_code == BTN_FUNC_OK) {
      if (common_data.current_parameter == 0x00) { // Пункт меню BACK, нажата кнопка OK
        common_data.current_menu = MENU_PARAMETERS;// Поменять текущее меню на PARAMETERS
        common_data.current_parameter = 0x00;      // Текущий параметр - первый
        Serial.println("MENU: PARAMETERS ");
        return;                                    // Выйти из функции
      }
    }
    if (update_flag == 1) {
      update_flag = 0;
      Serial.println("MENU SETUP useless text");
      Serial.print("Parameter: ");Serial.println(common_data.current_parameter,DEC);        
    }
  }
  
  // МЕНЮ ПАРАМЕТРОВ
  if (common_data.current_menu == MENU_PARAMETERS)  {
    if (button_code == BTN_FUNC_LEFT) {
      if (common_data.parameters[common_data.current_parameter]>0) common_data.parameters[common_data.current_parameter]--;
      update_flag = 1;
    }
    if (button_code == BTN_FUNC_RIGHT) {
      if (common_data.parameters[common_data.current_parameter]<MIDI_MAX_VALUE) common_data.parameters[common_data.current_parameter]++;
      update_flag = 1;
    }
    if (button_code == BTN_FUNC_UP) {
      if (common_data.current_parameter>0) common_data.current_parameter--;
      update_flag = 1;
    }
    if (button_code == BTN_FUNC_DOWN) {
      if (common_data.current_parameter<MAX_PARAMETERS-1) common_data.current_parameter++;
      update_flag = 1;
    }
    if (button_code == BTN_FUNC_OK) {
      if (common_data.current_parameter == 0x0A) { // Пункт меню Setup, нажата кнопка OK
        common_data.current_menu = MENU_SETUP;     // Поменять текущее меню на SETUP
        common_data.current_parameter = 0x00;      // Текущий параметр - первый
        update_flag = 1;
        Serial.println("MENU: SETUP ");
        return;                                    // Выйти из функции
      }
    }
    if (update_flag == 1) {
      update_flag = 0;
      Serial.print("Parameter: ");Serial.println(common_data.current_parameter,DEC);
      Serial.print("Value: "); Serial.println(common_data.parameters[common_data.current_parameter],DEC);
      LD.printString_12x16(&(str_parameters[common_data.current_parameter][0]), 0, 2);
      char pos = LD.printNumber((float)common_data.parameters[common_data.current_parameter],3, 0, 4);
      LD.printString_12x16("   ", pos*12, 4);
      if (common_data.current_parameter == 0x00) { // Voice
        char sound_name[MIDI_NAME_LENGTH];
        for (int index=0;index<MIDI_NAME_LENGTH;index++)
          sound_name[index] = pgm_read_byte(&sounds_names[common_data.parameters[0x00]][index]);
        LD.printString_6x8(sound_name, 0, 6);
        for (int channel=0;channel<16;channel++)
          Command2 ((MIDI_PROGRAM_CHANGE | channel),common_data.parameters[common_data.current_parameter],3);
      } else {
        char sound_name[MIDI_NAME_LENGTH];
        for (int index=0;index<MIDI_NAME_LENGTH;index++)
          sound_name[index] = pgm_read_byte(&sounds_names[128][index]);
        LD.printString_6x8(sound_name, 0, 6);
      }
      
      if (common_data.current_parameter == 0x01) { // Volume
        for (int channel=0;channel<16;channel++)
          Command3 ((MIDI_CONTROL_CHANGE | channel),0x07,common_data.parameters[common_data.current_parameter],3);
      };
      if (common_data.current_parameter == 0x02) { // Variation
        
      };
      if (common_data.current_parameter == 0x03) { // Reverb
        Command3(MIDI_CONTROL_CHANGE, MIDI_CONTROL_REVERB, common_data.parameters[common_data.current_parameter], 3);    
      };
      if (common_data.current_parameter == 0x04) { // Chorus
        Command3(MIDI_CONTROL_CHANGE, MIDI_CONTROL_CHORUS, common_data.parameters[common_data.current_parameter], 3);    
      };
      if (common_data.current_parameter == 0x08) { // Bank
        Command3(MIDI_CONTROL_CHANGE, 0x00,common_data.parameters[common_data.current_parameter],3); // Bank Select
      };
      if (common_data.current_parameter == 0x09) { // Modulation
        Command3(MIDI_CONTROL_CHANGE, MIDI_CONTROL_MODULATION_WHEEL, common_data.parameters[common_data.current_parameter], 3);    
      };

  /*
  {"Voice      \0",},
  {"Volume     \0",},
  {"Variation  \0",},
  {"Reverb     \0",},
  {"Chorus     \0",},
  {"Transpose  \0",},
  {"Style      \0",},
  {"Tempo      \0",},
  {"Bank       \0",},
  {"Modulation \0",}
  */

    }
  }
/*
#define MENU_SETUP 1
#define MENU_BUTTONS 2
#define MENU_MIDI_IO 3

typedef struct
{
  char current_line;
  char current_byte;
  char midi_lock;
  unsigned int analog[MAX_ANALOG];
  unsigned char buttons_state[MAX_BUTTONS];  // здесь применяются маски BTN_CURRENT, BTN_PREV, BTN_PRESS, BTN_RELEASE
  unsigned char buttons_debounce_counter[MAX_BUTTONS];
  char current_menu;
  char current_parameter;
  char parameters[10]; //Voice=0,Volume=1,Variation=2,Reverb=3,Chorus=4,Transpose=5,Style=6,Tempo=7,Bank=8,Modulation=9
} DATA;
*/
  
}

/*
#define MIDI_NOTE_ON 0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_POLY_KEY 0xA0
#define MIDI_CONTROL_CHANGE 0xB0
#define MIDI_PROGRAM_CHANGE 0xC0
#define MIDI_CHANNEL_PRESSURE 0xD0
#define MIDI_PITCH_WHEEL 0xE0

//  plays a MIDI note.  Doesn't check to see that
//  cmd is greater than 127, or that data values are  less than 127:
//  Функция проигрывающая ноту
void noteOn(int cmd, int pitch, int velocity) {
  if (common_data.midi_lock == 0) {
    Serial1.write(cmd);
    Serial1.write(pitch);
    Serial1.write(velocity);
    Serial2.write(cmd);
    Serial2.write(pitch);
    Serial2.write(velocity);
  }  
*/

// Передать команду MIDI из двух байт. channels определяет в какой из выходов её передать. 1,2-отдельные. 3-оба.
void Command2 (char byte1, char byte2, char channels) {
  if (common_data.midi_lock == 0) {
    if (channels & 1) {
      Serial1.write(byte1);
      Serial1.write(byte2);
    }
    if (channels & 2) {
      Serial2.write(byte1);
      Serial2.write(byte2);
    }
    for (volatile unsigned int a=0;a<50;a++);
  }  
}

// Передать команду MIDI из трёх байт. channels определяет в какой из выходов её передать. 1,2-отдельные. 3-оба.
void Command3 (char byte1, char byte2, char byte3, char channels) {
  if (common_data.midi_lock == 0) {
    if (channels & 1) {
      Serial1.write(byte1);
      Serial1.write(byte2);
      Serial1.write(byte3);
    }
    if (channels & 2) {
      Serial2.write(byte1);
      Serial2.write(byte2);
      Serial2.write(byte3);
    }
    for (volatile unsigned int a=0;a<50;a++);
  }  
}
  
void ProcessInput(void) {
  if (Serial1.available() > 0) {
    static int prev;
    int incomingByte = Serial1.read();
    if (incomingByte > 0x7F) Serial.println("");
    switch (incomingByte & 0xF0)
    {
      case 0x80:
        Serial.print("Note Off, Ch="); Serial.print(incomingByte & 0x0F, DEC); Serial.print(" ");
      break;
      case 0x90:
        Serial.print("Note On, Ch="); Serial.print(incomingByte & 0x0F, DEC); Serial.print(" ");
      break;
      case 0xA0:
        Serial.print("Polyphonic key pressure, Ch="); Serial.print(incomingByte & 0x0F, DEC); Serial.print(" ");
      break;
      case 0xB0:
        Serial.print("Control Change, Ch="); Serial.print(incomingByte & 0x0F, DEC); Serial.print(" ");
      break;
      case 0xC0:
        Serial.print("Program Change, Ch="); Serial.print(incomingByte & 0x0F, DEC); Serial.print(" ");
      break;
      case 0xD0:
        Serial.print("Channel Pressure, Ch="); Serial.print(incomingByte & 0x0F, DEC); Serial.print(" ");
      break;
      case 0xE0:
        Serial.print("Pitch Wheel Change, Ch="); Serial.print(incomingByte & 0x0F, DEC); Serial.print(" ");
      break;
      default:
        switch (prev & 0xF0) {
          case 0x80:
          {
            char note_name[MIDI_NOTE_NAME_LENGTH];
            for (int index=0;index<MIDI_NOTE_NAME_LENGTH;index++)
              note_name[index] = pgm_read_byte(&notes_names[incomingByte][index]);
            Serial.print("Note=");
            Serial.print(note_name[0]);
            Serial.print(note_name[1]);
            Serial.print(note_name[2]);
            Serial.print(" ");
          }  
          break;
          case 0x90:
          {
            char note_name[MIDI_NOTE_NAME_LENGTH];
            for (int index=0;index<MIDI_NOTE_NAME_LENGTH;index++)
              note_name[index] = pgm_read_byte(&notes_names[incomingByte][index]);
            Serial.print("Note=");
            Serial.print(note_name[0]);
            Serial.print(note_name[1]);
            Serial.print(note_name[2]);
            Serial.print(" ");
          }  
          break;
          case 0xB0:
            switch (incomingByte) {
              case 0x00: Serial.print("Bank select MSB="); break;
              case 0x01: Serial.print("Modulation Wheel MSB="); break;
              case 0x02: Serial.print("Breath controller MSB="); break;
              case 0x04: Serial.print("Foot controller MSB="); break;
              case 0x05: Serial.print("Portamento time MSB="); break;
              case 0x06: Serial.print("Data entry MSB="); break;
              case 0x07: Serial.print("Channel volume MSB="); break;
              case 0x08: Serial.print("Balance MSB="); break;
              case 0x0A: Serial.print("Pan MSB="); break;
              case 0x0B: Serial.print("Expression controller MSB="); break;
              case 0x0C: Serial.print("Effect control 1 MSB="); break;
              case 0x0D: Serial.print("Effect control 2 MSB="); break;
              case 0x10: Serial.print("General purpose controller 1 MSB="); break;
              case 0x11: Serial.print("General purpose controller 2 MSB="); break;
              case 0x12: Serial.print("General purpose controller 3 MSB="); break;
              case 0x13: Serial.print("General purpose controller 4 MSB="); break;
              case 0x20: Serial.print("Bank select LSB="); break;
              case 0x21: Serial.print("Modulation Wheel LSB="); break;
              case 0x22: Serial.print("Breath controller LSB="); break;
              case 0x24: Serial.print("Foot controller LSB="); break;
              case 0x25: Serial.print("Portamento time LSB="); break;
              case 0x26: Serial.print("Data entry LSB="); break;
              case 0x27: Serial.print("Channel volume LSB="); break;
              case 0x28: Serial.print("Balance LSB="); break;
              case 0x2A: Serial.print("Pan LSB="); break;
              case 0x2B: Serial.print("Expression controller LSB="); break;
              case 0x2C: Serial.print("Effect control 1 LSB="); break;
              case 0x2D: Serial.print("Effect control 2 LSB="); break;
              case 0x30: Serial.print("General purpose controller 1 LSB="); break;
              case 0x31: Serial.print("General purpose controller 2 LSB="); break;
              case 0x32: Serial.print("General purpose controller 3 LSB="); break;
              case 0x33: Serial.print("General purpose controller 4 LSB="); break;
              case 0x40: Serial.print("Damper pedal on/off "); break;
              case 0x41: Serial.print("Portamento on/off "); break;
              case 0x42: Serial.print("Sustenuto on/off "); break;
              case 0x43: Serial.print("Soft pedal on/off "); break;
              case 0x44: Serial.print("Legato footswitch on/off "); break;
              case 0x45: Serial.print("Hold 2 on/off "); break;
              case 0x46: Serial.print("Sound controller 1 "); break;
              case 0x47: Serial.print("Sound controller 2 "); break;
              case 0x48: Serial.print("Sound controller 3 "); break;
              case 0x49: Serial.print("Sound controller 4 "); break;
              case 0x4A: Serial.print("Sound controller 5 "); break;
              case 0x4B: Serial.print("Sound controller 6 "); break;
              case 0x4C: Serial.print("Sound controller 7 "); break;
              case 0x4D: Serial.print("Sound controller 8 "); break;
              case 0x4E: Serial.print("Sound controller 9 "); break;
              case 0x4F: Serial.print("Sound controller 10 "); break;
              case 0x50: Serial.print("General purpose controller 5 "); break;
              case 0x51: Serial.print("General purpose controller 6 "); break;
              case 0x52: Serial.print("General purpose controller 7 "); break;
              case 0x53: Serial.print("General purpose controller 8 "); break;
              case 0x54: Serial.print("Portamento control "); break;
              case 0x5B: Serial.print("Effect 1 depth "); break;
              case 0x5C: Serial.print("Effect 2 depth "); break;
              case 0x5D: Serial.print("Effect 3 depth "); break;
              case 0x5E: Serial.print("Effect 4 depth "); break;
              case 0x60: Serial.print("Data increment "); break;
              case 0x61: Serial.print("Data decrement "); break;
              case 0x62: Serial.print("NRPN LSB="); break;
              case 0x63: Serial.print("NRPN MSB="); break;
              case 0x64: Serial.print("RPN LSB="); break;
              case 0x65: Serial.print("RPN MSB="); break;
              case 0x78: Serial.print("All sounds off "); break;
              case 0x79: Serial.print("Reset all controller "); break;
              case 0x7A: Serial.print("Local control on/off "); break;
              case 0x7B: Serial.print("All notes off "); break;
              case 0x7C: Serial.print("Omni mode off "); break;
              case 0x7D: Serial.print("Omni mode on "); break;
              case 0x7E: Serial.print("Poly mode off "); break;
              case 0x7F: Serial.print("Poly mode on "); break;
              default: Serial.print("UNDEFINED ("); Serial.print("0x"); Serial.print(incomingByte, HEX); Serial.print(")="); break;
            }
          break;
          default:
            Serial.print("0x"); Serial.print(incomingByte, HEX); Serial.print(" ");
          break;
        }
      break;
    }
    prev = incomingByte;
  }
  if (Serial2.available() > 0) {
    int incomingByte = Serial2.read();
    if (incomingByte > 0x7F) Serial.println("");
    Serial.print(incomingByte, HEX); Serial.print("h2 ");
  }
}

