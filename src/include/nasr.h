#include <glad/glad.h>
#include "GLFW/glfw3.h"

typedef struct NasrColor
{
    float r;
    float g;
    float b;
    float a;
} NasrColor;

void NasrColorPrint( const struct NasrColor * c );

typedef struct NasrRect
{
    float x;
    float y;
    float w;
    float h;
} NasrRect;

float NasrRectRight( const struct NasrRect * r );
float NasrRectBottom( const struct NasrRect * r );
int NasrRectEqual( const struct NasrRect * a, const struct NasrRect * b );
void NasrRectPrint( const struct NasrRect * r );

#define NASR_SHADER_VERTEX   0
#define NASR_SHADER_FRAGMENT 1

typedef struct NasrShader
{
    int type;
    const char * code;
} NasrShader;

#define NASR_GRAPHIC_RECT          0
#define NASR_GRAPHIC_RECT_GRADIENT 1

typedef struct NasrGraphicRect
{
    NasrRect rect;
    NasrColor color;
} NasrGraphicRect;

typedef struct NasrGraphicRectGradient
{
    NasrRect rect;
    NasrColor color1;
    NasrColor color2;
    NasrColor color3;
    NasrColor color4;
} NasrGraphicRectGradient;

typedef union NasrGraphicData
{
    NasrGraphicRect         rect;
    NasrGraphicRectGradient gradient;
} NasrGraphicData;

typedef struct NasrGraphic
{
    int type;
    NasrGraphicData data;
} NasrGraphic;

