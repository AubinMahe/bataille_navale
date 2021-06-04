#include "ihm.h"

#include "traces.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//https://docs.microsoft.com/en-us/windows/console/console-functions

#include <signal.h>
#include <string.h>

#define KEY_UP    72
#define KEY_LEFT  75
#define KEY_RIGHT 77
#define KEY_DOWN  80

const char COIN_HAUT_GAUCHE = (char)218;
const char COIN_HAUT_DROITE = (char)191;
const char COIN_BAS_GAUCHE  = (char)192;
const char COIN_BAS_DROITE  = (char)217;
const char TORPILLE         = (char)207;
const char BATEAU           = (char)219;
const char LIGNE_HORIZ      = (char)196;
const char LIGNE_VERTI      = (char)179;
const char CROIX            = (char)197;
const char T_HAUT           = (char)194;
const char T_GAUCHE         = (char)195;
const char T_DROITE         = (char)180;
const char T_BAS            = (char)193;

const WORD BLANC = ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );
const WORD JAUNE = ( FOREGROUND_RED | FOREGROUND_GREEN );
const WORD COULEUR_TORPILLE_NON_PLACEE = ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );
const WORD COULEUR_TORPILLE_DANS_L_EAU = FOREGROUND_BLUE;
const WORD COULEUR_TORPILLE_A_TOUCHE   = ( FOREGROUND_RED | FOREGROUND_GREEN );
const WORD COULEUR_TORPILLE_A_COULE    = FOREGROUND_RED;
const WORD COULEUR_BATEAU_NON_PLACE    = ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );
const WORD COULEUR_BATEAU_PLACE        = FOREGROUND_BLUE;
const WORD COULEUR_BATEAU_TOUCHE       = ( FOREGROUND_RED | FOREGROUND_GREEN );
const WORD COULEUR_BATEAU_COULE        = FOREGROUND_RED;

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

static HANDLE      hConsoleScreenBuffer;
static const COORD origine = { 0, 0 };
static const int   largeur_de_la_console = 84;
static const int   hauteur_de_la_console = 25;

static bool error( Jeu * jeu, int line, const char * function, BOOL result, const char * call ) {
   if( ! result ) {
      DWORD lastError = GetLastError();
      char buffer[1024];
      FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastError, 0, buffer, sizeof( buffer ), NULL );
      ajouter_une_entree_au_journal( jeu->journal, __FILE__, line, function, "%s: %s", call, buffer );
      jeu->action = Quitter;
      jeu->etat   = Fin_du_programme;
      return false;
   }
   return true;
}

#define CHECK_BOOL(J,C)    error( J, __LINE__, __func__, C, #C )
#define CHECK_INVALID(J,C) error( J, __LINE__, __func__, (C) != INVALID_HANDLE_VALUE, #C )

static void effacer( Jeu * jeu ) {
   DWORD numberOfCharsWritten = 0;
   CHECK_BOOL( jeu,
      FillConsoleOutputCharacter(
         hConsoleScreenBuffer,
         ' ',
         (DWORD)( largeur_de_la_console * hauteur_de_la_console ),
         origine,
         &numberOfCharsWritten ));
}

static void ecrire( Jeu * jeu, int x, int y, WORD attribute, const char* message, ... ) {
   va_list args;
   va_start( args, message );
   char buffer[100];
   vsnprintf( buffer, sizeof( buffer ), message, args );
   va_end( args );
   COORD coord = { .X = (short)x, .Y = (short)y };
   DWORD numberOfCharsWritten = 0;
   DWORD len = (DWORD)strlen( buffer );
   CHECK_BOOL( jeu, WriteConsoleOutputCharacter( hConsoleScreenBuffer, buffer, len, coord, &numberOfCharsWritten ));
   for( DWORD l = 0; l < len; ++l ) {
      CHECK_BOOL( jeu, WriteConsoleOutputAttribute( hConsoleScreenBuffer, &attribute, 1, coord, &numberOfCharsWritten ));
      coord.X++;
   }
}

static void ecrirec( Jeu * jeu, int x, int y, WORD attribute, char c ) {
   DWORD numberOfCharsWritten = 0;
   COORD coord = { .X = (short)x, .Y = (short)y };
   char buffer[] = { c, '\0' };
   CHECK_BOOL( jeu, WriteConsoleOutputCharacter( hConsoleScreenBuffer, buffer, 1, coord, &numberOfCharsWritten ));
   CHECK_BOOL( jeu, WriteConsoleOutputAttribute( hConsoleScreenBuffer, &attribute, 1, coord, &numberOfCharsWritten ));
}

