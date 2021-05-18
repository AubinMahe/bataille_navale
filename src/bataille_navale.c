#include "bataille_navale.h"
#include "errors.h"
#include "protocole.h"
#include "traces.h"
#include "utils.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
   Navire   navires[NAVIRE_MAX];
   Torpille torpilles[TORPILLE_MAX];
   int      index_torpille;
} Jeu_de_l_ordinateur;

static void initialiser_navire( const char * nom, int taille, Navire * navire ) {
   snprintf( navire->nom, sizeof( navire->nom ), "%s", nom );
   if( taille <= TAILLE_DU_NAVIRE_LE_PLUS_LONG ) {
      navire->taille = taille;
   }
   else {
      navire->taille = 1;
   }
}

void initialiser( Jeu * jeu ) {
   srand((unsigned)( ((unsigned)getpid()) * heure_courante_en_ms()));
   memset( jeu, 0, sizeof( Jeu ));
   jeu->journal           = stderr;
   jeu->etat              = Debut_du_programme;
   jeu->haut_de_la_grille = 2;
   initialiser_navire( "porte-avion"          , 5, jeu->navires + 0 );
   initialiser_navire( "croiseur"             , 4, jeu->navires + 1 );
   initialiser_navire( "contre-torpilleur n°1", 3, jeu->navires + 2 );
   initialiser_navire( "contre-torpilleur n°2", 3, jeu->navires + 3 );
   initialiser_navire( "torpilleur"           , 2, jeu->navires + 4 );
}

void reinitialiser( Jeu * jeu ) {
   ENTREE( jeu->journal );
   for( int i = 0; i < NAVIRE_MAX; ++i ) {
      memset( jeu->navires[i].degats, 0, sizeof( jeu->navires[i].degats ));
   }
   memset( jeu->navires_adverses, 0, sizeof( jeu->navires_adverses ));
   memset( jeu->torpilles       , 0, sizeof( jeu->torpilles ));
   jeu->etat           = Placement_du_porte_avion;
   jeu->action         = Jouer;
   jeu->index_navire   = 0;
   jeu->index_torpille = 0;
   jeu->partie_gagnee  = false;
}

static chtype valeur_de_la_cellule( const Jeu * jeu, int x, int y, const Navire ** pNavire, const Torpille ** pTorpille ) {
   if( pNavire ) {
      *pNavire   = NULL;
   }
   if( pTorpille ) {
      *pTorpille = NULL;
   }
   for( int i = 0; i < jeu->index_navire; ++i ) {
      const Navire * navire = jeu->navires + i;
      if( navire->orientation == Horizontale ) {
         int nav_y = jeu->haut_de_la_grille + 2 * navire->ligne;
         int min_x = 2 + 4 * navire->colonne;
         int max_x = min_x + 4 * navire->taille - 1;
         if(( nav_y == y )&&( min_x <= x )&&( x < max_x )) {
            if( pNavire ) {
               *pNavire = navire;
            }
            return ACS_CKBOARD;
         }
      }
      else {
         int min_y = jeu->haut_de_la_grille + 2 * navire->ligne;
         int max_y = min_y + 2 * navire->taille - 1;
         int nav_x = 2 + 4 * navire->colonne;
         if(( nav_x == x )&&( min_y <= y )&&( y < max_y )) {
            if( pNavire ) {
               *pNavire = navire;
            }
            return ACS_CKBOARD;
         }
      }
   }
   for( int i = 0; i < jeu->index_torpille; ++i ) {
      const Torpille * torpille = jeu->torpilles + i;
      int tx = 43 + 4 * torpille->colonne;
      int ty = jeu->haut_de_la_grille + 2 * torpille->ligne;
      if(( x == tx )&&( y == ty )) {
         if( pTorpille ) {
            *pTorpille = torpille;
         }
         return ACS_DIAMOND;
      }
   }
   if(( x % 4 ) == 0 ) {
      return ACS_VLINE;
   }
   if(( y % 2 ) == 0 ) {
      return ' ';
   }
   return ACS_HLINE;
}

