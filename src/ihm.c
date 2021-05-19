#include "ihm.h"

#include "traces.h"

#include <ncurses.h>
#include <signal.h>
#include <string.h>

#define TOUCHE_ECHAP 27

#define COULEUR_BATEAU_NON_PLACE    1
#define COULEUR_BATEAU_PLACE        2
#define COULEUR_BATEAU_TOUCHE       3
#define COULEUR_BATEAU_COULE        4
#define COULEUR_TORPILLE_DANS_L_EAU 5
#define COULEUR_TORPILLE_A_TOUCHE   6
#define COULEUR_TORPILLE_A_COULE    7

static Jeu ** ctrl_c_handler_context;

static void ctrl_c_handler( int num ) {
   for( size_t i = 0; i < 2; ++i ) {
      Jeu * jeu = ctrl_c_handler_context[i];
      if( jeu ) {
         jeu->action = Quitter;
         jeu->etat   = Fin_du_programme;
      }
   }
   (void)num;
}

static bool echap_est_presse( void ) {
   return getch() == TOUCHE_ECHAP;
}

void initialiser_l_ihm( Jeu ** jeux ) {
   ENTREE( jeux[0]->journal );
   ctrl_c_handler_context = jeux;
   signal( SIGINT, &ctrl_c_handler );
   jeux[0]->interruption = echap_est_presse;
   initscr();
   cbreak();
   noecho();
   halfdelay( 1 );
   keypad( stdscr, TRUE );
   curs_set( 0 );
   start_color();
   init_pair( COULEUR_BATEAU_NON_PLACE   , COLOR_WHITE , COLOR_BLACK );
   init_pair( COULEUR_BATEAU_PLACE       , COLOR_BLUE  , COLOR_BLACK );
   init_pair( COULEUR_BATEAU_TOUCHE      , COLOR_YELLOW, COLOR_BLACK );
   init_pair( COULEUR_BATEAU_COULE       , COLOR_RED   , COLOR_BLACK );
   init_pair( COULEUR_TORPILLE_DANS_L_EAU, COLOR_BLUE  , COLOR_BLACK );
   init_pair( COULEUR_TORPILLE_A_TOUCHE  , COLOR_YELLOW, COLOR_BLACK );
   init_pair( COULEUR_TORPILLE_A_COULE   , COLOR_RED   , COLOR_BLACK );
   clear();
   attron( A_BOLD );
   const char * message = "-#-  En attente de l'adversaire...  -#-";
   mvaddstr( 0, 0, "Bataille navale" );
   mvaddstr( 11, (84 - (int)strlen( message )) / 2, message );
   attroff( A_BOLD );
   refresh();
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

static int couleur_navire( const Navire * navire, int index ) {
   switch( navire->etat ) {
   default:
   case en_Aucun : return COULEUR_BATEAU_NON_PLACE;
   case en_Place : return COULEUR_BATEAU_PLACE;
   case en_Touche: return navire->degats[index] ? COULEUR_BATEAU_TOUCHE : COULEUR_BATEAU_PLACE;
   case en_Coule : return COULEUR_BATEAU_COULE;
   }
}

static void efface_cellule( const Jeu * jeu, int x, int yy ) {
   chtype ch = valeur_de_la_cellule( jeu, x, yy, NULL, NULL );
   if( ch == ACS_CKBOARD ) {
      attron( COLOR_PAIR( COULEUR_BATEAU_PLACE ));
   }
   mvaddch( yy, x, ch );
   if( ch == ACS_CKBOARD ) {
      attroff( COLOR_PAIR( COULEUR_BATEAU_PLACE ));
   }
}

static void effacer_le_navire( const Jeu * jeu, const Navire * navire ) {
   int y = jeu->haut_de_la_grille + 2 * navire->ligne;
   int x = 2 + 4 * navire->colonne;
   if( navire->orientation == Verticale ) {
      for( int i = 0; i < navire->taille; ++i ) {
         int yy = y + 2*i;
         efface_cellule( jeu, x, yy );
         if( i != navire->taille - 1 ) {
            efface_cellule( jeu, x, ++yy );
         }
      }
   }
   else {
      for( int i = 0; i < navire->taille; ++i ) {
         int xx = x + 4*i - 1;
         efface_cellule( jeu, xx++, y );
         efface_cellule( jeu, xx++, y );
         efface_cellule( jeu, xx++, y );
         if( i != navire->taille - 1 ) {
            efface_cellule( jeu, xx, y );
         }
      }
   }
}

static void dessiner_le_navire( const Jeu * jeu, const Navire * navire, int offset_colonne ) {
   int y = jeu->haut_de_la_grille + 2 * navire->ligne;
   int x = 2 + offset_colonne + 4 * navire->colonne;
   if( navire->orientation == Verticale ) {
      for( int i = 0; i < navire->taille; ++i ) {
         int couleur = couleur_navire( navire, i );
         attron( COLOR_PAIR( couleur ));
         mvaddch( y + 2*i, x, ACS_CKBOARD );
         if( i != navire->taille - 1 ) {
            mvaddch( y + 2*i + 1, x, ACS_CKBOARD );
         }
         attroff( COLOR_PAIR( couleur ));
      }
   }
   else {
      for( int i = 0; i < navire->taille; ++i ) {
         int couleur = couleur_navire( navire, i );
         attron( COLOR_PAIR( couleur ));
         if( i ) {
            mvaddch( y, x + 4*i - 1, ACS_CKBOARD );
         }
         mvaddch( y, x + 4*i + 0, ACS_CKBOARD );
         if( i != navire->taille - 1 ) {
            mvaddch( y, x + 4*i + 1, ACS_CKBOARD );
            mvaddch( y, x + 4*i + 2, ACS_CKBOARD );
         }
         attroff( COLOR_PAIR( couleur ));
      }
   }
}

static void dessiner_la_torpille( const Jeu * jeu, const Torpille * torpille ) {
   int y = jeu->haut_de_la_grille   + 2 * torpille->ligne;
   int x = 43 + 4 * torpille->colonne;
   switch( torpille->etat ) {
   case et_Dans_l_eau: attron( COLOR_PAIR( COULEUR_TORPILLE_DANS_L_EAU )); break;
   case et_Touche    : attron( COLOR_PAIR( COULEUR_TORPILLE_A_TOUCHE ));   break;
   case et_Coule     : attron( COLOR_PAIR( COULEUR_TORPILLE_A_COULE ));    break;
   default: break;
   }
   mvaddch( y, x, ACS_DIAMOND );
   switch( torpille->etat ) {
   case et_Dans_l_eau: attroff( COLOR_PAIR( COULEUR_TORPILLE_DANS_L_EAU )); break;
   case et_Touche    : attroff( COLOR_PAIR( COULEUR_TORPILLE_A_TOUCHE ));   break;
   case et_Coule     : attroff( COLOR_PAIR( COULEUR_TORPILLE_A_COULE ));    break;
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
      dessiner_le_navire( jeu, jeu->navires + i, 0 );
   }
   if( jeu->etat != Attendre_decision_rejouer ) {
      for( int i = 0; i < jeu->index_torpille; ++i ) {
         const Torpille * torpille = jeu->torpilles + i;
         dessiner_la_torpille( jeu, torpille );
      }
   }
   else {
      for( int i = 0; i < NAVIRE_MAX; ++i ) {
         dessiner_le_navire( jeu, jeu->navires_adverses + i, 41 );
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
         case Placement_du_porte_avion        : dessiner_le_navire  ( jeu, jeu->navires + 0, 0 ); break;
         case Placement_du_croiseur           : dessiner_le_navire  ( jeu, jeu->navires + 1, 0 ); break;
         case Placement_du_contre_torpilleur_1: dessiner_le_navire  ( jeu, jeu->navires + 2, 0 ); break;
         case Placement_du_contre_torpilleur_2: dessiner_le_navire  ( jeu, jeu->navires + 3, 0 ); break;
         case Placement_du_torpilleur         : dessiner_le_navire  ( jeu, jeu->navires + 4, 0 ); break;
         case Placement_d_une_torpille        : dessiner_la_torpille( jeu, jeu->torpilles + jeu->index_torpille ); break;
         }
         refresh();
      }
      TRACE( jeu->journal, "Décision du joueur : %s", action_texte( jeu->action ));
   }
   else {
      TRACE( jeu->journal, "L'état '%s' n'est pas interactif", etat_jeu_texte( jeu->etat ));
   }
}

void liberer_les_ressources_ihm( Jeu * jeu ) {
   ENTREE( jeu->journal );
   clear();
   nocbreak();
   endwin();
}
