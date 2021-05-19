#include "errors.h"

#include "delais.h"
#include "ia.h"
#include "ihm.h"
#include "protocole.h"
#include "traces.h"
#include "utils.h"

#include <limits.h>  // PATH_MAX
#include <stdlib.h>  // EXIT_{SUCCESS,FAILURE}, malloc
#include <string.h>  // strcmp, strlen, strncmp

static const char * ARG_NOM_DU_JOUEUR = "--nom=";
static const char * ARG_INFOS_RESEAU  = "--reseau=";
static const char * ARG_JOURNAL       = "--journal=";

static int usage( const char * exename, const char * arg ) {
   if( arg && strcmp( arg, "-?" ) && strcmp( arg, "--help" )) {
      fprintf( stderr, "Argument inattendu : '%s'\n", arg );
   }
   fprintf( stderr, "usage: %s OPTIONS\n"
      "OPTIONS:\n"
      "\t%s<nom>\n"
      "\t[%s<Adresse IP locale>:<Port local>/<Adresse IP de l'adversaire>:<Port de l'adversaire>]\n"
      "\t[%s<chemin du journal sans le nom de fichier>\n"
      "Ne donnez aucune valeur à l'option réseau pour jouer contre l'ordinateur.\n",
      exename, ARG_NOM_DU_JOUEUR, ARG_INFOS_RESEAU, ARG_JOURNAL );
   return EXIT_FAILURE;
}

int main( int argc, char * argv[] ) {
   const size_t joueur_len  = strlen( ARG_NOM_DU_JOUEUR );
   const size_t reseau_len  = strlen( ARG_INFOS_RESEAU );
   const size_t journal_len = strlen( ARG_JOURNAL );
   char chemin_du_journal[PATH_MAX] = "";
   Jeu * jeu = malloc( sizeof( Jeu ));
   Jeu * jeux[] = { jeu, NULL };
   initialiser( jeu );
   for( int i = 1; i < argc; ++i ) {
      const char * arg = argv[i];
      if( strncmp( ARG_NOM_DU_JOUEUR, arg, joueur_len ) == 0 ) {
         snprintf( jeu->nom_du_joueur, sizeof( jeu->nom_du_joueur ), "%s", arg + joueur_len );
      }
      else if( strncmp( ARG_INFOS_RESEAU, arg, reseau_len ) == 0 ) {
         snprintf( jeu->nom_de_l_adversaire, sizeof( jeu->nom_de_l_adversaire ), "%s", arg + reseau_len );
      }
      else if( strncmp( ARG_JOURNAL, arg, journal_len ) == 0 ) {
         snprintf( chemin_du_journal, sizeof( chemin_du_journal ), "%s", arg + journal_len );
      }
      else {
         return usage( argv[0], arg );
      }
   }
   if( 0 == jeu->nom_du_joueur[0] ) {
      return usage( argv[0], NULL );
   }
   initialiser_le_journal( jeu, chemin_du_journal );
   pthread_t ia_thread = 0UL;
   if(( 0 == jeu->nom_de_l_adversaire[0] )&&( ! initialiser_ia( jeux, chemin_du_journal, &ia_thread ))) {
      return EXIT_FAILURE;
   }
   initialiser_l_ihm( jeux );
   if( ! initialiser_le_protocole( jeu, ia_thread != 0, DELAI_MAX_POUR_CONNECTER_LES_JOUEURS )) {
      return EXIT_FAILURE;
   }
   reinitialiser( jeu );
   while( jeu->etat != Fin_du_programme ) {
      afficher( jeu );
      obtenir_la_decision_du_joueur( jeu );
      jouer( jeu );
   }
   Jeu * ia = jeux[1];
   if( ia_thread ) {
      TRACE( jeu->journal, "en attente de la fin de %s", "pthread_join" );
      ia->action = Quitter;
      ia->etat   = Fin_du_programme;
      CHECK_SYS( jeu, pthread_join( ia_thread, NULL ));
      TRACE( jeu->journal, "Thread I.A. %s", "terminé." );
   }
   liberer_les_ressources_reseau( jeux );
   liberer_les_ressources_ihm( jeu );
   liberer_les_ressources( ia );
   liberer_les_ressources( jeu );
   return EXIT_SUCCESS;
}