static void effacer_le_navire( const Jeu * jeu, const Navire * navire ) {
   int y = jeu->haut_de_la_grille + 2 * navire->ligne;
   int x = 2 + 4 * navire->colonne;
   if( navire->orientation == Verticale ) {
      for( int i = 0; i < navire->taille; ++i ) {
         int yy = y + 2*i;
         mvaddch( yy, x, valeur_de_la_cellule( jeu, x, yy, NULL, NULL ));
         if( i != navire->taille - 1 ) {
            ++yy;
            mvaddch( yy, x, valeur_de_la_cellule( jeu, x, yy, NULL, NULL ));
         }
      }
   }
   else {
      for( int i = 0; i < navire->taille; ++i ) {
         int xx = x + 4*i - 1;
         mvaddch( y, xx, valeur_de_la_cellule( jeu, xx, y, NULL, NULL )); ++xx;
         mvaddch( y, xx, valeur_de_la_cellule( jeu, xx, y, NULL, NULL )); ++xx;
         mvaddch( y, xx, valeur_de_la_cellule( jeu, xx, y, NULL, NULL )); ++xx;
         if( i != navire->taille - 1 ) {
            mvaddch( y, xx, valeur_de_la_cellule( jeu, xx, y, NULL, NULL ));
         }
      }
   }
}

static void dessiner_le_navire( const Jeu * jeu, const Navire * navire, bool highlighted, int offset_colonne ) {
   int y = jeu->haut_de_la_grille + 2 * navire->ligne;
   int x = 2 + offset_colonne + 4 * navire->colonne;
   if( highlighted ) {
      attron( COLOR_PAIR( COULEUR_BATEAU_EN_COURS_DE_PLACEMENT ));
   }
   else {
      attron( COLOR_PAIR( COULEUR_BATEAU ));
   }
   if( navire->orientation == Verticale ) {
      for( int i = 0; i < navire->taille; ++i ) {
         if( navire->degats[i] ) {
            attron( COLOR_PAIR( COULEUR_BATEAU_EN_COURS_DE_PLACEMENT ));
         }
         mvaddch( y + 2*i, x, ACS_CKBOARD );
         if( i != navire->taille - 1 ) {
            mvaddch( y + 2*i + 1, x, ACS_CKBOARD );
         }
         if( navire->degats[i] ) {
            attroff( COLOR_PAIR( COULEUR_BATEAU_EN_COURS_DE_PLACEMENT ));
         }
      }
   }
   else {
      for( int i = 0; i < navire->taille; ++i ) {
         mvaddch( y, x + 4*i - 1, ACS_CKBOARD );
         mvaddch( y, x + 4*i + 0, ACS_CKBOARD );
         mvaddch( y, x + 4*i + 1, ACS_CKBOARD );
         if( i != navire->taille - 1 ) {
            mvaddch( y, x + 4*i + 2, ACS_CKBOARD );
         }
      }
   }
   if( highlighted ) {
      attroff( COLOR_PAIR( COULEUR_BATEAU_EN_COURS_DE_PLACEMENT ));
   }
   else {
      attroff( COLOR_PAIR( COULEUR_BATEAU ));
   }
}

static void dessiner_la_torpille( const Jeu * jeu, const Torpille * torpille ) {
   int y = jeu->haut_de_la_grille   + 2 * torpille->ligne;
   int x = 43 + 4 * torpille->colonne;
   switch( torpille->etat ) {
   case Dans_l_eau: attron( COLOR_PAIR( COULEUR_TORPILLE_DANS_L_EAU )); break;
   case Touche    : attron( COLOR_PAIR( COULEUR_TORPILLE_A_TOUCHE ));   break;
   case Coule     : attron( COLOR_PAIR( COULEUR_TORPILLE_A_COULE ));    break;
   default: break;
   }
   mvaddch( y, x, ACS_DIAMOND );
   switch( torpille->etat ) {
   case Dans_l_eau: attroff( COLOR_PAIR( COULEUR_TORPILLE_DANS_L_EAU )); break;
   case Touche    : attroff( COLOR_PAIR( COULEUR_TORPILLE_A_TOUCHE ));   break;
   case Coule     : attroff( COLOR_PAIR( COULEUR_TORPILLE_A_COULE ));    break;
   default: break;
   }
}

