#include "AL/al.h"
#include <AL/alut.h>
#include "nasr_audio.h"
#include "nasr_log.h"
#include "nasr_math.h"

typedef struct Song
{
    int id;
    int mute;
    float volume;
    float pitch;
} Song;

typedef struct SongEntry
{
    NasrHashKey key;
    int id;
} SongEntry;

typedef struct SongMap
{
    SongEntry * entries;
    unsigned int capacity;
} SongMap;

static float master_volume = 0.5f;
static ALuint * buffers = 0;
static ALuint * sources = 0;
static Song * songs = 0;
static SongMap songmap = { 0, 0 };
static int maxsongs = 0;
static int current_song = 0;

static void TestForErrors( void );
static SongEntry * NasrFindSongEntry( const char * needle_string, hash_t needle_hash );

int NasrAudioInit( unsigned int max )
{
    maxsongs = NASR_MATH_MIN( 256, max );

    if ( max > 256 )
    {
        NasrLog( "Max too long: Can only load 256 songs at once.\n" );
    }

    ALfloat listenerpos[] = { 0.0,0.0,4.0 };
    ALfloat listenervel[] = { 0.0,0.0,0.0 };
    ALfloat listenerori[] = { 0.0,0.0,1.0, 0.0,1.0,0.0 };

    if ( !alutInit( 0, 0 ) )
    {
        TestForErrors();
        return -1;
    }

    alListenerfv( AL_POSITION, listenerpos );
    alListenerfv( AL_VELOCITY, listenervel );
    alListenerfv( AL_ORIENTATION, listenerori );

    buffers = calloc( maxsongs, sizeof( ALuint ) );
    alGenBuffers( maxsongs, buffers );
    sources = calloc( maxsongs, sizeof( ALuint ) );
    alGenSources( maxsongs, sources );
    TestForErrors();

    songs = malloc( sizeof( Song ) * maxsongs );
    for ( int i = 0; i < maxsongs; ++i )
    {
        songs[ i ].id = -1;
        songs[ i ].mute = 0;
        songs[ i ].volume = master_volume;
        songs[ i ].pitch = 1.0f;
    }
    songmap.capacity = NasrGetNextPrime( maxsongs + 1 );
    songmap.entries = calloc( songmap.capacity, sizeof( SongEntry ) );

    return 0;
};

void NasrAudioClose( void )
{
    if ( songs )
    {
        free( songs );
    }
    if ( songmap.entries )
    {
        free( songmap.entries );
    }
    alDeleteSources( maxsongs, sources );
    if ( sources )
    {
        free( sources );
    }
    alDeleteBuffers( maxsongs, buffers );
    if ( buffers )
    {
        free( buffers );
    }
    alutExit();
};

unsigned int NasrLoadSong( const char * filename )
{
    int id = -1;
    hash_t needle_hash = NasrHashString( filename, songmap.capacity );
    SongEntry * entry = NasrFindSongEntry( filename, needle_hash );
    if ( !entry->key.string )
    {
        entry->key.string = ( char * )( malloc( strlen( filename ) + 1 ) );
        strcpy( entry->key.string, filename );
        entry->key.hash = needle_hash;

        for ( int i = 0; i < maxsongs; ++i )
        {
            if ( songs[ i ].id < 0 )
            {
                id = i;
                break;
            }
        }

        if ( id == -1 )
        {
            NasrLog( "Cannot load song: too many songs loaded.\n" );
            return -1;
        }

        // Initialize song.
        songs[ id ].id = entry->id = id;
        songs[ id ].mute = 0;
        songs[ id ].volume = master_volume;
        songs[ id ].pitch = 1.0f;

        alSourceStop( sources[ id ] );

        ALfloat pos[] = { -2.0, 0.0, 0.0 };
        ALfloat velocity[] = { 0.0, 0.0, 0.0 };

        buffers[ id ] = alutCreateBufferFromFile( filename );

        alSourcef( sources[ id ], AL_PITCH, songs[ id ].pitch );
        alSourcef( sources[ id ], AL_GAIN, songs[ id ].volume );
        alSourcefv( sources[ id ], AL_POSITION, pos );
        alSourcefv( sources[ id ], AL_VELOCITY, velocity );
        alSourcei( sources[ id ], AL_BUFFER, buffers[ id ] );
        alSourcei( sources[ id ], AL_LOOPING, AL_FALSE );
    }

    return entry->id;
};

