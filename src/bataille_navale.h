#pragma once

#include "portability.h"

#include <stdbool.h>
#include <stdio.h>      // FILE

#define COLONNE_MAX     10
#define LIGNE_MAX       10
#define NAVIRE_MAX      5
#define TORPILLE_MAX    (COLONNE_MAX*LIGNE_MAX)
#define ADVERSAIRE_MAX  (3+1+3+1+3+1+3+1+5+1+3+1+3+1+3+1+3+1+5+1) //!< IP:PORT/IP:PORT
#define TAILLE_DU_NAVIRE_LE_PLUS_LONG  5

typedef enum {
   Debut_du_programme,
   Placement_du_porte_avion,
   Placement_du_croiseur,
   Placement_du_contre_torpilleur_1,
   Placement_du_contre_torpilleur_2,
   Placement_du_torpilleur,
   Tirage_au_sort,
   En_attente_de_l_autre_joueur,
   Placement_d_une_torpille,
   Partie_achevee,
   Attendre_decision_rejouer,
   Fin_du_programme
} Etat_du_jeu;

typedef enum {
   Aucune,
   Jouer,
   Rejouer,
   Quitter
} Action;

typedef enum {
   Verticale,
   Horizontale
} Orientation;

typedef enum {
   en_Aucun,
   en_Place,
   en_Touche,
   en_Coule
} Etat_navire;

typedef struct {
   char        nom[25];
   Orientation orientation;
   int         ligne;
   int         colonne;
   int         taille;
   bool        degats[TAILLE_DU_NAVIRE_LE_PLUS_LONG];
   Etat_navire etat;
} Navire;

typedef enum {
   et_Aucun,
   et_Posee,
   et_Dans_l_eau,
   et_Touche,
   et_Coule
} Etat_torpille;

typedef struct {
   int           ligne;
   int           colonne;
   Etat_torpille etat;
} Torpille;

typedef struct {
   FILE *      journal;
   Etat_du_jeu etat;
   char        nom_du_joueur[21];
   char        nom_de_l_adversaire[ADVERSAIRE_MAX];
   bool        est_une_ia;
   bool ( *    interruption )( void ); //!< Valide uniquement si est_une_ia = false
   socket_t    socket;
   Action      action;
   int         haut_de_la_grille;
   Navire      navires[NAVIRE_MAX];
   int         index_navire;
   Torpille    torpilles[TORPILLE_MAX];
   int         index_torpille;
   Navire      navires_adverses[NAVIRE_MAX];
   bool        partie_gagnee;
} Jeu;

BN_API void initialiser( Jeu * jeu );
BN_API void initialiser_le_journal( Jeu * jeu, const char * chemin_du_journal );
BN_API void reinitialiser( Jeu * jeu );
BN_API bool une_action_du_joueur_est_attendue( const Jeu * jeu );
BN_API void jouer( Jeu * jeu );
BN_API void liberer_les_ressources( Jeu * jeu );

bool controler_le_placement_du_navire( const Jeu * jeu );
bool controler_le_placement_de_la_torpille( const Jeu * jeu );