static void effacer_la_torpille( const Jeu * jeu, const Torpille * torpille ) {
   int y = jeu->haut_de_la_grille + 2 * torpille->ligne;
   int x = 43 + 4 * torpille->colonne;
   const Torpille * t = NULL;
   if( valeur_de_la_cellule( jeu, x, y, NULL, &t ) == ACS_DIAMOND ) {
      dessiner_la_torpille( jeu, t );
   }
   else {
      mvaddch( y, x, ' ' );
   }
}

static void dessiner_la_grille( int x, int y ) {
   mvaddch( y, x + 0, ACS_ULCORNER );
   for( int i = 0; i < COLONNE_MAX; ++i ) {
      mvaddch( y, x + 4*i+1, ACS_HLINE );
      mvaddch( y, x + 4*i+2, ACS_HLINE );
      mvaddch( y, x + 4*i+3, ACS_HLINE );
      mvaddch( y, x + 4*i+4, ACS_TTEE );
   }
   mvaddch( y, x + 40, ACS_URCORNER );
   ++y;
   for( int r = 0; r < LIGNE_MAX; ++r ) {
      for( int i = 0; i < COLONNE_MAX; ++i ) {
         mvaddch( y, x + 4*i+0, ACS_VLINE );
         mvaddch( y, x + 4*i+1, ' ' );
         mvaddch( y, x + 4*i+2, ' ' );
         mvaddch( y, x + 4*i+3, ' ' );
      }
      mvaddch( y, x + 40, ACS_VLINE );
      ++y;
      mvaddch( y, x + 0, ACS_LTEE );
      for( int i = 0; i < COLONNE_MAX; ++i ) {
         mvaddch( y, x + 4*i+1, ACS_HLINE );
         mvaddch( y, x + 4*i+2, ACS_HLINE );
         mvaddch( y, x + 4*i+3, ACS_HLINE );
         mvaddch( y, x + 4*i+4, ACS_PLUS );
      }
      mvaddch( y, x + 40, ACS_RTEE );
      ++y;
   }
   --y;
   mvaddch( y, x + 0, ACS_LLCORNER );
   for( int i = 0; i < COLONNE_MAX; ++i ) {
      mvaddch( y, x + 4*i+1, ACS_HLINE );
      mvaddch( y, x + 4*i+2, ACS_HLINE );
      mvaddch( y, x + 4*i+3, ACS_HLINE );
      mvaddch( y, x + 4*i+4, ACS_BTEE );
   }
   mvaddch( y, x + 40, ACS_LRCORNER );
}