static void repeter( Jeu * jeu, int x, int y, WORD attribute, char c, DWORD nb ) {
   DWORD numberOfCharsWritten = 0;
   COORD coord = { .X = (short)x, .Y = (short)y };
   CHECK_BOOL( jeu, FillConsoleOutputCharacter( hConsoleScreenBuffer, c        , nb, coord, &numberOfCharsWritten ));
   CHECK_BOOL( jeu, FillConsoleOutputAttribute( hConsoleScreenBuffer, attribute, nb, coord, &numberOfCharsWritten ));
}

static void centrer( Jeu * jeu, int y, WORD attribute, const char* message ) {
   int x = ( largeur_de_la_console - (int)strlen( message )) / 2;
   ecrire( jeu, x, y, attribute, message );
}

static int lire_le_clavier( void ) {
   if( _kbhit()) {
      int key = _getch();
      if( key == 224 ) {
         key = _getch();
      }
      return key;
   }
   return -1;
}

static bool echap_est_presse( void ) {
   return lire_le_clavier() == VK_ESCAPE;
}

bool initialiser_l_ihm( Jeu ** jeux ) {
   Jeu * jeu = jeux[0];
   ENTREE( jeu->journal );
   ctrl_c_handler_context = jeux;
   signal( SIGINT, &ctrl_c_handler );
   jeu->interruption = echap_est_presse;
   if(   CHECK_INVALID( jeu, hConsoleScreenBuffer =
                                CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL ))
      && CHECK_BOOL( jeu, SetConsoleActiveScreenBuffer( hConsoleScreenBuffer )))
   {
      effacer( jeu );
      ecrire( jeu, 0, 0, JAUNE, "Bataille navale" );
      centrer( jeu, 11, JAUNE, "-#-  En attente de l'adversaire...  -#-" );
      return true;
   }
   return false;
}

static char valeur_de_la_cellule( const Jeu * jeu, int x, int y, const Navire ** pNavire, const Torpille ** pTorpille ) {
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
            return BATEAU;
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
            return BATEAU;
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
         return TORPILLE;
      }
   }
   if(( x % 4 ) == 0 ) {
      return LIGNE_VERTI;
   }
   if(( y % 2 ) == 0 ) {
      return ' ';
   }
   return LIGNE_HORIZ;
}

static WORD couleur_navire( const Navire * navire, int index ) {
   switch( navire->etat ) {
   default:
   case en_Aucun : return COULEUR_BATEAU_NON_PLACE;
   case en_Place : return COULEUR_BATEAU_PLACE;
   case en_Touche: return navire->degats[index] ? COULEUR_BATEAU_TOUCHE : COULEUR_BATEAU_PLACE;
   case en_Coule : return COULEUR_BATEAU_COULE;
   }
}

static void efface_cellule( Jeu * jeu, int x, int yy ) {
   char v = valeur_de_la_cellule( jeu, x, yy, NULL, NULL );
   if( v == BATEAU ) {
      ecrirec( jeu, x, yy, COULEUR_BATEAU_PLACE, v );
   }
}

