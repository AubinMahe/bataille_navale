#include "ia.h"

#include "delais.h"
#include "errors.h"
#include "protocole.h"
#include "traces.h"
#include "utils.h"

#include <limits.h>  // PATH_MAX
#include <stdlib.h>  // malloc, rand
#include <string.h>  // memset
#include <unistd.h>  // close

#define ORDINATEUR "ordinateur"

typedef Etat_torpille Tentatives[COLONNE_MAX][LIGNE_MAX];

static bool placer_un_navire( Jeu * ia, Etat_du_jeu prochain ) {
   TRACE( ia->journal, "Navire n°%d", ia->index_navire );
   Navire * navire     = ia->navires + ia->index_navire;
   navire->colonne     = nombre_aleatoire_entre_zero_et( COLONNE_MAX );
   navire->ligne       = nombre_aleatoire_entre_zero_et( LIGNE_MAX );
   navire->orientation = rand() > RAND_MAX/2;
   unsigned interruption_de_boucle_infinie = 0;
   while( ! controler_le_placement_du_navire( ia )) {
      navire->colonne     = nombre_aleatoire_entre_zero_et( COLONNE_MAX );
      navire->ligne       = nombre_aleatoire_entre_zero_et( LIGNE_MAX );
      navire->orientation = rand() > RAND_MAX/2;
      if( ++interruption_de_boucle_infinie > 100 ) {
         return false;
      }
   }
   TRACE( ia->journal, "Navire placé en : {%d, %d, %s}",
      navire->colonne, navire->ligne, navire->orientation == Horizontale ? "Horizontale" : "Verticale" );
   ++(ia->index_navire);
   ia->etat = prochain;
   return true;
}

static bool rechercher_a_l_horizontale( Jeu * ia, Tentatives tentatives, int * colonne, int ligne, int sens ) {
   TRACE( ia->journal, "colonne = %d, ligne = %d", *colonne, ligne);
   for( int c = *colonne; ( c >= 0 )&&( c < COLONNE_MAX ); c += sens ) {
      Etat_torpille e = tentatives[c][ligne];
      if( e == et_Aucun ) {
         *colonne = c;
         TRACE( ia->journal, "L'extrémité n'est pas bordée en {%d, %d}, on le tente.", c, ligne );
         return true;
      }
      if( e == et_Dans_l_eau ) {
         TRACE( ia->journal, "L'extrémité est bordée (dans l'eau), %s", "il faut chercher ailleurs." );
         return false;
      }
   }
   TRACE( ia->journal, "L'extrémité est bordée (le bord), %s", "il faut chercher ailleurs." );
   return false;
}

static bool rechercher_a_la_verticale( Jeu * ia, Tentatives tentatives, int colonne, int * ligne, int sens ) {
   TRACE( ia->journal, "colonne = %d, ligne = %d", colonne, *ligne);
   for( int l = *ligne; ( l >= 0 )&&( l < LIGNE_MAX ); l += sens ) {
      Etat_torpille e = tentatives[colonne][l];
      if( e == et_Aucun ) {
         *ligne = l;
         TRACE( ia->journal, "L'extrémité n'est pas bordée en {%d, %d}, on le tente.", colonne, l );
         return true;
      }
      if( e == et_Dans_l_eau ) {
         TRACE( ia->journal, "L'extrémité est bordée (dans l'eau), %s", "il faut chercher ailleurs." );
         return false;
      }
   }
   TRACE( ia->journal, "L'extrémité est bordée (le bord), %s", "il faut chercher ailleurs." );
   return false;
}

static bool cherche_une_ligne( Jeu * ia, Tentatives tentatives, Torpille * torpille  ) {
   ENTREE( ia->journal );
   bool trouve = false;
   for( int i = 0; ( ! trouve )&&( i < ia->index_torpille ); ++i ) {
      Torpille * t = ia->torpilles + i;
      if( t->etat == et_Touche ) {
         for( int sens = -1; sens < 2; sens += 2 ) {
            if( t->colonne == 0 ) {
               sens = +1;
            }
            if( t->colonne == COLONNE_MAX - 1 ) {
               break;
            }
            Etat_torpille voisine = tentatives[t->colonne+sens][t->ligne];
            if( voisine == et_Touche ) {
               TRACE( ia->journal, "Une ligne horizontale se dessine en {%d, %d}, on doit vérifier l'extrémité %s",
                  t->colonne, t->ligne, ( sens < 0 ) ? "gauche" : "droite" );
               torpille->colonne = t->colonne;
               torpille->ligne   = t->ligne;
               trouve = rechercher_a_l_horizontale( ia, tentatives, &torpille->colonne, torpille->ligne, sens );
            }
         }
         if( ! trouve ) {
            for( int sens = -1; sens < 2; sens += 2 ) {
               if( t->ligne == 0 ) {
                  sens = +1;
               }
               if( t->ligne == LIGNE_MAX - 1 ) {
                  break;
               }
               Etat_torpille voisine = tentatives[t->colonne][t->ligne+sens];
               if( voisine == et_Touche ) {
                  TRACE( ia->journal, "Une ligne verticale se dessine en {%d, %d}, on doit vérifier l'extrémité %s",
                     t->colonne, t->ligne, ( sens < 0 ) ? "gauche" : "droite" );
                  torpille->colonne = t->colonne;
                  torpille->ligne   = t->ligne;
                  trouve = rechercher_a_la_verticale( ia, tentatives, torpille->colonne, &torpille->ligne, sens );
               }
            }
         }
      }
   }
   return trouve;
}

