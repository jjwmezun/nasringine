#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include "nasr.h"
#include "nasr_input.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct KeyInputList {
    int count;
    int * data;
} KeyInputList;



// Static Data
static uint_fast8_t init;
static int max_inputs;
static int inputs_per_key;
static int keys_per_input;
static int key_input_size;
static int input_key_size;
static int input_keys_start;
static int pressed_keys_start;
static int held_keys_start;
static int pressed_start;
static int pressed_size;
static int held_start;
static int * keydata;
static int nasr_to_glfw_map[] =
{
	GLFW_KEY_UNKNOWN, // NASR_KEY_UNKNOWN
	GLFW_KEY_SPACE, // NASR_KEY_SPACE
	GLFW_KEY_APOSTROPHE, // NASR_KEY_APOSTROPHE
	GLFW_KEY_COMMA, // NASR_KEY_COMMA
	GLFW_KEY_MINUS, // NASR_KEY_MINUS
	GLFW_KEY_PERIOD, // NASR_KEY_PERIOD
	GLFW_KEY_SLASH, // NASR_KEY_SLASH
	GLFW_KEY_0, // NASR_KEY_0
	GLFW_KEY_1, // NASR_KEY_1
	GLFW_KEY_2, // NASR_KEY_2
	GLFW_KEY_3, // NASR_KEY_3
	GLFW_KEY_4, // NASR_KEY_4
	GLFW_KEY_5, // NASR_KEY_5
	GLFW_KEY_6, // NASR_KEY_6
	GLFW_KEY_7, // NASR_KEY_7
	GLFW_KEY_8, // NASR_KEY_8
	GLFW_KEY_9, // NASR_KEY_9
	GLFW_KEY_SEMICOLON, // NASR_KEY_SEMICOLON
	GLFW_KEY_EQUAL, // NASR_KEY_EQUAL
	GLFW_KEY_A, // NASR_KEY_A
	GLFW_KEY_B, // NASR_KEY_B
	GLFW_KEY_C, // NASR_KEY_C
	GLFW_KEY_D, // NASR_KEY_D
	GLFW_KEY_E, // NASR_KEY_E
	GLFW_KEY_F, // NASR_KEY_F
	GLFW_KEY_G, // NASR_KEY_G
	GLFW_KEY_H, // NASR_KEY_H
	GLFW_KEY_I, // NASR_KEY_I
	GLFW_KEY_J, // NASR_KEY_J
	GLFW_KEY_K, // NASR_KEY_K
	GLFW_KEY_L, // NASR_KEY_L
	GLFW_KEY_M, // NASR_KEY_M
	GLFW_KEY_N, // NASR_KEY_N
	GLFW_KEY_O, // NASR_KEY_O
	GLFW_KEY_P, // NASR_KEY_P
	GLFW_KEY_Q, // NASR_KEY_Q
	GLFW_KEY_R, // NASR_KEY_R
	GLFW_KEY_S, // NASR_KEY_S
	GLFW_KEY_T, // NASR_KEY_T
	GLFW_KEY_U, // NASR_KEY_U
	GLFW_KEY_V, // NASR_KEY_V
	GLFW_KEY_W, // NASR_KEY_W
	GLFW_KEY_X, // NASR_KEY_X
	GLFW_KEY_Y, // NASR_KEY_Y
	GLFW_KEY_Z, // NASR_KEY_Z
	GLFW_KEY_LEFT_BRACKET, // NASR_KEY_LEFT_BRACKET
	GLFW_KEY_BACKSLASH, // NASR_KEY_BACKSLASH
	GLFW_KEY_RIGHT_BRACKET, // NASR_KEY_RIGHT_BRACKET
	GLFW_KEY_GRAVE_ACCENT, // NASR_KEY_GRAVE_ACCENT
	GLFW_KEY_WORLD_1, // NASR_KEY_WORLD_1
	GLFW_KEY_WORLD_2, // NASR_KEY_WORLD_2
	GLFW_KEY_ESCAPE, // NASR_KEY_ESCAPE
	GLFW_KEY_ENTER, // NASR_KEY_ENTER
	GLFW_KEY_TAB, // NASR_KEY_TAB
	GLFW_KEY_BACKSPACE, // NASR_KEY_BACKSPACE
	GLFW_KEY_INSERT, // NASR_KEY_INSERT
	GLFW_KEY_DELETE, // NASR_KEY_DELETE
	GLFW_KEY_RIGHT, // NASR_KEY_RIGHT
	GLFW_KEY_LEFT, // NASR_KEY_LEFT
	GLFW_KEY_DOWN, // NASR_KEY_DOWN
	GLFW_KEY_UP, // NASR_KEY_UP
	GLFW_KEY_PAGE_UP, // NASR_KEY_PAGE_UP
	GLFW_KEY_PAGE_DOWN, // NASR_KEY_PAGE_DOWN
	GLFW_KEY_HOME, // NASR_KEY_HOME
	GLFW_KEY_END, // NASR_KEY_END
	GLFW_KEY_CAPS_LOCK, // NASR_KEY_CAPS_LOCK
	GLFW_KEY_SCROLL_LOCK, // NASR_KEY_SCROLL_LOCK
	GLFW_KEY_NUM_LOCK, // NASR_KEY_NUM_LOCK
	GLFW_KEY_PRINT_SCREEN, // NASR_KEY_PRINT_SCREEN
	GLFW_KEY_PAUSE, // NASR_KEY_PAUSE
	GLFW_KEY_F1, // NASR_KEY_F1
	GLFW_KEY_F2, // NASR_KEY_F2
	GLFW_KEY_F3, // NASR_KEY_F3
	GLFW_KEY_F4, // NASR_KEY_F4
	GLFW_KEY_F5, // NASR_KEY_F5
	GLFW_KEY_F6, // NASR_KEY_F6
	GLFW_KEY_F7, // NASR_KEY_F7
	GLFW_KEY_F8, // NASR_KEY_F8
	GLFW_KEY_F9, // NASR_KEY_F9
	GLFW_KEY_F10, // NASR_KEY_F10
	GLFW_KEY_F11, // NASR_KEY_F11
	GLFW_KEY_F12, // NASR_KEY_F12
	GLFW_KEY_F13, // NASR_KEY_F13
	GLFW_KEY_F14, // NASR_KEY_F14
	GLFW_KEY_F15, // NASR_KEY_F15
	GLFW_KEY_F16, // NASR_KEY_F16
	GLFW_KEY_F17, // NASR_KEY_F17
	GLFW_KEY_F18, // NASR_KEY_F18
	GLFW_KEY_F19, // NASR_KEY_F19
	GLFW_KEY_F20, // NASR_KEY_F20
	GLFW_KEY_F21, // NASR_KEY_F21
	GLFW_KEY_F22, // NASR_KEY_F22
	GLFW_KEY_F23, // NASR_KEY_F23
	GLFW_KEY_F24, // NASR_KEY_F24
	GLFW_KEY_F25, // NASR_KEY_F25
	GLFW_KEY_KP_0, // NASR_KEY_KP_0
	GLFW_KEY_KP_1, // NASR_KEY_KP_1
	GLFW_KEY_KP_2, // NASR_KEY_KP_2
	GLFW_KEY_KP_3, // NASR_KEY_KP_3
	GLFW_KEY_KP_4, // NASR_KEY_KP_4
	GLFW_KEY_KP_5, // NASR_KEY_KP_5
	GLFW_KEY_KP_6, // NASR_KEY_KP_6
	GLFW_KEY_KP_7, // NASR_KEY_KP_7
	GLFW_KEY_KP_8, // NASR_KEY_KP_8
	GLFW_KEY_KP_9, // NASR_KEY_KP_9
	GLFW_KEY_KP_DECIMAL, // NASR_KEY_KP_DECIMAL
	GLFW_KEY_KP_DIVIDE, // NASR_KEY_KP_DIVIDE
	GLFW_KEY_KP_MULTIPLY, // NASR_KEY_KP_MULTIPLY
	GLFW_KEY_KP_SUBTRACT, // NASR_KEY_KP_SUBTRACT
	GLFW_KEY_KP_ADD, // NASR_KEY_KP_ADD
	GLFW_KEY_KP_ENTER, // NASR_KEY_KP_ENTER
	GLFW_KEY_KP_EQUAL, // NASR_KEY_KP_EQUAL
	GLFW_KEY_LEFT_SHIFT, // NASR_KEY_LEFT_SHIFT
	GLFW_KEY_LEFT_CONTROL, // NASR_KEY_LEFT_CONTROL
	GLFW_KEY_LEFT_ALT, // NASR_KEY_LEFT_ALT
	GLFW_KEY_LEFT_SUPER, // NASR_KEY_LEFT_SUPER
	GLFW_KEY_RIGHT_SHIFT, // NASR_KEY_RIGHT_SHIFT
	GLFW_KEY_RIGHT_CONTROL, // NASR_KEY_RIGHT_CONTROL
	GLFW_KEY_RIGHT_ALT, // NASR_KEY_RIGHT_ALT
	GLFW_KEY_RIGHT_SUPER, // NASR_KEY_RIGHT_SUPER
	GLFW_KEY_MENU, // NASR_KEY_MENU
	GLFW_KEY_LAST  // NASR_KEY_LAST
};



