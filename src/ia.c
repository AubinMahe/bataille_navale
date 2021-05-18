#include "ia.h"
#include "protocole.h"
#include "traces.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // close()

typedef Etat_torpille Tentatives[COLONNE_MAX][LIGNE_MAX];

static int nombre_aleatoire_entre_zero_et_dix( void ) {
   double value = 10.0 * rand();
   value /= RAND_MAX;
   return (int)value;
}

static bool placer_un_navire( Jeu * ia, Etat prochain ) {
   TRACE( ia->journal, "Navire n°%d", ia->index_navire );
   Navire * navire     = ia->navires + ia->index_navire;
   navire->colonne     = nombre_aleatoire_entre_zero_et_dix();
   navire->ligne       = nombre_aleatoire_entre_zero_et_dix();
   navire->orientation = rand() > RAND_MAX/2;
   unsigned interruption_de_boucle_infinie = 0;
   while( ! controler_le_placement_du_navire( ia )) {
      navire->colonne     = nombre_aleatoire_entre_zero_et_dix();
      navire->ligne       = nombre_aleatoire_entre_zero_et_dix();
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
   for( int c = *colonne; ( c >= 0 )&&( c < 10 ); c += sens ) {
      Etat_torpille e = tentatives[c][ligne];
      if( e == Etat_torpille_Aucun ) {
         *colonne = c;
         TRACE( ia->journal, "L'extrémité n'est pas bordée en {%d, %d}, on le tente.", c, ligne );
         return true;
      }
      if( e == Dans_l_eau ) {
         TRACE( ia->journal, "L'extrémité est bordée (dans l'eau), %s", "il faut chercher ailleurs." );
         return false;
      }
   }
   TRACE( ia->journal, "L'extrémité est bordée (le bord), %s", "il faut chercher ailleurs." );
   return false;
}

static bool rechercher_a_la_verticale( Jeu * ia, Tentatives tentatives, int colonne, int * ligne, int sens ) {
   TRACE( ia->journal, "colonne = %d, ligne = %d", colonne, *ligne);
   for( int l = *ligne; ( l >= 0 )&&( l < 10 ); l += sens ) {
      Etat_torpille e = tentatives[colonne][l];
      if( e == Etat_torpille_Aucun ) {
         *ligne = l;
         TRACE( ia->journal, "L'extrémité n'est pas bordée en {%d, %d}, on le tente.", colonne, l );
         return true;
      }
      if( e == Dans_l_eau ) {
         TRACE( ia->journal, "L'extrémité est bordée (dans l'eau), %s", "il faut chercher ailleurs." );
         return false;
      }
   }
   TRACE( ia->journal, "L'extrémité est bordée (le bord), %s", "il faut chercher ailleurs." );
   return false;
}

static bool placer_une_torpille( Jeu * ia, Tentatives tentatives ) {
   ENTREE( ia->journal );
   Torpille * torpille = ia->torpilles + ia->index_torpille;
   bool trouve  = false;
   for( int i = 0; ( ! trouve )&&( i < ia->index_torpille ); ++i ) {
      Torpille * t = ia->torpilles + i;
      if( t->etat == Touche ) {
         for( int sens = -1; sens < 2; sens += 2 ) {
            if( t->colonne == 0 ) {
               sens = +1;
            }
            if( t->colonne == 9 ) {
               break;
            }
            Etat_torpille voisine = tentatives[t->colonne+sens][t->ligne];
            if( voisine == Touche ) {
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
               if( t->ligne == 9 ) {
                  break;
               }
               Etat_torpille voisine = tentatives[t->colonne][t->ligne+sens];
               if( voisine == Touche ) {
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
   if( ! trouve ) {
      TRACE( ia->journal, "Aucune ligne ne se dessine, on tire au %s", "sort" );
      torpille->colonne = nombre_aleatoire_entre_zero_et_dix();
      torpille->ligne   = nombre_aleatoire_entre_zero_et_dix();
      unsigned interruption_de_boucle_infinie = 0;
      while( ! controler_le_placement_de_la_torpille( ia )) {
         torpille->colonne = nombre_aleatoire_entre_zero_et_dix();
         torpille->ligne   = nombre_aleatoire_entre_zero_et_dix();
         if( ++interruption_de_boucle_infinie > 100 ) {
            return false;
         }
      }
   }
   torpille->etat = Posee;
   int action[] = {
      torpille->colonne,
      torpille->ligne
   };
   if(   envoyer_message( ia, PROTOCOLE_TORPILLE_POSEE, action, sizeof( action ))
      && lire_la_socket_et_le_clavier( ia, PROTOCOLE_DEGATS_OCCASIONNES, &torpille->etat, sizeof( torpille->etat ), 500 ))
   {
      TRACE( ia->journal, "résultat : %s", etat_torpille_texte( torpille->etat ));
      tentatives[torpille->colonne][torpille->ligne] = torpille->etat;
      ++(ia->index_torpille);
      unsigned compteur_de_bateaux_coules = 0;
      for( int i = 0; i < ia->index_torpille; ++i ) {
         if( ia->torpilles[i].etat == Coule ) {
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

void * ia_main( void * context ) {
   Jeu ** jeux   = context;
   Jeu *  humain = jeux[0];
   Jeu *  ia     = jeux[1];
   ENTREE( ia->journal );
   Tentatives tentatives;
   srand((unsigned int)heure_courante_en_ms());
   if( ! initialiser_la_socket( ia )) {
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
#ifdef _WIN32
      closesocket( ia->socket );
#else
      close( ia->socket );
#endif
   }
   fclose( ia->journal );
   return &retCode;
}