#define NASR_KEY_UNKNOWN		GLFW_KEY_UNKNOWN
#define NASR_KEY_SPACE			GLFW_KEY_SPACE
#define NASR_KEY_APOSTROPHE		GLFW_KEY_APOSTROPHE
#define NASR_KEY_COMMA			GLFW_KEY_COMMA
#define NASR_KEY_MINUS			GLFW_KEY_MINUS
#define NASR_KEY_PERIOD			GLFW_KEY_PERIOD
#define NASR_KEY_SLASH			GLFW_KEY_SLASH
#define NASR_KEY_0				GLFW_KEY_0
#define NASR_KEY_1				GLFW_KEY_1
#define NASR_KEY_2				GLFW_KEY_2
#define NASR_KEY_3				GLFW_KEY_3
#define NASR_KEY_4				GLFW_KEY_4
#define NASR_KEY_5				GLFW_KEY_5
#define NASR_KEY_6				GLFW_KEY_6
#define NASR_KEY_7				GLFW_KEY_7
#define NASR_KEY_8				GLFW_KEY_8
#define NASR_KEY_9				GLFW_KEY_9
#define NASR_KEY_SEMICOLON		GLFW_KEY_SEMICOLON
#define NASR_KEY_EQUAL			GLFW_KEY_EQUAL
#define NASR_KEY_A				GLFW_KEY_A
#define NASR_KEY_B				GLFW_KEY_B
#define NASR_KEY_C				GLFW_KEY_C
#define NASR_KEY_D				GLFW_KEY_D
#define NASR_KEY_E				GLFW_KEY_E
#define NASR_KEY_F				GLFW_KEY_F
#define NASR_KEY_G				GLFW_KEY_G
#define NASR_KEY_H				GLFW_KEY_H
#define NASR_KEY_I				GLFW_KEY_I
#define NASR_KEY_J				GLFW_KEY_J
#define NASR_KEY_K				GLFW_KEY_K
#define NASR_KEY_L				GLFW_KEY_L
#define NASR_KEY_M				GLFW_KEY_M
#define NASR_KEY_N				GLFW_KEY_N
#define NASR_KEY_O				GLFW_KEY_O
#define NASR_KEY_P				GLFW_KEY_P
#define NASR_KEY_Q				GLFW_KEY_Q
#define NASR_KEY_R				GLFW_KEY_R
#define NASR_KEY_S				GLFW_KEY_S
#define NASR_KEY_T				GLFW_KEY_T
#define NASR_KEY_U				GLFW_KEY_U
#define NASR_KEY_V				GLFW_KEY_V
#define NASR_KEY_W				GLFW_KEY_W
#define NASR_KEY_X				GLFW_KEY_X
#define NASR_KEY_Y				GLFW_KEY_Y
#define NASR_KEY_Z				GLFW_KEY_Z
#define NASR_KEY_LEFT_BRACKET	GLFW_KEY_LEFT_BRACKET
#define NASR_KEY_BACKSLASH		GLFW_KEY_BACKSLASH
#define NASR_KEY_RIGHT_BRACKET	GLFW_KEY_RIGHT_BRACKET
#define NASR_KEY_GRAVE_ACCENT	GLFW_KEY_GRAVE_ACCENT
#define NASR_KEY_WORLD_1		GLFW_KEY_WORLD_1
#define NASR_KEY_WORLD_2		GLFW_KEY_WORLD_2
#define NASR_KEY_ESCAPE			GLFW_KEY_ESCAPE
#define NASR_KEY_ENTER			GLFW_KEY_ENTER
#define NASR_KEY_TAB			GLFW_KEY_TAB
#define NASR_KEY_BACKSPACE		GLFW_KEY_BACKSPACE
#define NASR_KEY_INSERT			GLFW_KEY_INSERT
#define NASR_KEY_DELETE			GLFW_KEY_DELETE
#define NASR_KEY_RIGHT			GLFW_KEY_RIGHT
#define NASR_KEY_LEFT			GLFW_KEY_LEFT
#define NASR_KEY_DOWN			GLFW_KEY_DOWN
#define NASR_KEY_UP				GLFW_KEY_UP
#define NASR_KEY_PAGE_UP		GLFW_KEY_PAGE_UP
#define NASR_KEY_PAGE_DOWN		GLFW_KEY_PAGE_DOWN
#define NASR_KEY_HOME			GLFW_KEY_HOME
#define NASR_KEY_END			GLFW_KEY_END
#define NASR_KEY_CAPS_LOCK		GLFW_KEY_CAPS_LOCK
#define NASR_KEY_SCROLL_LOCK	GLFW_KEY_SCROLL_LOCK
#define NASR_KEY_NUM_LOCK		GLFW_KEY_NUM_LOCK
#define NASR_KEY_PRINT_SCREEN	GLFW_KEY_PRINT_SCREEN
#define NASR_KEY_PAUSE			GLFW_KEY_PAUSE
#define NASR_KEY_F1				GLFW_KEY_F1
#define NASR_KEY_F2			    GLFW_KEY_F2
#define NASR_KEY_F3			    GLFW_KEY_F3
#define NASR_KEY_F4			    GLFW_KEY_F4
#define NASR_KEY_F5			    GLFW_KEY_F5
#define NASR_KEY_F6			    GLFW_KEY_F6
#define NASR_KEY_F7			    GLFW_KEY_F7
#define NASR_KEY_F8			    GLFW_KEY_F8
#define NASR_KEY_F9				GLFW_KEY_F9
#define NASR_KEY_F10			GLFW_KEY_F10
#define NASR_KEY_F11			GLFW_KEY_F11
#define NASR_KEY_F12			GLFW_KEY_F12
#define NASR_KEY_F13			GLFW_KEY_F13
#define NASR_KEY_F14			GLFW_KEY_F14
#define NASR_KEY_F15			GLFW_KEY_F15
#define NASR_KEY_F16			GLFW_KEY_F16
#define NASR_KEY_F17			GLFW_KEY_F17
#define NASR_KEY_F18			GLFW_KEY_F18
#define NASR_KEY_F19			GLFW_KEY_F19
#define NASR_KEY_F20			GLFW_KEY_F20
#define NASR_KEY_F21			GLFW_KEY_F21
#define NASR_KEY_F22			GLFW_KEY_F22
#define NASR_KEY_F23			GLFW_KEY_F23
#define NASR_KEY_F24			GLFW_KEY_F24
#define NASR_KEY_F25			GLFW_KEY_F25
#define NASR_KEY_KP_0			GLFW_KEY_KP_0
#define NASR_KEY_KP_1			GLFW_KEY_KP_1
#define NASR_KEY_KP_2			GLFW_KEY_KP_2
#define NASR_KEY_KP_3			GLFW_KEY_KP_3
#define NASR_KEY_KP_4			GLFW_KEY_KP_4
#define NASR_KEY_KP_5			GLFW_KEY_KP_5
#define NASR_KEY_KP_6			GLFW_KEY_KP_6
#define NASR_KEY_KP_7			GLFW_KEY_KP_7
#define NASR_KEY_KP_8			GLFW_KEY_KP_8
#define NASR_KEY_KP_9			GLFW_KEY_KP_9
#define NASR_KEY_KP_DECIMAL		GLFW_KEY_KP_DECIMAL
#define NASR_KEY_KP_DIVIDE		GLFW_KEY_KP_DIVIDE
#define NASR_KEY_KP_MULTIPLY	GLFW_KEY_KP_MULTIPLY
#define NASR_KEY_KP_SUBTRACT	GLFW_KEY_KP_SUBTRACT
#define NASR_KEY_KP_ADD			GLFW_KEY_KP_ADD
#define NASR_KEY_KP_ENTER		GLFW_KEY_KP_ENTER
#define NASR_KEY_KP_EQUAL		GLFW_KEY_KP_EQUAL
#define NASR_KEY_LEFT_SHIFT		GLFW_KEY_LEFT_SHIFT
#define NASR_KEY_LEFT_CONTROL	GLFW_KEY_LEFT_CONTROL
#define NASR_KEY_LEFT_ALT		GLFW_KEY_LEFT_ALT
#define NASR_KEY_LEFT_SUPER		GLFW_KEY_LEFT_SUPER
#define NASR_KEY_RIGHT_SHIFT	GLFW_KEY_RIGHT_SHIFT
#define NASR_KEY_RIGHT_CONTROL	GLFW_KEY_RIGHT_CONTROL
#define NASR_KEY_RIGHT_ALT		GLFW_KEY_RIGHT_ALT
#define NASR_KEY_RIGHT_SUPER	GLFW_KEY_RIGHT_SUPER
#define NASR_KEY_MENU			GLFW_KEY_MENU
#define NASR_KEY_LAST			GLFW_KEY_LAST

