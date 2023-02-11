#ifndef NASR_INPUT_H
#define NASR_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#define NASR_KEY_UNKNOWN 0
#define NASR_KEY_SPACE 1
#define NASR_KEY_APOSTROPHE 2
#define NASR_KEY_COMMA 3
#define NASR_KEY_MINUS 4
#define NASR_KEY_PERIOD 5
#define NASR_KEY_SLASH 6
#define NASR_KEY_0 7
#define NASR_KEY_1 8
#define NASR_KEY_2 9
#define NASR_KEY_3 10
#define NASR_KEY_4 11
#define NASR_KEY_5 12
#define NASR_KEY_6 13
#define NASR_KEY_7 14
#define NASR_KEY_8 15
#define NASR_KEY_9 16
#define NASR_KEY_SEMICOLON 17
#define NASR_KEY_EQUAL 18
#define NASR_KEY_A 19
#define NASR_KEY_B 20
#define NASR_KEY_C 21
#define NASR_KEY_D 22
#define NASR_KEY_E 23
#define NASR_KEY_F 24
#define NASR_KEY_G 25
#define NASR_KEY_H 26
#define NASR_KEY_I 27
#define NASR_KEY_J 28
#define NASR_KEY_K 29
#define NASR_KEY_L 30
#define NASR_KEY_M 31
#define NASR_KEY_N 32
#define NASR_KEY_O 33
#define NASR_KEY_P 34
#define NASR_KEY_Q 35
#define NASR_KEY_R 36
#define NASR_KEY_S 37
#define NASR_KEY_T 38
#define NASR_KEY_U 39
#define NASR_KEY_V 40
#define NASR_KEY_W 41
#define NASR_KEY_X 42
#define NASR_KEY_Y 43
#define NASR_KEY_Z 44
#define NASR_KEY_LEFT_BRACKET 45
#define NASR_KEY_BACKSLASH 46
#define NASR_KEY_RIGHT_BRACKET 47
#define NASR_KEY_GRAVE_ACCENT 48
#define NASR_KEY_WORLD_1 49
#define NASR_KEY_WORLD_2 50
#define NASR_KEY_ESCAPE 51
#define NASR_KEY_ENTER 52
#define NASR_KEY_TAB 53
#define NASR_KEY_BACKSPACE 54
#define NASR_KEY_INSERT 55
#define NASR_KEY_DELETE 56
#define NASR_KEY_RIGHT 57
#define NASR_KEY_LEFT 58
#define NASR_KEY_DOWN 59
#define NASR_KEY_UP 60
#define NASR_KEY_PAGE_UP 61
#define NASR_KEY_PAGE_DOWN 62
#define NASR_KEY_HOME 63
#define NASR_KEY_END 64
#define NASR_KEY_CAPS_LOCK 65
#define NASR_KEY_SCROLL_LOCK 66
#define NASR_KEY_NUM_LOCK 67
#define NASR_KEY_PRINT_SCREEN 68
#define NASR_KEY_PAUSE 69
#define NASR_KEY_F1 70
#define NASR_KEY_F2 71
#define NASR_KEY_F3 72
#define NASR_KEY_F4 73
#define NASR_KEY_F5 74
#define NASR_KEY_F6 75
#define NASR_KEY_F7 76
#define NASR_KEY_F8 77
#define NASR_KEY_F9 78
#define NASR_KEY_F10 79
#define NASR_KEY_F11 80
#define NASR_KEY_F12 81
#define NASR_KEY_F13 82
#define NASR_KEY_F14 83
#define NASR_KEY_F15 84
#define NASR_KEY_F16 85
#define NASR_KEY_F17 86
#define NASR_KEY_F18 87
#define NASR_KEY_F19 88
#define NASR_KEY_F20 89
#define NASR_KEY_F21 90
#define NASR_KEY_F22 91
#define NASR_KEY_F23 92
#define NASR_KEY_F24 93
#define NASR_KEY_F25 94
#define NASR_KEY_KP_0 95
#define NASR_KEY_KP_1 96
#define NASR_KEY_KP_2 97
#define NASR_KEY_KP_3 98
#define NASR_KEY_KP_4 99

typedef struct NasrInput
{
    unsigned int id;
    unsigned int key;
} NasrInput;

void NasrInputUpdate( void );
int NasrHeld( int id );
int NasrPressed( int id );
void NasrRegisterInputs( const NasrInput * inputs, int num_o_inputs );
void NasrInputClose( void );

#ifdef __cplusplus
}
#endif

#endif // NASR_INPUT_H