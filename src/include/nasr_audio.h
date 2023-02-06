#ifndef NASR_AUDIO_H
#define NASR_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Init/Close
int NasrAudioInit( unsigned int maxsongs, unsigned int perma_queuesize, unsigned int temp_queuesize );
void NasrAudioClose( void );

// Resetting State
void NasrAudioClear( void );

// Loading
int NasrLoadSong( const char * filename );
int NasrAddTemporarySoundtoQueue( unsigned int songid, uint_fast8_t loop );
int NasrAddPermanentSoundtoQueue( unsigned int songid, uint_fast8_t loop );

// Play/Pause/Stop
void NasrPlaySong( unsigned int id );
void NasrStopSong( unsigned int id );
void NasrPauseSong( unsigned int id );
void NasrToggleSong( unsigned int id );

// Volume
void NasrVolumeSet( unsigned int id, float amount );
void NasrVolumeIncrease( unsigned int id, float amount );
void NasrVolumeDecrease( unsigned int id, float amount );
void NasrVolumeMute( unsigned int id );
void NasrVolumeUnMute( unsigned int id );
void NasrVolumeToggleMute( unsigned int id );

// Pitch
void NasrPitchSet( unsigned int id, float amount );
void NasrPitchIncrease( unsigned int id, float amount );
void NasrPitchDecrease( unsigned int id, float amount );

// Loop
void NasrSetSongLoop( unsigned int id, uint_fast8_t value );

#ifdef __cplusplus
}
#endif

#endif // NASR_AUDIO_H