void afficher( Jeu * jeu ) {
   clear();
   int len = (int)strlen( jeu->nom_du_joueur );
   attron( A_BOLD );
   mvaddstr( 0,  0, "Bataille navale - " );
   mvaddstr( 0, 18, jeu->nom_du_joueur );
   attroff( A_BOLD );
   mvaddstr( 0, 19 + len, "contre" );
   attron( A_BOLD );
   mvaddstr( 0, 26 + len, jeu->nom_de_l_adversaire );
   attroff( A_BOLD );
   char invite1[81] = "";
   char invite2[81] = "";
   move( 22, 0 ); clrtoeol();
   move( 23, 0 ); clrtoeol();
   switch( jeu->etat ) {
   case Placement_du_porte_avion:
   case Placement_du_croiseur:
   case Placement_du_contre_torpilleur_1:
   case Placement_du_contre_torpilleur_2:
   case Placement_du_torpilleur:
      snprintf( invite1, sizeof( invite1 ), "Placer votre %s au moyen des flèches de directions",
         jeu->navires[jeu->index_navire].nom );
      snprintf( invite2, sizeof( invite2 ), "Faites-le tourner au moyen de la barre d'espace" );
      break;
   case Tirage_au_sort:
      snprintf( invite1, sizeof( invite1 ), "Tirage au sort pour savoir lequel va jouer le premier" );
      break;
   case En_attente_de_l_autre_joueur:
      snprintf( invite1, sizeof( invite1 ), "En attente de l'autre joueur" );
      break;
   case Placement_d_une_torpille:
      snprintf( invite1, sizeof( invite1 ), "Placer votre torpille au moyen des flèches de directions" );
      break;
   case Attendre_decision_rejouer:
      snprintf( invite1, sizeof( invite1 ), "Vous avez %s",
         jeu->partie_gagnee ? "gagné ! Bravo !" : "perdu, vous ferez mieux la prochaine fois.");
      snprintf( invite2, sizeof( invite2 ), "[R]ejouer ou [Q]uitter ? " );
      break;
   default:
      snprintf( invite1, sizeof( invite1 ), ">>>>>>>> Etat = %d <<<<<<<<", jeu->etat );
      break;
   }
   mvaddstr( 22, 0, invite1 );
   mvaddstr( 23, 0, invite2 );
   int y = jeu->haut_de_la_grille - 1;
   dessiner_la_grille(  0, y );
   dessiner_la_grille( 41, y );
   for( int i = 0; i < jeu->index_navire; ++i ) {
      dessiner_le_navire( jeu, jeu->navires + i, false, 0 );
   }
   if( jeu->etat != Attendre_decision_rejouer ) {
      for( int i = 0; i < jeu->index_torpille; ++i ) {
         const Torpille * torpille = jeu->torpilles + i;
         dessiner_la_torpille( jeu, torpille );
      }
   }
   else {
      for( int i = 0; i < NAVIRE_MAX; ++i ) {
         dessiner_le_navire( jeu, jeu->navires_adverses + i, false, 40 );
      }
   }
   refresh();
}

static void haut( Jeu * jeu ) {
   if( jeu->etat == Placement_d_une_torpille ) {
      Torpille * torpille = jeu->torpilles + jeu->index_torpille;
      if( torpille->ligne ) {
         --(torpille->ligne);
      }
   }
   else {
      Navire * navire = jeu->navires + jeu->index_navire;
      if( navire->ligne ) {
         --(navire->ligne);
      }
   }
}

static void bas( Jeu * jeu ) {
   if( jeu->etat == Placement_d_une_torpille ) {
      Torpille * torpille = jeu->torpilles + jeu->index_torpille;
      if( torpille->ligne < LIGNE_MAX-1 ) {
         ++(torpille->ligne);
      }
   }
   else {
      Navire * navire = jeu->navires + jeu->index_navire;
      int max = LIGNE_MAX - 1;
      if( navire->orientation == Verticale ) {
         max = LIGNE_MAX - navire->taille;
      }
      if( navire->ligne < max ) {
         ++(navire->ligne);
      }
   }
}

static void gauche( Jeu * jeu ) {
   if( jeu->etat == Placement_d_une_torpille ) {
      Torpille * torpille = jeu->torpilles + jeu->index_torpille;
      if( torpille->colonne ) {
         --(torpille->colonne);
      }
   }
   else {
      Navire * navire = jeu->navires + jeu->index_navire;
      if( navire->colonne ) {
         --(navire->colonne);
      }
   }
}

