#include "AL/al.h"
#include <AL/alut.h>
#include "nasr_audio.h"
#include "nasr_log.h"
#include "nasr_math.h"

typedef struct Sound
{
    int id;
    int_fast8_t persistent;
    int_fast8_t mute;
    float volume;
    float pitch;
} Sound;

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
static Sound * queue = 0;
static SongMap songmap = { 0, 0 };
static int maxsongs_ = 0;
static int queuesize_ = 0;
static int song_pos = 0;
static int queue_pos = 0;

static void TestForErrors( void );
static SongEntry * NasrFindSongEntry( const char * needle_string, hash_t needle_hash );

int NasrAudioInit( unsigned int maxsongs, unsigned int queuesize )
{
    maxsongs_ = NASR_MATH_MIN( 256, maxsongs );
    queuesize_ = NASR_MATH_MIN( 256, queuesize );

    if ( maxsongs > 256 || queuesize > 256 )
    {
        NasrLog( "Max too long: Can only load 256 songs at once.\n" );
    }

    ALfloat listenerpos[] = { 0.0, 0.0, 4.0 };
    ALfloat listenervel[] = { 0.0, 0.0, 0.0 };
    ALfloat listenerori[] = { 0.0, 0.0, 1.0, 0.0, 1.0, 0.0 };

    if ( !alutInit( 0, 0 ) )
    {
        TestForErrors();
        return -1;
    }

    alListenerfv( AL_POSITION, listenerpos );
    alListenerfv( AL_VELOCITY, listenervel );
    alListenerfv( AL_ORIENTATION, listenerori );

    buffers = calloc( maxsongs_, sizeof( ALuint ) );
    alGenBuffers( maxsongs_, buffers );
    sources = calloc( queuesize_, sizeof( ALuint ) );
    alGenSources( queuesize_, sources );
    TestForErrors();

    queue = malloc( sizeof( Sound ) * queuesize_ );
    for ( int i = 0; i < queuesize_; ++i )
    {
        queue[ i ].id = -1;
        queue[ i ].persistent = 0;
        queue[ i ].mute = 0;
        queue[ i ].volume = master_volume;
        queue[ i ].pitch = 1.0f;
    }
    songmap.capacity = NasrGetNextPrime( maxsongs_ + 1 );
    songmap.entries = calloc( songmap.capacity, sizeof( SongEntry ) );

    return 0;
};

void NasrAudioClose( void )
{
    if ( queue )
    {
        free( queue );
    }
    if ( songmap.entries )
    {
        free( songmap.entries );
    }
    alDeleteSources( maxsongs_, sources );
    if ( sources )
    {
        free( sources );
    }
    alDeleteBuffers( maxsongs_, buffers );
    if ( buffers )
    {
        free( buffers );
    }
    alutExit();
};

int NasrLoadSong( const char * filename )
{
    hash_t needle_hash = NasrHashString( filename, songmap.capacity );
    SongEntry * entry = NasrFindSongEntry( filename, needle_hash );
    if ( !entry->key.string )
    {
        entry->key.string = ( char * )( malloc( strlen( filename ) + 1 ) );
        strcpy( entry->key.string, filename );
        entry->key.hash = needle_hash;

        if ( song_pos >= maxsongs_ )
        {
            NasrLog( "Cannot load song: too many songs loaded.\n" );
            return -1;
        }

        entry->id = song_pos++;

        buffers[ entry->id ] = alutCreateBufferFromFile( filename );
    }

    return entry->id;
};

int NasrAddSongToQueue( unsigned int songid, int_fast8_t persistent )
{
    int id = -1;
    if ( queue_pos >= queuesize_ )
    {
        for ( int i = 0; i < queuesize_; ++i )
        {
            if ( !queue[ i ].persistent )
            {               
                ALuint playing;
                alGetSourcei( sources[ i ], AL_SOURCE_STATE, &playing );
                if ( playing )
                {
                    id = i;
                    break;
                }
            }
        }

        if ( id == -1 )
        {
            NasrLog( "Cannot add song to queue: queue is full.\n" );
            return -1;
        }
    }
    else
    {
        id = queue_pos++;
    }

    // Initialize song.
    queue[ id ].id = id;
    queue[ id ].persistent = persistent;
    queue[ id ].mute = 0;
    queue[ id ].volume = master_volume;
    queue[ id ].pitch = 1.0f;

    alSourceStop( sources[ id ] );

    ALfloat pos[] = { -2.0, 0.0, 0.0 };
    ALfloat velocity[] = { 0.0, 0.0, 0.0 };

    alSourcef( sources[ id ], AL_PITCH, queue[ id ].pitch );
    alSourcef( sources[ id ], AL_GAIN, queue[ id ].volume );
    alSourcefv( sources[ id ], AL_POSITION, pos );
    alSourcefv( sources[ id ], AL_VELOCITY, velocity );
    alSourcei( sources[ id ], AL_BUFFER, buffers[ songid ] );
    alSourcei( sources[ id ], AL_LOOPING, AL_FALSE );

    return id;
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
    queue[ id ].volume = amount;
    if ( queue[ id ].volume < 0.0f )
    {
        queue[ id ].volume = 0.0f;
    }
    else if ( queue[ id ].volume > 2.0f )
    {
        queue[ id ].volume = 8.0f;
    }
    alSourcef( sources[ id ], AL_GAIN, queue[ id ].volume );
    queue[ id ].mute = 0;
};

void NasrVolumeIncrease( unsigned int id, float amount )
{
    NasrVolumeSet( id, queue[ id ].volume + amount );
};

void NasrVolumeDecrease( unsigned int id, float amount )
{
    NasrVolumeSet( id, queue[ id ].volume - amount );
};

void NasrVolumeMute( unsigned int id )
{
    alSourcef( sources[ id ], AL_GAIN, 0.0f );
    queue[ id ].mute = 1;
};

void NasrVolumeUnMute( unsigned int id )
{
    alSourcef( sources[ id ], AL_GAIN, queue[ id ].volume );
    queue[ id ].mute = 0;
};

void NasrVolumeToggleMute( unsigned int id )
{
    if ( queue[ id ].mute )
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
    queue[ id ].pitch = amount;
    if ( queue[ id ].pitch < 0.0f )
    {
        queue[ id ].pitch = 0.0f;
    }
    else if ( queue[ id ].pitch > 2.0f )
    {
        queue[ id ].pitch = 2.0f;
    }
    alSourcef( sources[ id ], AL_PITCH, queue[ id ].pitch );
};

void NasrPitchIncrease( unsigned int id, float amount )
{
    NasrPitchSet( id, queue[ id ].pitch + amount );
};

void NasrPitchDecrease( unsigned int id, float amount )
{
    NasrPitchSet( id, queue[ id ].pitch - amount );
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