#include "errors.h"
#include "ia.h"
#include "protocole.h"
#include "traces.h"
#include "utils.h"

#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

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

static Jeu * ctrl_c_handler_jeu;
static Jeu * ctrl_c_handler_ia;

static void ctrl_c_handler( int num ) {
   ctrl_c_handler_jeu->action = Quitter;
   ctrl_c_handler_jeu->etat   = Fin_du_programme;
   ctrl_c_handler_ia ->action = Quitter;
   ctrl_c_handler_ia ->etat   = Fin_du_programme;
   (void)num;
}

static void initialiser_curses( Jeu * jeu ) {
   ENTREE( jeu->journal );
   signal( SIGINT, &ctrl_c_handler );
   initscr();
   cbreak();
   noecho();
   halfdelay( 1 );
   keypad( stdscr, TRUE );
   curs_set( 0 );
   start_color();
   init_pair( COULEUR_BATEAU                      , COLOR_WHITE, COLOR_BLACK );
   init_pair( COULEUR_BATEAU_EN_COURS_DE_PLACEMENT, COLOR_RED  , COLOR_BLACK );
   init_pair( COULEUR_TORPILLE_DANS_L_EAU         , COLOR_BLUE , COLOR_BLACK );
   init_pair( COULEUR_TORPILLE_A_TOUCHE           , COLOR_GREEN, COLOR_BLACK );
   init_pair( COULEUR_TORPILLE_A_COULE            , COLOR_RED  , COLOR_BLACK );
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
   if( chemin_du_journal[0] ) {
      make_dir( chemin_du_journal, 0777 );
      char fullpath[PATH_MAX+21+21];
      snprintf( fullpath, sizeof( fullpath ), "%s/%s.txt", chemin_du_journal, jeu->nom_du_joueur );
      jeu->journal = fopen( fullpath, "wt" );
   }
   pthread_t ia_thread = 0UL;
   Jeu * ia = NULL;
   if(( 0 == jeu->nom_de_l_adversaire[0] )) {
      jeux[1] = ia = malloc( sizeof( Jeu ));
      initialiser( ia );
      snprintf( jeu->nom_de_l_adversaire, sizeof( jeu->nom_de_l_adversaire ), ORDINATEUR );
      snprintf( ia ->nom_du_joueur      , sizeof( ia ->nom_du_joueur       ), ORDINATEUR );
      snprintf( ia ->nom_de_l_adversaire, sizeof( ia ->nom_de_l_adversaire ), "%s", jeu->nom_du_joueur );
      if( chemin_du_journal[0] ) {
         char fullpath[PATH_MAX+30];
         snprintf( fullpath, sizeof( fullpath ), "%s/ia.txt", chemin_du_journal );
         ia->journal = fopen( fullpath, "wt" );
      }
      if( ! CHECK_SYS( jeu, pthread_create( &ia_thread, NULL, ia_main, jeux ))) {
         ECHEC( jeu->journal );
         return EXIT_FAILURE;
      }
   }
   if( ! initialiser_la_socket( jeu )) {
      ECHEC( jeu->journal );
      return EXIT_FAILURE;
   }
   initialiser_curses( jeu );
   reinitialiser( jeu );
   while( jeu->etat != Fin_du_programme ) {
      afficher( jeu );
      obtenir_la_decision_du_joueur( jeu );
      jouer( jeu );
   }
   if( ia_thread ) {
      TRACE( jeu->journal, "en attente de la fin de %s", "pthread_join" );
      CHECK_SYS( jeu, pthread_join( ia_thread, NULL ));
      if( ia ) {
         liberer_les_ressources( ia );
         free( ia );
      }
   }
   TRACE( jeu->journal, "Fin du %s", "programme" );
   liberer_les_ressources( jeu );
   free( jeu );
   return EXIT_SUCCESS;
}
