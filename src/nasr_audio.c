#include "AL/al.h"
#include <AL/alut.h>
#include "nasr_audio.h"
#include "nasr_log.h"
#include "nasr_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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



// Static Data
static float master_volume = 0.5f;
static ALuint * buffers;
static ALuint * sources;
static Sound * queue;
static SongMap songmap = { 0, 0 };
static int maxsongs_;
static int perma_queuesize_;
static int queuesize_;
static int song_pos;
static int perma_queue_pos;
static int temp_queue_pos;



// Static Functions
static int AddSongToQueue( unsigned int songid, int queueid, uint_fast8_t persistent, uint_fast8_t loop );
static SongEntry * FindSongEntry( const char * needle_string, hash_t needle_hash );
static void TestForErrors( void );



// Init/Close
int NasrAudioInit( unsigned int maxsongs, unsigned int perma_queuesize, unsigned int temp_queuesize )
{
    maxsongs_ = NASR_MATH_MIN( 256, maxsongs );
    queuesize_ = NASR_MATH_MIN( 256, perma_queuesize + temp_queuesize );
    perma_queuesize_ = perma_queuesize;
    temp_queue_pos = perma_queuesize;

    if ( maxsongs > 256 || queuesize_ > 256 )
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



// Loading
int NasrLoadSong( const char * filename )
{
    hash_t needle_hash = NasrHashString( filename, songmap.capacity );
    SongEntry * entry = FindSongEntry( filename, needle_hash );
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

int NasrAddTemporarySoundtoQueue( unsigned int songid, uint_fast8_t loop )
{
    int id = -1;
    for ( int i = temp_queue_pos; i < queuesize_; ++i )
    {
        if ( !queue[ i ].persistent )
        {               
            ALint playing;
            alGetSourcei( sources[ i ], AL_SOURCE_STATE, &playing );
            if ( playing )
            {
                id = i;
                break;
            }
        }
    }
    return AddSongToQueue( songid, id, 0, loop );
};

int NasrAddPermanentSoundtoQueue( unsigned int songid, uint_fast8_t loop )
{
    return AddSongToQueue( songid, perma_queue_pos < perma_queuesize_ ? perma_queue_pos++ : -1, 1, loop );
};



// Play/Pause/Stop
void NasrPlaySong( unsigned int id )
{
    ALint playing;
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
    ALint playing;
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



// Volume
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



// Pitch
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



// Loop
void NasrSetSongLoop( unsigned int id, uint_fast8_t value )
{
    alSourcei( sources[ id ], AL_LOOPING, value ? AL_TRUE : AL_FALSE );
};



// Static Functions
static int AddSongToQueue( unsigned int songid, int queueid, uint_fast8_t persistent, uint_fast8_t loop )
{
    if ( queueid == -1 )
    {
        NasrLog( "Cannot add song to queue: queue is full.\n" );
        return -1;
    }

    // Initialize song.
    queue[ queueid ].id = queueid;
    queue[ queueid ].persistent = persistent;
    queue[ queueid ].mute = 0;
    queue[ queueid ].volume = master_volume;
    queue[ queueid ].pitch = 1.0f;

    alSourceStop( sources[ queueid ] );

    ALfloat pos[] = { -2.0, 0.0, 0.0 };
    ALfloat velocity[] = { 0.0, 0.0, 0.0 };

    alSourcef( sources[ queueid ], AL_PITCH, queue[ queueid ].pitch );
    alSourcef( sources[ queueid ], AL_GAIN, queue[ queueid ].volume );
    alSourcefv( sources[ queueid ], AL_POSITION, pos );
    alSourcefv( sources[ queueid ], AL_VELOCITY, velocity );
    alSourcei( sources[ queueid ], AL_BUFFER, buffers[ songid ] );
    alSourcei( sources[ queueid ], AL_LOOPING, loop ? AL_TRUE : AL_FALSE );

    return queueid;
};

static SongEntry * FindSongEntry( const char * needle_string, hash_t needle_hash )
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

static void TestForErrors( void )
{
    int error = alGetError();
    if( error != AL_NO_ERROR ) 
    {
        printf( "Nasringine Audio Error: %s\n", alutGetErrorString( error ) );
    }
};