static void droite( Jeu * jeu ) {
   if( jeu->etat == Placement_d_une_torpille ) {
      Torpille * torpille = jeu->torpilles + jeu->index_torpille;
      if( torpille->colonne < COLONNE_MAX ) {
         ++(torpille->colonne);
      }
   }
   else {
      Navire * navire = jeu->navires + jeu->index_navire;
      int max = COLONNE_MAX - 1;
      if( navire->orientation == Horizontale ) {
         max = COLONNE_MAX - navire->taille;
      }
      if( navire->colonne < max ) {
         ++(navire->colonne);
      }
   }
}

static void changer_l_orientation( Jeu * jeu ) {
   Navire * navire = jeu->navires + jeu->index_navire;
   ++(navire->orientation);
   navire->orientation %= 2;
   int max_x = COLONNE_MAX - 1;
   int max_y = LIGNE_MAX   - 1;
   if( navire->orientation == Horizontale ) {
      max_x = COLONNE_MAX - navire->taille;
   }
   else {
      max_y = LIGNE_MAX - navire->taille;
   }
   if( navire->ligne > max_y ) {
      navire->ligne = max_y;
   }
   if( navire->colonne > max_x ) {
      navire->colonne = max_x;
   }
}

bool une_action_du_joueur_est_attendue( const Jeu * jeu ) {
   return( jeu->etat == Placement_du_porte_avion )
      || ( jeu->etat == Placement_du_croiseur )
      || ( jeu->etat == Placement_du_contre_torpilleur_1 )
      || ( jeu->etat == Placement_du_contre_torpilleur_2 )
      || ( jeu->etat == Placement_du_torpilleur )
      || ( jeu->etat == Placement_d_une_torpille )
      || ( jeu->etat == Attendre_decision_rejouer );
}

void obtenir_la_decision_du_joueur( Jeu * jeu ) {
   ENTREE( jeu->journal );
   jeu->action = Jouer;
   if( une_action_du_joueur_est_attendue( jeu )) {
      flushinp();
      jeu->action = Aucune;
      while( jeu->action == Aucune ) {
         int key = getch();
         switch( jeu->etat ) {
         default: break;
         case Placement_du_porte_avion        : effacer_le_navire  ( jeu, jeu->navires + 0 ); break;
         case Placement_du_croiseur           : effacer_le_navire  ( jeu, jeu->navires + 1 ); break;
         case Placement_du_contre_torpilleur_1: effacer_le_navire  ( jeu, jeu->navires + 2 ); break;
         case Placement_du_contre_torpilleur_2: effacer_le_navire  ( jeu, jeu->navires + 3 ); break;
         case Placement_du_torpilleur         : effacer_le_navire  ( jeu, jeu->navires + 4 ); break;
         case Placement_d_une_torpille        : effacer_la_torpille( jeu, jeu->torpilles + jeu->index_torpille ); break;
         }
         switch( key ) {
         default:
            if( key > -1 ) {
               TRACE( jeu->journal, "touche non gérée : %d", key );
            }
            break;
         case 'Q': case 'q':
         case 27       : jeu->action = Quitter; break;
         case KEY_UP   : haut  ( jeu ); break;
         case KEY_DOWN : bas   ( jeu ); break;
         case KEY_LEFT : gauche( jeu ); break;
         case KEY_RIGHT: droite( jeu ); break;
         case ' '      : changer_l_orientation( jeu ); break;
         case 'R': case 'r':
         case '\n'     : jeu->action = Jouer; break;
         }
         switch( jeu->etat ) {
         default: break;
         case Placement_du_porte_avion        : dessiner_le_navire  ( jeu, jeu->navires + 0, true, 0 ); break;
         case Placement_du_croiseur           : dessiner_le_navire  ( jeu, jeu->navires + 1, true, 0 ); break;
         case Placement_du_contre_torpilleur_1: dessiner_le_navire  ( jeu, jeu->navires + 2, true, 0 ); break;
         case Placement_du_contre_torpilleur_2: dessiner_le_navire  ( jeu, jeu->navires + 3, true, 0 ); break;
         case Placement_du_torpilleur         : dessiner_le_navire  ( jeu, jeu->navires + 4, true, 0 ); break;
         case Placement_d_une_torpille        : dessiner_la_torpille( jeu, jeu->torpilles + jeu->index_torpille ); break;
         }
         refresh();
      }
      TRACE( jeu->journal, "Décision du joueur : %s", action_texte( jeu->action ));
   }
   else {
      TRACE( jeu->journal, "L'état %s n'est pas interactif", etat_texte( jeu->etat ));
   }
}

