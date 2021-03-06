#include "utils.h"
#include "portability.h"

#ifdef _WIN32

uint64_t heure_courante_en_ms( void ) {
   FILETIME time;
   GetSystemTimePreciseAsFileTime( &time );
   uint64_t ms = time.dwHighDateTime;
   ms <<= 32;
   ms = ms | time.dwLowDateTime;
   return ms;
}

void sleep_ms( uint64_t ms ) {
   Sleep((DWORD)ms );
}

#else
#  include <stdlib.h> // NULL
#  include <time.h>   // nanosleep
#  include <unistd.h> // getpid()

BN_API uint64_t heure_courante_en_ms( void ) {
   struct timeval tv;
   gettimeofday( &tv, NULL );
   return (uint64_t)( tv.tv_sec * 1000 + tv.tv_usec / 1000 );
}

BN_API void sleep_ms( uint64_t ms ) {
   struct timespec ts = {
      .tv_sec  = (long)( ms / 1000UL ),
      .tv_nsec = (long)(( ms % 1000UL ) * 1000UL * 1000UL )
   };
   nanosleep( &ts, NULL );
}
#endif

#ifdef _MSVC_VER

BN_API int pthread_create( pthread_t * id, void * attr, void *( * routine )( void * ), void * arg ) {
   DWORD threadId = 0;
   HANDLE hThread = CreateThread( NULL, 0, routine, arg, 0, &threadId );
   *id = hThread;
   return ( hThread == NULL ) -1 : 0;
   (void)attr;
}

BN_API int pthread_join( pthread_t id, void ** not_used ) {
   WaitForSingleObject( id, INFINITE );
   return 0;
}

#endif

BN_API void initialiser_le_generateur_de_nombre_aleatoire( void ) {
   srand((unsigned)(((unsigned)getpid()) * heure_courante_en_ms()));
}

int nombre_aleatoire_entre_zero_et( double max ) {
   double value = max * rand();
   value /= RAND_MAX;
   return (int)value;
}
