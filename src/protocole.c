#include "protocole.h"
#include "errors.h"
#include "traces.h"
#include "utils.h"

#include <errno.h>
#include <fcntl.h>
#include <ncurses.h> // getch()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TOUCHE_ECHAP 27

bool envoyer_message( Jeu * jeu, Entete entete, const void * corps, size_t taille_du_corps ) {
   TRACE( jeu->journal, "%s", entete_texte( entete ));
   size_t taille = 1 + taille_du_corps + 1;
   char * message = malloc( taille );
   *message = (char)entete;
   memcpy( message+1, corps, taille_du_corps );
   ssize_t retCode;
   bool ok = CHECK_SYS( jeu, retCode = send( jeu->socket, message, taille, 0 ))
      &&  CHECK_VAL( jeu, retCode, taille );
   free( message );
   return ok;
}

bool requete_reponse(
   Jeu *        jeu,
   Entete       entete,
   const void * requete,
   size_t       taille_de_la_requete,
   void *       reponse,
   size_t       taille_de_la_reponse,
   unsigned     delai_max             )
{
   ssize_t retCode = 0;
   bool encore;
   uint64_t atStart = heure_courante_en_ms();
   if( envoyer_message( jeu, entete, requete, taille_de_la_requete )) {
      size_t taille = 1 + taille_de_la_reponse;
      char * message = malloc( taille );
      if( message == NULL ) {
         return false;
      }
      memset( message, 0, taille );
      encore = true;
      while( encore ) {
         retCode = recv( jeu->socket, message, taille, 0 );
         if( retCode < 0 ) {
            encore = ( errno == EAGAIN );
            if( encore ) {
               if(( ! jeu->est_une_ia )&&( getch() == TOUCHE_ECHAP )) {
                  jeu->action = Quitter;
                  jeu->etat   = Fin_du_programme;
                  free( message );
                  return false;
               }
               uint64_t now = heure_courante_en_ms();
               if( now - atStart > delai_max ) {
                  TRACE( jeu->journal, "expiration du délai (%ld > %ld)", now - atStart, delai_max );
                  jeu->action = Quitter;
                  jeu->etat   = Fin_du_programme;
                  free( message );
                  return false;
               }
            }
            else {
               free( message );
               return CHECK_SYS( jeu, -1 );
            }
         }
         else {
            const char * pluriel = ( retCode > 1 ) ? "s" : "";
            TRACE( jeu->journal, "%"FMT_SIZE_T"d octet%s reçu%s : %s", retCode, pluriel, pluriel, retCode > 0 ? entete_texte( message[0] ) : "" );
            if( (Entete)message[0] == entete ) {
               encore = false;
               memcpy( reponse, message+1, taille_de_la_reponse );
            }
            else {
               encore = true;
            }
         }
      }
      free( message );
   }
   TRACE( jeu->journal, "retCode : %"FMT_SIZE_T"d", retCode );
   return ( retCode > 0 );
}

bool lire_la_socket_et_le_clavier( Jeu * jeu, Entete entete, void * corps, size_t taille_du_corps, unsigned delai_max ) {
   TRACE( jeu->journal, "%s, delai = %ld", entete_texte( entete ), delai_max );
   uint64_t atStart = heure_courante_en_ms();
   size_t taille = 1+taille_du_corps;
   char * message = malloc( taille );
   for(;;) {
      memset( message, 0, taille );
      ssize_t retCode = recv( jeu->socket, message, taille, 0 );
      if( retCode < 0 ) {
         if( errno != EAGAIN ) {
            free( message );
            return CHECK_SYS( jeu, -1 );
         }
      }
      else if((Entete)message[0] == entete ) {
         const char * pluriel = ( retCode > 1 ) ? "s" : "";
         TRACE( jeu->journal, "%"FMT_SIZE_T"d octet%s reçu%s", retCode, pluriel, pluriel );
         if( CHECK_VAL( jeu, retCode, taille )) {
            memcpy( corps, message+1, taille_du_corps );
            free( message );
            return true;
         }
         free( message );
         return false;
      }
      sleep_ms( 20 );
      if(( ! jeu->est_une_ia )&&( getch() == TOUCHE_ECHAP )) {
         jeu->action = Quitter;
         jeu->etat   = Fin_du_programme;
         TRACE( jeu->journal, "touche 'Echap' %s", "pressée" );
         free( message );
         return false;
      }
      uint64_t now = heure_courante_en_ms();
      if( now - atStart > delai_max ) {
         jeu->action = Quitter;
         jeu->etat   = Fin_du_programme;
         TRACE( jeu->journal, "expiration du délai (%ld > %ld)", now - atStart, delai_max );
         free( message );
         return false;
      }
   }
}

bool initialiser_la_socket( Jeu * jeu ) {
   ENTREE( jeu->journal );
#ifdef _WIN32
   WSADATA wsaData;
   if( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) ) {
      return false;
   }
#endif
   jeu->socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
   if( jeu->socket
#ifdef _WIN32
                   == INVALID_SOCKET
#else
                   <  0
#endif
   ) {
      ECHEC( jeu->journal );
      return false;
   }
