#pragma once

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <direct.h>
#  include <process.h>
#  include <windows.h>
   typedef SSIZE_T         ssize_t;
   typedef SOCKET          socket_t;
#  ifdef _MSC_VER
#     define  PATH_MAX     MAX_PATH
      typedef HANDLE       pthread_t;
      int pthread_create( pthread_t * id, void * attr, void * ( * routine )( void *), void * arg );
      int pthread_join( pthread_t id, void** not_used );
      pthread_t pthread_self( void );
#  else
#     include <pthread.h>
#  endif
#  define  send(S,M,T,F)   send(S,M,(int)T,F)
#  define  recv(S,M,T,F)   send(S,M,(int)T,F)
#  define  make_dir(C,P)   _mkdir(C)
#  define  getpid          _getpid
#  define  FMT_SIZE_T      "ll"
#else
#  include <arpa/inet.h>
#  include <pthread.h>
#  include <sys/socket.h>
#  include <sys/time.h>
#  include <sys/un.h>
   typedef int             socket_t;
#  define  make_dir(C,P)   mkdir(C,P)
#  define  FMT_SIZE_T      "l"
#endif
