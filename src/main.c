#include "nasr.h"

static int running = 1;

int main( int argc, char ** argv )
{
    NasrInit( "Nasringine 0.1", 520, 320, 1024 );
    const NasrColor canvas_color = { 255.0f, 255.0f, 255.0f, 255.0f };
    NasrGraphicsAddCanvas( canvas_color );
    const NasrRect rect = { 64.0f, 64.0f, 32.0f, 32.0f };
    const NasrColor color = { 255.0f, 0.0f, 0.0f, 255.0f };
    NasrGraphicsAddRect(
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
            NasrMoveCamera( 1.0f, 1.0f, 600.0f, 400.0f );
            NasrUpdate();
        }
    }
    NasrClose();
    return 0;
}