// Static Functions
static int * GetHeld( int id );
static int * GetPressed( int id );
static int * GetHeldKeys( int input );
static int * GetPressedKeys( int input );
static int * GetInputKeys( int input );
static int * GetKeyInputs( int key );
static void HandleInput( void * window, int key, int scancode, int action, int mods );



// Public Functions
void NasrInputUpdate( void )
{
    memset( &keydata[ pressed_keys_start ], 0, pressed_size );
};

int NasrHeld( int id )
{
    if ( id >= max_inputs )
    {
        NasrLog( "NasrHeld Error: invalid id #%d.", id );
        return 0;
    }
    return GetHeld( id )[ 0 ];
};

int NasrPressed( int id )
{
    if ( id >= max_inputs )
    {
        NasrLog( "NasrPressed Error: invalid id #%d.", id );
        return 0;
    }
    return GetPressed( id )[ 0 ];
};

void NasrRegisterInputs( const NasrInput * inputs, int num_o_inputs )
{
    // Init Input
    if ( !init )
    {
        NasrRegisterInputHandler( HandleInput );
        init = 1;
    }

    // If this is not the 1st time using this function, make sure we reset e’erything to prevent memory leaks &
    // inaccurate #s.
    if ( keydata )
    {
        free( keydata );
    }
    max_inputs = inputs_per_key = keys_per_input = pressed_size = 0;

    // These several loops are necessary to find the max inputs per key & max keys per input.
    int keys_max_inputs[ GLFW_KEY_LAST ] = { 0 };
    const int mapsize = sizeof( nasr_to_glfw_map ) / sizeof( int );
    for ( int i = 0; i < num_o_inputs; ++i )
    {
        if ( inputs[ i ].key >= mapsize )
        {
            NasrLog( "NasrRegisterInputs Error: Input #%d has invalid key “%d”.", i, inputs[ i ].key );
            max_inputs = 0;
            return;
        }
        if ( inputs[ i ].id + 1 > max_inputs )
        {
            max_inputs = inputs[ i ].id + 1;
        }
        ++keys_max_inputs[ nasr_to_glfw_map[ inputs[ i ].key ] ];
    }
    int * inputs_max_keys = calloc( max_inputs, sizeof( int ) );
    for ( int i = 0; i < GLFW_KEY_LAST; ++i )
    {
        if ( keys_max_inputs[ i ] > inputs_per_key )
        {
            inputs_per_key = keys_max_inputs[ i ];
        }
    }
    for ( int i = 0; i < num_o_inputs; ++i )
    {
        ++inputs_max_keys[ inputs[ i ].id ];
    }
    for ( int i = 0; i < max_inputs; ++i )
    {
        if ( inputs_max_keys[ i ] > keys_per_input )
        {
            keys_per_input = inputs_max_keys[ i ];
        }
    }
    free( inputs_max_keys );

    // Calculate size o’ chunks.
    // 4 chunks are:
    // * List o’ inputs for each key ( list o’ lists )
    //       -> MAX_KEYS * ( 1 int for inner list count + inner list length )
    // * List o’ keys for each input ( list o’ lists )
    //       -> MAX_INPUTS * ( 1 int for inner list count + inner list length )
    // * List o’ keys held for each input ( list o’ lists )
    //       -> MAX_INPUTS * inner list length.
    //          We don’t need counts, as these are only e’er referenced from inputs & ne’er looped thru.
    // * List o’ keys pressed for each input ( list o’ lists )
    //       -> MAX_INPUTS * inner list length.
    // * List o’ inputs pressed ( 1D list )
    //       -> MAX_INPUTS
    //          This is needed for inputs pressed, regardless o’ efficiency, as we need to test if that particular
    //          input has already been pressed before.
    // * List o’ inputs held ( 1D list )
    //       -> MAX_INPUTS
    //          Tho this value can be found thru the previous held_keys list, it’s mo’ efficient to only loop thru &
    //          check all keys held during press or release & set 1 simple value to be checked during the main game
    //          loop, 60 times per second.
    key_input_size = 1 + inputs_per_key;
    const int key_inputs_size = key_input_size * GLFW_KEY_LAST;
    input_keys_start = key_inputs_size;
    input_key_size = 1 + keys_per_input;
    const int input_keys_size = input_key_size * max_inputs;
    held_keys_start = input_keys_start + input_keys_size;
    const int pressed_keys_count = keys_per_input * max_inputs;
    pressed_keys_start = held_keys_start + pressed_keys_count;
    pressed_start = pressed_keys_start + pressed_keys_count;
    const int pressed_count = max_inputs;
    pressed_size = ( pressed_keys_count + pressed_count ) * sizeof( int );
    held_start = pressed_start + pressed_count;
    const int total_size = key_inputs_size + input_keys_size + input_keys_size + pressed_keys_count + pressed_keys_count + pressed_count;
    keydata = calloc( total_size, sizeof( int ) );

    // Loop thru given list o’ key/input pairs & set KeyInputs & InputKeys,
    // incrementing their counts as we go.
    for ( int i = 0; i < num_o_inputs; ++i )
    {
        int * keyinputs = GetKeyInputs( nasr_to_glfw_map[ inputs[ i ].key ] );
        int * keycount = &keyinputs[ 0 ];
        keyinputs[ 1 + keycount[ 0 ] ] = inputs[ i ].id;
        ++*keycount;

        int * inputkeys = GetInputKeys( inputs[ i ].id );
        int * inputcount = &inputkeys[ 0 ];
        inputkeys[ 1 + inputcount[ 0 ] ] = nasr_to_glfw_map[ inputs[ i ].key ];
        ++*inputcount;
    }
};