#ifdef _WIN32
   unsigned long l = 1;
   ioctlsocket( jeu->socket, (int)FIONBIO, &l );
#else
   fcntl( jeu->socket, F_SETFL, fcntl( jeu->socket, F_GETFL, 0 ) | O_NONBLOCK );
#endif
   jeu->est_une_ia = strcmp( jeu->nom_du_joueur, ORDINATEUR ) == 0;
   bool l_adversaire_est_l_ordinateur = ( ! jeu->est_une_ia )&&( strcmp( jeu->nom_de_l_adversaire, ORDINATEUR ) == 0 );
   TRACE( jeu->journal, "le joueur est une ia : %s, l_adversaire_est_l_ordinateur : %s",
      jeu->est_une_ia ? "oui" : "non", l_adversaire_est_l_ordinateur ? "oui" : "non" );
   if( l_adversaire_est_l_ordinateur ) {
      snprintf( jeu->nom_de_l_adversaire, sizeof( jeu->nom_de_l_adversaire ), "127.0.0.1:2416/127.0.0.1:2417" );
   }
   else if( jeu->est_une_ia ) {
      snprintf( jeu->nom_de_l_adversaire, sizeof( jeu->nom_de_l_adversaire ), "127.0.0.1:2417/127.0.0.1:2416" );
   }
   char * s = strchr( jeu->nom_de_l_adversaire, ':' );
   if( s == NULL ) {
      ECHEC( jeu->journal );
      return false;
   }
   char * ip_locale = jeu->nom_de_l_adversaire;
   *s = '\0';
   unsigned short port_local = (unsigned short)atoi( ++s );
   s = strchr( s, '/' );
   if( s == NULL ) {
      ECHEC( jeu->journal );
      return false;
   }
   *s = '\0';
   char * ip_adv = ++s;
   s = strchr( s, ':' );
   if( s == NULL ) {
      ECHEC( jeu->journal );
      return false;
   }
   *s = '\0';
   unsigned short port_adv = (unsigned short)atoi( ++s );
   TRACE( jeu->journal, "ip locale : %s, port local : %d, ip distante: %s, port distant: %d", ip_locale, port_local, ip_adv, port_adv );
   struct sockaddr_in adresse;
   adresse.sin_family = AF_INET;
   adresse.sin_port   = htons( port_local );
   if(   CHECK_SYS( jeu, inet_pton( AF_INET, ip_locale, &adresse.sin_addr.s_addr ))
      && CHECK_SYS( jeu, bind( jeu->socket, (const struct sockaddr *)&adresse, sizeof( adresse )))
      && CHECK_SYS( jeu, inet_pton( AF_INET, ip_adv, &adresse.sin_addr.s_addr )))
   {
      adresse.sin_port = htons( port_adv );
      if( ! CHECK_SYS( jeu, connect( jeu->socket, (const struct sockaddr *)&adresse, sizeof( adresse )))) {
         ECHEC( jeu->journal );
         return false;
      }
      ssize_t retCode;
      bool encore;
      uint64_t atStart = heure_courante_en_ms();
      memset( jeu->nom_de_l_adversaire, 0, ADVERSAIRE_MAX );
      do {
         encore = false;
         if( envoyer_message( jeu, PROTOCOLE_POIGNEE_DE_MAIN, jeu->nom_du_joueur, strlen( jeu->nom_du_joueur ) + 1 )) {
            if( jeu->etat != Debut_du_programme ) {
               TRACE( jeu->journal, "requête envoyée (%"FMT_SIZE_T"d octet%s)", retCode, ( retCode > 1 ) ? "s" : "" );
            }
            char message[1+ADVERSAIRE_MAX];
            retCode = recv( jeu->socket, message, 1+ADVERSAIRE_MAX, 0 );
            if( retCode < 0 ) {
               encore = ( errno == EAGAIN )||( errno == ECONNREFUSED );
               if( encore ) {
                  sleep_ms( 1000 );
                  uint64_t now = heure_courante_en_ms();
                  if( now - atStart > DELAI_MAX_POUR_CONNECTER_LES_JOUEURS ) {
                     jeu->action = Quitter;
                     jeu->etat   = Fin_du_programme;
                     TRACE( jeu->journal, "échec : expiration du délai (%ld > %u)", now - atStart, DELAI_MAX_POUR_CONNECTER_LES_JOUEURS );
                     return false;
                  }
               }
               else {
                  ECHEC( jeu->journal );
                  return CHECK_SYS( jeu, -1 );
               }
            }
            else {
               encore = ( message[0] != PROTOCOLE_POIGNEE_DE_MAIN )&&( jeu->etat != Fin_du_programme );
               snprintf( jeu->nom_de_l_adversaire, sizeof( jeu->nom_de_l_adversaire ), "%s", message + 1 );
               TRACE( jeu->journal, "réponse reçue (%"FMT_SIZE_T"d octet%s)", retCode, ( retCode > 1 ) ? "s" : "" );
            }
         }
         else {
            encore = false;
         }
      } while( encore );
      TRACE( jeu->journal, "return %s", ( retCode > 0 ) ? "true" : "false" );
      return ( retCode > 0 );
   }
   ECHEC( jeu->journal );
   return false;
}
