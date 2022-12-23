#include "nasr_audio.h"
#include "AL/al.h"
#include <AL/alut.h>

static char * lastfile = 0;
static float volume = 0.5f;
static float pitch = 1.0f;
static ALuint buffer = 0;
static ALuint source = 0;
static int is_playing = 0;
static int is_mute = 0;

static void TestForErrors( void );

void NasrAudioInit( void )
{
    ALfloat listenerpos[] = { 0.0,0.0,4.0 };
    ALfloat listenervel[] = { 0.0,0.0,0.0 };
    ALfloat listenerori[] = { 0.0,0.0,1.0, 0.0,1.0,0.0 };

    if ( !alutInit( 0, 0 ) )
    {
        TestForErrors();
    }

    alListenerfv( AL_POSITION, listenerpos );
    alListenerfv( AL_VELOCITY, listenervel );
    alListenerfv( AL_ORIENTATION, listenerori );

    alGenBuffers( 1, &buffer );
    alGenSources( 1, &source );
};

void NasrAudioClose( void )
{
    if ( lastfile )
    {
        free( lastfile );
    }
    alDeleteSources( 1, &source );
    alDeleteBuffers( 1, &buffer );
    alutExit();
};

void NasrLoadSong( const char * filename )
{
    if ( !lastfile || strcmp( filename, lastfile ) != 0 )
    {
        alSourceStop( source );

        ALfloat pos[] = { -2.0, 0.0, 0.0 };
        ALfloat velocity[] = { 0.0, 0.0, 0.0 };
        ALsizei size, freq;
        ALenum format;
        ALvoid * data;
        ALboolean al_bool;

        buffer = alutCreateBufferFromFile( filename );

        alSourcef( source, AL_PITCH, pitch );
        alSourcef( source, AL_GAIN, volume );
        alSourcefv( source, AL_POSITION, pos );
        alSourcefv( source, AL_VELOCITY, velocity );
        alSourcei( source, AL_BUFFER, buffer );
        alSourcei( source, AL_LOOPING, AL_TRUE );

        if ( lastfile )
        {
            free( lastfile );
        }

        lastfile = malloc( strlen( filename ) + 1 );
        strcpy( lastfile, filename );
    }
};

void NasrPlaySong( const char * filename )
{
    if ( filename )
    {
        NasrLoadSong( filename );
    }
    alSourcePlay( source );
    is_playing = 1;
};

void NasrStopSong( void )
{
    alSourceStop( source );
    is_playing = 0;
};

void NasrPauseSong( void )
{
    alSourcePause( source );
    is_playing = 0;
};

void NasrToggleSong( void )
{
    if ( is_playing )
    {
        alSourcePause( source );
    }
    else
    {
        alSourcePlay( source );
    }
    is_playing = !is_playing;
};

void NasrVolumeSet( float amount )
{
    volume = amount;
    if ( volume < 0.0f )
    {
        volume = 0.0f;
    }
    else if ( volume > 2.0f )
    {
        volume = 2.0f;
    }
    alSourcef( source, AL_GAIN, volume );
    is_mute = 0;
};

void NasrVolumeIncrease( float amount )
{
    NasrVolumeSet( volume + amount );
};

void NasrVolumeDecrease( float amount )
{
    NasrVolumeSet( volume - amount );
};

void NasrVolumeMute( void )
{
    alSourcef( source, AL_GAIN, 0.0f );
    is_mute = 1;
};

void NasrVolumeUnMute( void )
{
    alSourcef( source, AL_GAIN, volume );
    is_mute = 0;
};

void NasrVolumeToggleMute( void )
{
    if ( is_mute )
    {
        NasrVolumeUnMute();
    }
    else
    {
        NasrVolumeMute();
    }
};

void NasrPitchSet( float amount )
{
    pitch = amount;
    if ( pitch < 0.0f )
    {
        pitch = 0.0f;
    }
    else if ( pitch > 2.0f )
    {
        pitch = 2.0f;
    }
    alSourcef( source, AL_PITCH, pitch );
};

void NasrPitchIncrease( float amount )
{
    NasrPitchSet( pitch + amount );
};

void NasrPitchDecrease( float amount )
{
    NasrPitchSet( pitch - amount );
};


static void TestForErrors( void )
{
    int error = alGetError();
    if( error != AL_NO_ERROR ) 
    {
        printf( "Nasringine Audio Error: %s\n", alutGetErrorString( error ) );
    }
};