void NasrInputClose( void )
{
    free( keydata );
};



// Static Functions
static int * GetHeld( int id )
{
    return &keydata[ held_start + id ];
};

static int * GetPressed( int id )
{
    return &keydata[ pressed_start + id ];
};

static int * GetHeldKeys( int input )
{
    return &keydata[ held_keys_start + ( keys_per_input * input ) ];
};

static int * GetPressedKeys( int input )
{
    return &keydata[ pressed_keys_start + ( keys_per_input * input ) ];
};

static int * GetInputKeys( int input )
{
    return &keydata[ input_keys_start + ( input_key_size * input ) ];
};

static int * GetKeyInputs( int key )
{
    return &keydata[ key_input_size * key ];
};

static void HandleInput( void * window, int key, int scancode, int action, int mods )
{
    if ( !keydata )
    {
        NasrLog( "HandleInput Error: key data not initialized." );
        return;
    }
    switch ( action )
    {
        case ( GLFW_PRESS ):
        {
            // Loop thru this key’s inputs so we can set them as held.
            int * inputs = GetKeyInputs( key );
            const int count = inputs[ 0 ];
            for ( int i = 0; i < count; ++i )
            {
                const int input = inputs[ 1 + i ];
                GetHeld( input )[ 0 ] = 1;
                GetPressed( input )[ 0 ] = 1;
                const int * input_keys = GetInputKeys( input );
                const int input_keys_count = input_keys[ 0 ];
                // While we can settle for just setting the 1D held list to 1, since if any key for an input is
                // pressed, the input is considered pressed, we need to also register the specific key held in
                // a list o’ keys for the input so we can check it later for release.
                for ( int j = 0; j < input_keys_count; ++j )
                {
                    if ( input_keys[ j + 1 ] == key )
                    {
                        GetPressedKeys( input )[ j ] = 1;
                        GetHeldKeys( input )[ j ] = 1;
                    }
                }
            }
        }
        break;
        case ( GLFW_RELEASE ):
        {
            int * inputs = GetKeyInputs( key );
            const int count = inputs[ 0 ];
            for ( int i = 0; i < count; ++i )
            {
                const int input = inputs[ 1 + i ];
                // Unlike the press code, we need this for later, so keep pointer.
                int * held = GetHeld( input );
                int * pressed = GetPressed( input );
                *held = *pressed = 0;
                const int * input_keys = GetInputKeys( input );
                const int input_keys_count = input_keys[ 0 ];
                for ( int j = 0; j < input_keys_count; ++j )
                {
                    // Unlike the press code, we need this for later, so keep pointer.
                    int * heldkeys = &GetHeldKeys( input )[ j ];
                    int * pressedkeys = &GetPressedKeys( input )[ j ];
                    if ( input_keys[ j + 1 ] == key )
                    {
                        *heldkeys = *pressedkeys = 0;
                    }
                    // This is a simple & efficient way to say that if any o’ the keys for this input are held, still
                    // consider it held.
                    held[ 0 ] = held[ 0 ] || *heldkeys;
                    pressed[ 0 ] = pressed[ 0 ] || *pressedkeys;
                }
            }
        }
        break;
    }
};