static bool cherche_a_gauche( Jeu * ia, Tentatives tentatives, Torpille * touche, Torpille * torpille ) {
   ENTREE( ia->journal );
   if(( touche->colonne > 0 )&&( tentatives[touche->colonne-1][touche->ligne] == et_Aucun )) {
      torpille->colonne = touche->colonne - 1;
      torpille->ligne   = touche->ligne;
      return true;
   }
   return false;
}

static bool cherche_a_droite( Jeu * ia, Tentatives tentatives, Torpille * touche, Torpille * torpille ) {
   ENTREE( ia->journal );
   if(( touche->colonne < COLONNE_MAX - 1 )&&( tentatives[touche->colonne+1][touche->ligne] == et_Aucun )) {
      torpille->colonne = touche->colonne + 1;
      torpille->ligne   = touche->ligne;
      return true;
   }
   return false;
}

static bool cherche_en_haut( Jeu * ia, Tentatives tentatives, Torpille * touche, Torpille * torpille ) {
   ENTREE( ia->journal );
   if(( touche->ligne > 0 )&&( tentatives[touche->colonne][touche->ligne-1] == et_Aucun )) {
      torpille->colonne = touche->colonne;
      torpille->ligne   = touche->ligne - 1;
      return true;
   }
   return false;
}


static bool cherche_en_bas( Jeu * ia, Tentatives tentatives, Torpille * touche, Torpille * torpille ) {
   ENTREE( ia->journal );
   if(( touche->ligne < LIGNE_MAX - 1 )&&( tentatives[touche->colonne][touche->ligne+1] == et_Aucun )) {
      torpille->colonne = touche->colonne;
      torpille->ligne   = touche->ligne + 1;
      return true;
   }
   return false;
}

typedef bool ( * cherche_autour_t )( Jeu * ia, Tentatives tentatives, Torpille * touche, Torpille * torpille );

static cherche_autour_t cherche_autour[] = {
   cherche_a_gauche,
   cherche_a_droite,
   cherche_en_haut,
   cherche_en_bas
};

static bool cherche_un_bateau_touche( Jeu * ia, Tentatives tentatives, Torpille * torpille ) {
   ENTREE( ia->journal );
   bool placee = false;
   for( int i = 0; ( ! placee )&&( i < ia->index_torpille ); ++i ) {
      Torpille * t = ia->torpilles + i;
      if( t->etat == et_Touche ) {
         int essais = 0;
         // Tente de placer une mine autour du point "t" dans un ordre aléatoire
         while(( essais != 0x0F )&&( ! placee )) {
            int index = nombre_aleatoire_entre_zero_et( 4 );
            essais = essais | ( 1 << index );
            placee = cherche_autour[index]( ia, tentatives, t, torpille );
         }
      }
   }
   return placee;
}

static bool placer_la_torpille_au_hasard( Jeu * ia, Torpille * torpille ) {
   ENTREE( ia->journal );
   torpille->colonne = nombre_aleatoire_entre_zero_et( COLONNE_MAX );
   torpille->ligne   = nombre_aleatoire_entre_zero_et( LIGNE_MAX );
   unsigned interruption_de_boucle_infinie = 0;
   while( ! controler_le_placement_de_la_torpille( ia )) {
      torpille->colonne = nombre_aleatoire_entre_zero_et( COLONNE_MAX );
      torpille->ligne   = nombre_aleatoire_entre_zero_et( LIGNE_MAX );
      if( ++interruption_de_boucle_infinie > 100 ) {
         return false;
      }
   }
   return true;
}

