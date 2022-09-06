#include "nasr.h"

static int running = 1;

typedef enum Input {
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_DOWN,
    INPUT_UP
} Input;

int main( int argc, char ** argv )
{
    NasrInit( "Nasringine 0.1", 520, 320, 1024 );

    NasrInput inputs[] =
    {
        { INPUT_RIGHT, NASR_KEY_RIGHT },
        { INPUT_LEFT, NASR_KEY_LEFT },
        { INPUT_DOWN, NASR_KEY_DOWN },
        { INPUT_UP, NASR_KEY_UP },
        { INPUT_DOWN, NASR_KEY_D }
    };
    NasrRegisterInputs( inputs, 5 );

    const NasrColor canvas_color = { 255.0f, 255.0f, 255.0f, 255.0f };
    NasrGraphicsAddCanvas( canvas_color );
    const NasrRect rect = { 64.0f, 64.0f, 32.0f, 32.0f };
    const NasrColor color = { 255.0f, 0.0f, 0.0f, 255.0f };
    const int rectid = NasrGraphicsAddRect(
        rect,
        color
    );
    while ( running )
    {
        if ( NasrHasClosed() )
        {
            running = 0;
        }
        else
        {
            NasrGraphic * rect = NasrGraphicGet( rectid );
            if ( NasrHeld( INPUT_RIGHT ) )
            {
                rect->data.rect.rect.x += 1.0f;
            }
            else if ( NasrHeld( INPUT_LEFT ) )
            {
                rect->data.rect.rect.x -= 1.0f;
            }
            if ( NasrHeld( INPUT_DOWN ) )
            {
                rect->data.rect.rect.y += 1.0f;
            }
            else if ( NasrHeld( INPUT_UP ) )
            {
                rect->data.rect.rect.y -= 1.0f;
            }
            NasrAdjustCamera( &rect->data.rect.rect, 1024.0f, 640.0f );
            NasrUpdate();
        }
    }
    NasrClose();
    return 0;
}