static bool navire_est_touche( const Navire * navire, int colonne, int ligne, size_t * index_collision ) {
   if( navire->orientation == Horizontale ) {
      if(( navire->ligne == ligne )&&( navire->colonne <= colonne )&&( colonne < navire->colonne + navire->taille )) {
         *index_collision = (size_t)( colonne - navire->colonne );
         return true;
      }
      return false;
   }
   if(( colonne == navire->colonne )&&( navire->ligne <= ligne )&&( ligne < navire->ligne + navire->taille )) {
      if( index_collision ) {
         *index_collision = (size_t)( ligne - navire->ligne );
      }
      return true;
   }
   return false;
}

static bool navire_est_coule( const Navire * navire ) {
   for( int i = 0; i < navire->taille; ++i ) {
      if( navire->degats[i] == false ) {
         return false;
      }
   }
   return true;
}

bool controler_le_placement_du_navire( const Jeu * jeu ) {
   const Navire * navire = jeu->navires + jeu->index_navire;
   TRACE( jeu->journal, "Navire n°%d : {%d, %d}", jeu->index_navire, navire->colonne, navire->ligne );
   for( int i = 0; i < navire->taille; ++i ) {
      int colonne = navire->colonne;
      int ligne   = navire->ligne;
      if( navire->orientation == Horizontale ) {
         colonne += i;
      }
      else {
         ligne += i;
      }
      if(( colonne > 9 )||( ligne > 9 )) {
         return false;
      }
      for( int j = 0; j < jeu->index_navire; ++j ) {
         const Navire * p = jeu->navires + j;
         size_t index_collision = 0;
         if( navire_est_touche( p, colonne, ligne, &index_collision )) {
            ECHEC( jeu->journal );
            return false;
         }
      }
   }
   return true;
}

bool controler_le_placement_de_la_torpille( const Jeu * jeu ) {
   const Torpille * torpille = jeu->torpilles + jeu->index_torpille;
   TRACE( jeu->journal, "Torpille n°%d : {%d, %d}", jeu->index_torpille, torpille->colonne, torpille->ligne );
   for( int i = 0; i < jeu->index_torpille; ++i ) {
      const Torpille * torpille_deja_posee = jeu->torpilles + i;
      if(( torpille_deja_posee->colonne == torpille->colonne )&&( torpille_deja_posee->ligne == torpille->ligne )) {
         return false;
      }
   }
   return true;
}

static void placer_navire( Jeu * jeu, Etat prochain ) {
   ENTREE( jeu->journal );
   if( controler_le_placement_du_navire( jeu )) {
      Navire * navire = jeu->navires + ++(jeu->index_navire);
      navire->colonne     = 0;
      navire->ligne       = 0;
      navire->orientation = Verticale;
      for( int i = 0; i < COLONNE_MAX; ++i ) {
         if( controler_le_placement_du_navire( jeu )) {
            break;
         }
         ++navire->colonne;
      }
      if( navire->colonne == COLONNE_MAX ) {
         navire->colonne = 0;
      }
      jeu->etat = prochain;
   }
   else {
      printf( "\a" );
   }
}