static void effacer_le_navire( Jeu * jeu, const Navire * navire ) {
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

static void dessiner_le_navire( Jeu * jeu, const Navire * navire, int offset_colonne ) {
   int y = jeu->haut_de_la_grille + 2 * navire->ligne;
   int x = 2 + offset_colonne + 4 * navire->colonne;
   if( navire->orientation == Verticale ) {
      for( int i = 0; i < navire->taille; ++i ) {
         WORD couleur = couleur_navire( navire, i );
         ecrirec( jeu, x, y + 2*i, couleur, BATEAU );
         if( i != navire->taille - 1 ) {
            ecrirec( jeu, x, y + 2*i + 1, couleur, BATEAU );
         }
      }
   }
   else {
      for( int i = 0; i < navire->taille; ++i ) {
         WORD couleur = couleur_navire( navire, i );
         if( i ) {
            ecrirec( jeu, x + 4*i - 1, y, couleur, BATEAU );
         }
         ecrirec( jeu, x + 4*i + 0, y, couleur, BATEAU );
         if( i != navire->taille - 1 ) {
            ecrirec( jeu, x + 4*i + 1, y, couleur, BATEAU );
            ecrirec( jeu, x + 4*i + 2, y, couleur, BATEAU );
         }
      }
   }
}

static void dessiner_la_torpille( Jeu * jeu, const Torpille * torpille ) {
   int x = 43 + 4 * torpille->colonne;
   int y = jeu->haut_de_la_grille + 2 * torpille->ligne;
   switch( torpille->etat ) {
   case et_Dans_l_eau: ecrirec( jeu, x, y, COULEUR_TORPILLE_DANS_L_EAU, TORPILLE ); break;
   case et_Touche    : ecrirec( jeu, x, y, COULEUR_TORPILLE_A_TOUCHE, TORPILLE ); break;
   case et_Coule     : ecrirec( jeu, x, y, COULEUR_TORPILLE_A_COULE, TORPILLE ); break;
   default: break;
   }
}

static void effacer_la_torpille( Jeu * jeu, const Torpille * torpille ) {
   int x = 43 + 4 * torpille->colonne;
   int y = jeu->haut_de_la_grille + 2 * torpille->ligne;
   const Torpille * t = NULL;
   if( valeur_de_la_cellule( jeu, x, y, NULL, &t ) == TORPILLE ) {
      dessiner_la_torpille( jeu, t );
   }
   else {
      ecrirec( jeu, x, y, BLANC, ' ' );
   }
}

static void dessiner_la_grille( Jeu * jeu ) {
   int y = jeu->haut_de_la_grille - 1;
   ecrirec( jeu, 0, y, BLANC, COIN_HAUT_GAUCHE );
   for( int j = 0; j < 2 * COLONNE_MAX; ++j ) {
      int x = 4 * j;
      repeter( jeu, x + 1, y, BLANC, LIGNE_HORIZ, 3 );
      if( j == 2 * COLONNE_MAX - 1 ) {
         ecrirec( jeu, x + 4, y, BLANC, COIN_HAUT_DROITE );
      }
      else {
         ecrirec( jeu, x + 4, y, BLANC, T_HAUT );
      }
   }
   for( int i = 0; i < LIGNE_MAX; ++i ) {
      y = 2 * i + jeu->haut_de_la_grille;
      for( int j = 0; j < 2 * COLONNE_MAX; ++j ) {
         int x = 4 * j;
         ecrirec( jeu, x, y, BLANC, LIGNE_VERTI );
         repeter( jeu, x + 1, y + 1, BLANC, LIGNE_HORIZ, 3 );
         if( j == 0 ) {
            if( i == LIGNE_MAX - 1 ) {
               ecrirec( jeu, x, y + 1, BLANC, COIN_BAS_GAUCHE );
            }
            else {
               ecrirec( jeu, x, y + 1, BLANC, T_GAUCHE );
            }
         }
         else if( j == 2 * COLONNE_MAX - 1 ) {
            ecrirec( jeu, x + 4, y, BLANC, LIGNE_VERTI );
            if( i == LIGNE_MAX - 1 ) {
               ecrirec( jeu, x + 4, y + 1, BLANC, COIN_BAS_DROITE );
            }
            else {
               ecrirec( jeu, x + 4, y + 1, BLANC, T_DROITE );
            }
         }
         else {
            if( i == LIGNE_MAX - 1 ) {
               ecrirec( jeu, x, y + 1, BLANC, T_BAS );
            }
            else {
               ecrirec( jeu, x, y + 1, BLANC, CROIX );
            }
         }
      }
      if( i == LIGNE_MAX - 1 ) {
         ecrirec( jeu, 2 * 4 * COLONNE_MAX - 4, y + 1, BLANC, T_BAS );
      }
      else {
         ecrirec( jeu, 2 * 4 * COLONNE_MAX - 4, y + 1, BLANC, CROIX );
      }
   }
}

void afficher( Jeu * jeu ) {
   effacer( jeu );
   int len = (int)strlen( jeu->nom_du_joueur );
   ecrire( jeu,  0      , 0, JAUNE|FOREGROUND_INTENSITY, "Bataille navale - " );
   ecrire( jeu, 18      , 0, JAUNE|FOREGROUND_INTENSITY, jeu->nom_du_joueur );
   ecrire( jeu, 19 + len, 0, JAUNE, "contre" );
   ecrire( jeu, 26 + len, 0, JAUNE|FOREGROUND_INTENSITY, jeu->nom_de_l_adversaire );
   char invite1[81] = "";
   char invite2[81] = "";
   repeter( jeu, 0, 22, BLANC, ' ', (DWORD)largeur_de_la_console );
   repeter( jeu, 0, 23, BLANC, ' ', (DWORD)largeur_de_la_console );
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
   ecrire( jeu, 0, 22, BLANC, invite1 );
   ecrire( jeu, 0, 23, BLANC, invite2 );
   dessiner_la_grille( jeu );
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
      jeu->action = Aucune;
      while( jeu->action == Aucune ) {
         int key = lire_le_clavier();
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
      }
      TRACE( jeu->journal, "Décision du joueur : %s", action_texte( jeu->action ));
   }
   else {
      TRACE( jeu->journal, "L'état '%s' n'est pas interactif", etat_jeu_texte( jeu->etat ));
   }
}

void liberer_les_ressources_ihm( Jeu * jeu ) {
   ENTREE( jeu->journal );
//   CloseHandle( consoleScreenBuffer );
}