void NasrPlaySong( unsigned int id )
{
    ALuint playing;
    alGetSourcei( sources[ id ], AL_SOURCE_STATE, &playing );
    if ( playing != AL_PLAYING )
    {
        alSourcePlay( sources[ id ] );
    }
};

void NasrStopSong( unsigned int id )
{
    alSourceStop( sources[ id ] );
};

void NasrPauseSong( unsigned int id )
{
    alSourcePause( sources[ id ] );
};

void NasrToggleSong( unsigned int id )
{
    ALuint playing;
    alGetSourcei( sources[ id ], AL_SOURCE_STATE, &playing );
    if ( playing == AL_PLAYING )
    {
        alSourcePause( sources[ id ] );
    }
    else
    {
        alSourcePlay( sources[ id ] );
    }
};

void NasrVolumeSet( unsigned int id, float amount )
{
    songs[ id ].volume = amount;
    if ( songs[ id ].volume < 0.0f )
    {
        songs[ id ].volume = 0.0f;
    }
    else if ( songs[ id ].volume > 2.0f )
    {
        songs[ id ].volume = 8.0f;
    }
    alSourcef( sources[ id ], AL_GAIN, songs[ id ].volume );
    songs[ id ].mute = 0;
};

void NasrVolumeIncrease( unsigned int id, float amount )
{
    NasrVolumeSet( id, songs[ id ].volume + amount );
};

void NasrVolumeDecrease( unsigned int id, float amount )
{
    NasrVolumeSet( id, songs[ id ].volume - amount );
};

void NasrVolumeMute( unsigned int id )
{
    alSourcef( sources[ id ], AL_GAIN, 0.0f );
    songs[ id ].mute = 1;
};

void NasrVolumeUnMute( unsigned int id )
{
    alSourcef( sources[ id ], AL_GAIN, songs[ id ].volume );
    songs[ id ].mute = 0;
};

void NasrVolumeToggleMute( unsigned int id )
{
    if ( songs[ id ].mute )
    {
        NasrVolumeUnMute( id );
    }
    else
    {
        NasrVolumeMute( id );
    }
};

void NasrPitchSet( unsigned int id, float amount )
{
    songs[ id ].pitch = amount;
    if ( songs[ id ].pitch < 0.0f )
    {
        songs[ id ].pitch = 0.0f;
    }
    else if ( songs[ id ].pitch > 2.0f )
    {
        songs[ id ].pitch = 2.0f;
    }
    alSourcef( sources[ id ], AL_PITCH, songs[ id ].pitch );
};

void NasrPitchIncrease( unsigned int id, float amount )
{
    NasrPitchSet( id, songs[ id ].pitch + amount );
};

void NasrPitchDecrease( unsigned int id, float amount )
{
    NasrPitchSet( id, songs[ id ].pitch - amount );
};

static void TestForErrors( void )
{
    int error = alGetError();
    if( error != AL_NO_ERROR ) 
    {
        printf( "Nasringine Audio Error: %s\n", alutGetErrorString( error ) );
    }
};

static SongEntry * NasrFindSongEntry( const char * needle_string, hash_t needle_hash )
{
    while ( 1 )
    {
        SongEntry * entry = &songmap.entries[ needle_hash ];
        if ( !entry->key.string || strcmp( entry->key.string, needle_string ) == 0 )
        {
            return entry;
        }
        needle_hash = ( needle_hash + 1 ) % songmap.capacity;
    }
};