static void initialiser_la_prochaine_torpille( Jeu * jeu ) {
   ENTREE( jeu->journal );
   ++(jeu->index_torpille);
   Torpille * torpille = jeu->torpilles + jeu->index_torpille;
   torpille->etat    = Etat_torpille_Aucun;
   torpille->colonne = jeu->torpilles[jeu->index_torpille-1].colonne;
   torpille->ligne   = jeu->torpilles[jeu->index_torpille-1].ligne;
   for( int i = 0; i < 2; ++i ) {
      while( torpille->ligne < LIGNE_MAX ) {
         while( torpille->colonne < COLONNE_MAX ) {
            if( controler_le_placement_de_la_torpille( jeu )) {
               return;
            }
            ++torpille->colonne;
         }
         torpille->colonne = 0;
         ++torpille->ligne;
      }
      if(( torpille->colonne > COLONNE_MAX-1 )||( torpille->ligne > LIGNE_MAX-1 )) {
         torpille->colonne = 0;
         torpille->ligne   = 0;
      }
      else {
         break;
      }
   }
}

static void placer_torpille( Jeu * jeu ) {
   ENTREE( jeu->journal );
   if( controler_le_placement_de_la_torpille( jeu )) {
      Torpille * torpille = jeu->torpilles + jeu->index_torpille;
      torpille->etat = Posee;
      int action[] = {
         torpille->colonne,
         torpille->ligne
      };
      if(   envoyer_message( jeu, PROTOCOLE_TORPILLE_POSEE, action, sizeof( action ))
         && lire_la_socket_et_le_clavier( jeu, PROTOCOLE_DEGATS_OCCASIONNES, &torpille->etat, sizeof( torpille->etat ), 500 ))
      {
         TRACE( jeu->journal, "résultat : %s", etat_torpille_texte( torpille->etat ));
         initialiser_la_prochaine_torpille( jeu );
         unsigned compteur_de_bateaux_coules = 0;
         for( int i = 0; i < jeu->index_torpille; ++i ) {
            if( jeu->torpilles[i].etat == Coule ) {
               ++compteur_de_bateaux_coules;
            }
         }
         jeu->action = Jouer;
         if( compteur_de_bateaux_coules == NAVIRE_MAX ) {
            jeu->partie_gagnee = true;
            jeu->etat          = Partie_achevee;
         }
         else {
            jeu->etat = En_attente_de_l_autre_joueur;
         }
      }
      else {
         ECHEC( jeu->journal );
         jeu->action = Quitter;
         jeu->etat   = Fin_du_programme;
      }
   }
   else {
      printf( "\a" );
   }
}

static void tirer_au_sort_le_premier_a_jouer( Jeu * jeu ) {
   ENTREE( jeu->journal );
   int valeur_locale   = 0;
   int valeur_distante = 0;
   while( valeur_locale == valeur_distante ) {
      valeur_locale = rand();
      if( ! requete_reponse( jeu, PROTOCOLE_TIRAGE_AU_SORT,
         &valeur_locale  , sizeof( valeur_locale   ),
         &valeur_distante, sizeof( valeur_distante ),
         DELAI_MAX_POUR_TIRER_AU_SORT_LE_PREMIER_A_JOUER ))
      {
         jeu->action = Quitter;
         jeu->etat   = Fin_du_programme;
         ECHEC( jeu->journal );
         return;
      }
   }
   TRACE( jeu->journal, "%s", ( valeur_locale > valeur_distante ) ? "c'est moi" : "c'est lui" );
   jeu->action = Jouer;
   if( valeur_locale < valeur_distante ) {
      jeu->etat = En_attente_de_l_autre_joueur;
   }
   else {
      jeu->etat = Placement_d_une_torpille;
   }
}

static Etat_torpille evaluation_du_coup_adverse( Jeu * jeu, int colonne, int ligne ) {
   Etat_torpille resultat = Dans_l_eau;
   for( int i = 0; ( resultat == Dans_l_eau )&&( i < jeu->index_navire ); ++i ) {
      Navire * navire = jeu->navires + i;
      size_t index_collision = 0;
      if( navire_est_touche( navire, colonne, ligne, &index_collision )) {
         navire->degats[index_collision] = true;
         if( navire_est_coule( navire )) {
            resultat = Coule;
         }
         else {
            resultat = Touche;
         }
      }
   }
   TRACE( jeu->journal, "résultat: %s", etat_torpille_texte( resultat ));
   return resultat;
}