typedef struct NasrInput
{
    int id;
    int key;
} NasrInput;

#define NASR_DIR_UP        0
#define NASR_DIR_UPRIGHT   1
#define NASR_DIR_RIGHT     2
#define NASR_DIR_DOWNRIGHT 3
#define NASR_DIR_DOWN      4
#define NASR_DIR_DOWNLEFT  5
#define NASR_DIR_LEFT      6
#define NASR_DIR_UPLEFT    7

int NasrHeld( int id );
void NasrRegisterInputs( const NasrInput * inputs, int num_o_inputs );

int NasrInit( const char * program_title, float canvas_width, float canvas_height, int max_graphics );
void NasrClose( void );
void NasrUpdate( void );

int NasrHasClosed( void );
void NasrLog( const char * message );

void NasrResetCamera( void );
void NasrAdjustCamera( struct NasrRect * target, float max_w, float max_h );
void NasrMoveCamera( float x, float y, float max_w, float max_h );

NasrGraphic * NasrGraphicGet( int id );
int NasrGraphicsAddCanvas( struct NasrColor color );
int NasrGraphicsAddRect(
    struct NasrRect rect,
    struct NasrColor color
);
int NasrGraphicsAddRectGradient(
    struct NasrRect rect,
    int dir,
    struct NasrColor color1,
    struct NasrColor color2
);