static bool placer_une_torpille( Jeu * ia, Tentatives tentatives ) {
   ENTREE( ia->journal );
   Torpille * torpille = ia->torpilles + ia->index_torpille;
   bool trouve = cherche_une_ligne( ia, tentatives, torpille );
   if( ! trouve ) {
      trouve = cherche_un_bateau_touche( ia, tentatives, torpille );
      if( ! trouve ) {
         trouve = placer_la_torpille_au_hasard( ia, torpille );
         if( ! trouve ) {
            return false;
         }
      }
   }
   torpille->etat = et_Posee;
   int action[] = {
      torpille->colonne,
      torpille->ligne
   };
   if(   envoyer_message( ia, PROTOCOLE_TORPILLE_POSEE, action, sizeof( action ))
      && lire_la_socket( ia, PROTOCOLE_DEGATS_OCCASIONNES, &torpille->etat, sizeof( torpille->etat ), 500 ))
   {
      TRACE( ia->journal, "résultat : %s", etat_torpille_texte( torpille->etat ));
      tentatives[torpille->colonne][torpille->ligne] = torpille->etat;
      ++(ia->index_torpille);
      unsigned compteur_de_bateaux_coules = 0;
      for( int i = 0; i < ia->index_torpille; ++i ) {
         if( ia->torpilles[i].etat == et_Coule ) {
            ++compteur_de_bateaux_coules;
         }
      }
      ia->action = Jouer;
      if( compteur_de_bateaux_coules == NAVIRE_MAX ) {
         ia->partie_gagnee = true;
         ia->etat          = Partie_achevee;
      }
      else {
         ia->etat = En_attente_de_l_autre_joueur;
      }
   }
   else {
      ECHEC( ia->journal );
      return false;
   }
   return true;
}

static bool reflechir( Jeu ** jeux, Tentatives tentatives ) {
   Jeu * humain = jeux[0];
   Jeu * ia     = jeux[1];
   ENTREE( ia->journal );
   ia->action = Jouer;
   if( ia->etat == Attendre_decision_rejouer ) {
      TRACE( ia->journal, "Partie terminée en attente de la décision du %s", "joueur" );
      if( humain->etat == Tirage_au_sort ) {
         TRACE( ia->journal, "Le joueur a choisi de %s", "rejouer, on réinitialise." );
         reinitialiser( ia );
      }
      else {
         TRACE( ia->journal, "Mise en sommeil pour une %s", "seconde" );
         sleep_ms( 1000 );
      }
   }
   else if( une_action_du_joueur_est_attendue( ia )) {
      switch( ia->etat ) {
      default: break;
      case Placement_du_porte_avion        : return placer_un_navire( ia, Placement_du_croiseur );
      case Placement_du_croiseur           : return placer_un_navire( ia, Placement_du_contre_torpilleur_1 );
      case Placement_du_contre_torpilleur_1: return placer_un_navire( ia, Placement_du_contre_torpilleur_2 );
      case Placement_du_contre_torpilleur_2: return placer_un_navire( ia, Placement_du_torpilleur );
      case Placement_du_torpilleur         : return placer_un_navire( ia, Tirage_au_sort );
      case Placement_d_une_torpille        : return placer_une_torpille( ia, tentatives );
      }
   }
   return true;
}

static int retCode = 0;

static void * ia_main( void * context ) {
   Jeu ** jeux   = context;
   Jeu *  humain = jeux[0];
   Jeu *  ia     = jeux[1];
   ENTREE( ia->journal );
   Tentatives tentatives;
   if( ! initialiser_le_protocole( ia, false, DELAI_MAX_POUR_CONNECTER_LES_JOUEURS )) {
      ECHEC( ia->journal );
   }
   else {
      reinitialiser( ia );
      memset( tentatives, 0, sizeof( tentatives ));
      while(( humain->etat != Fin_du_programme )&&( ia->etat != Fin_du_programme )) {
         if( ! reflechir( jeux, tentatives )) {
            ia->action = Quitter;
            ia->etat   = Fin_du_programme;
            retCode = 1;
         }
         if(( humain->etat != Fin_du_programme )&&( ia->etat != Fin_du_programme )&&( ia->etat != Attendre_decision_rejouer )) {
            jouer( ia );
         }
      }
   }
   TRACE( ia->journal, "Fin du %s", "thread" );
   return &retCode;
}

BN_API bool initialiser_ia( Jeu ** jeux, const char * chemin_du_journal, pthread_t * thread ) {
   Jeu * jeu = jeux[0];
   Jeu * ia  = malloc( sizeof( Jeu ));
   jeux[1] = ia;
   initialiser( ia );
   ia->est_une_ia = true;
   snprintf( jeu->nom_de_l_adversaire, sizeof( jeu->nom_de_l_adversaire ), ORDINATEUR );
   snprintf( ia ->nom_du_joueur      , sizeof( ia ->nom_du_joueur       ), ORDINATEUR );
   snprintf( ia ->nom_de_l_adversaire, sizeof( ia ->nom_de_l_adversaire ), "%s", jeu->nom_du_joueur );
   if( chemin_du_journal[0] ) {
      char fullpath[PATH_MAX+30];
      snprintf( fullpath, sizeof( fullpath ), "%s/ia.txt", chemin_du_journal );
      ia->journal = fopen( fullpath, "wt" );
   }
   return CHECK_SYS( jeu, pthread_create( thread, NULL, ia_main, jeux ));
}