static void attendre_le_coup_de_l_autre_joueur( Jeu * jeu ) {
   ENTREE( jeu->journal );
   int action[2];
   if( ! lire_la_socket_et_le_clavier( jeu,
            PROTOCOLE_TORPILLE_POSEE, action, sizeof( action ), DELAI_MAX_POUR_PLACER_UNE_TORPILLE )) {
      ECHEC( jeu->journal );
      return;
   }
   Etat_torpille resultat = evaluation_du_coup_adverse( jeu, action[0], action[1] );
   TRACE( jeu->journal, "résultat : %s", etat_torpille_texte( resultat ));
   if( envoyer_message( jeu, PROTOCOLE_DEGATS_OCCASIONNES, &resultat, sizeof( resultat ))) {
      unsigned coules = 0;
      for( int i = 0; i < NAVIRE_MAX; ++i ) {
         Navire * navire = jeu->navires + i;
         if( navire_est_coule( navire )) {
            ++coules;
         }
      }
      jeu->action = Jouer;
      if( coules == NAVIRE_MAX ) {
         jeu->partie_gagnee = false;
         jeu->etat          = Partie_achevee;
      }
      else {
         jeu->etat = Placement_d_une_torpille;
      }
   }
   TRACE( jeu->journal, "Nouvel état : %s", etat_texte( jeu->etat ));
}

void jouer( Jeu * jeu ) {
   TRACE( jeu->journal, "état d'entrée : %s, action: %s", etat_texte( jeu->etat ), action_texte( jeu->action ));
   switch( jeu->action ) {
   case Jouer:
      switch( jeu->etat ) {
      case Placement_du_porte_avion        : placer_navire( jeu, Placement_du_croiseur );            break;
      case Placement_du_croiseur           : placer_navire( jeu, Placement_du_contre_torpilleur_1 ); break;
      case Placement_du_contre_torpilleur_1: placer_navire( jeu, Placement_du_contre_torpilleur_2 ); break;
      case Placement_du_contre_torpilleur_2: placer_navire( jeu, Placement_du_torpilleur );          break;
      case Placement_du_torpilleur         : placer_navire( jeu, Tirage_au_sort );                   break;
      case Tirage_au_sort                  : tirer_au_sort_le_premier_a_jouer( jeu );                break;
      case En_attente_de_l_autre_joueur    : attendre_le_coup_de_l_autre_joueur( jeu );              break;
      case Placement_d_une_torpille        : placer_torpille( jeu );                                 break;
      case Partie_achevee:
         requete_reponse( jeu,
            PROTOCOLE_PARTIE_ACHEVEE,
            jeu->navires         , sizeof( jeu->navires ),
            jeu->navires_adverses, sizeof( jeu->navires_adverses ),
            DELAI_MAX_POUR_ENVOYER_LES_BATEAUX );
         jeu->etat = Attendre_decision_rejouer;
         break;
      case Attendre_decision_rejouer: reinitialiser( jeu ); break;
      default: break;
      }
      break;
   case Quitter: jeu->etat = Fin_du_programme; break;
   default:
      break;
   }
   TRACE( jeu->journal, "état de sortie : %s", etat_texte( jeu->etat ));
}

void liberer_les_ressources( Jeu * jeu ) {
   ENTREE( jeu->journal );
   clear();
   nocbreak();
   endwin();
#ifdef _WIN32
   closesocket( jeu->socket );
#else
   close( jeu->socket );
#endif
   jeu->socket = 1;
//   dump( jeu );
   if( jeu->journal != stderr ) {
      fclose( jeu->journal );
   }
}
