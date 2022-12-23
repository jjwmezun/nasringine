#ifndef NASR_AUDIO_H
#define NASR_AUDIO_H

int NasrAudioInit( unsigned int max );
void NasrAudioClose( void );
unsigned int NasrLoadSong( const char * filename );
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

#endif // NASR_AUDIO_H