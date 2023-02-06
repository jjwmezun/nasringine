#ifndef NASR_LOCALIZATION_H
#define NASR_LOCALIZATION_H

#ifdef __cplusplus
extern "C" {
#endif

int NasrSetLanguage( const char * filename, const char * domain );
void NasrCloseLanguage( void );
const char * Nasr__( const char * string, const char * domain );
const char * Nasr_x( const char * string, const char * context, const char * domain );
const char * Nasr_n( const char * singular, const char * plural, int count, const char * domain );

#ifdef __cplusplus
}
#endif

#endif // NASR_LOCALIZATION_H