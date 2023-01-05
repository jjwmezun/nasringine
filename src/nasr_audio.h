#ifndef NASR_AUDIO_H
#define NASR_AUDIO_H

#include <stdint.h>

int NasrAudioInit( unsigned int maxsongs, unsigned int perma_queuesize, unsigned int temp_queuesize );
void NasrAudioClose( void );
int NasrLoadSong( const char * filename );
int NasrAddTemporarySoundtoQueue( unsigned int songid, uint_fast8_t loop );
int NasrAddPermanentSoundtoQueue( unsigned int songid, uint_fast8_t loop );
void NasrPlaySong( unsigned int id );
void NasrStopSong( unsigned int id );
void NasrPauseSong( unsigned int id );
void NasrToggleSong( unsigned int id );
void NasrVolumeSet( unsigned int id, float amount );
void NasrVolumeIncrease( unsigned int id, float amount );
void NasrVolumeDecrease( unsigned int id, float amount );
void NasrVolumeMute( unsigned int id );
void NasrVolumeUnMute( unsigned int id );
void NasrVolumeToggleMute( unsigned int id );
void NasrPitchSet( unsigned int id, float amount );
void NasrPitchIncrease( unsigned int id, float amount );
void NasrPitchDecrease( unsigned int id, float amount );
void NasrSetSongLoop( unsigned int id, uint_fast8_t value );

#endif // NASR_AUDIO_H