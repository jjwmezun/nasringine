#ifndef NASR_AUDIO_H
#define NASR_AUDIO_H

void NasrAudioInit( void );
void NasrAudioClose( void );
void NasrLoadSong( const char * filename );
void NasrPlaySong( const char * filename );
void NasrStopSong( void );
void NasrPauseSong( void );
void NasrToggleSong( void );
void NasrVolumeSet( float amount );
void NasrVolumeIncrease( float amount );
void NasrVolumeDecrease( float amount );
void NasrVolumeMute( void );
void NasrVolumeUnMute( void );
void NasrVolumeToggleMute( void );
void NasrPitchSet( float amount );
void NasrPitchIncrease( float amount );
void NasrPitchDecrease( float amount );

#endif // NASR_AUDIO_H