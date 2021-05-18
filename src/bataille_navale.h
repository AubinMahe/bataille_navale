#pragma once

#include "portability.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

#define COLONNE_MAX     10
#define LIGNE_MAX       10
#define NAVIRE_MAX      5
#define TORPILLE_MAX    (COLONNE_MAX*LIGNE_MAX)
#define ADVERSAIRE_MAX  (4*4+5+1+4*4+5+1)
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
} Etat;

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

typedef struct {
   char        nom[25];
   Orientation orientation;
   int         ligne;
   int         colonne;
   int         taille;
   bool        degats[TAILLE_DU_NAVIRE_LE_PLUS_LONG];
} Navire;

typedef enum {
   Etat_torpille_Aucun,
   Posee,
   Dans_l_eau,
   Touche,
   Coule
} Etat_torpille;

typedef struct {
   int           ligne;
   int           colonne;
   Etat_torpille etat;
} Torpille;

typedef struct {
   FILE *   journal;
   Etat     etat;
   char     nom_du_joueur[21];
   char     nom_de_l_adversaire[ADVERSAIRE_MAX]; // 192.168.1.10:2416/192.168.1.40:2417
   bool     est_une_ia;
   socket_t socket;
   Action   action;
   int      haut_de_la_grille;
   Navire   navires[NAVIRE_MAX];
   int      index_navire;
   Torpille torpilles[TORPILLE_MAX];
   int      index_torpille;
   Navire   navires_adverses[NAVIRE_MAX];
   bool     partie_gagnee;
} Jeu;

#define SECONDE                                          1000
#define DELAI_MAX_POUR_CONNECTER_LES_JOUEURS             ( 30 * SECONDE )
#define DELAI_MAX_POUR_TIRER_AU_SORT_LE_PREMIER_A_JOUER  ( 60 * SECONDE )
#define DELAI_MAX_POUR_PLACER_UNE_TORPILLE               ( 30 * SECONDE )
#define DELAI_MAX_POUR_ENVOYER_LES_BATEAUX               500

#define COULEUR_BATEAU                          1
#define COULEUR_BATEAU_EN_COURS_DE_PLACEMENT    2
#define COULEUR_TORPILLE_DANS_L_EAU             3
#define COULEUR_TORPILLE_A_TOUCHE               4
#define COULEUR_TORPILLE_A_COULE                5

void initialiser( Jeu * jeu );
void reinitialiser( Jeu * jeu );
void afficher( Jeu * jeu );
bool une_action_du_joueur_est_attendue( const Jeu * jeu );
void obtenir_la_decision_du_joueur( Jeu * jeu );
bool controler_le_placement_du_navire( const Jeu * jeu );
bool controler_le_placement_de_la_torpille( const Jeu * jeu );
void jouer( Jeu * jeu );
void liberer_les_ressources( Jeu * jeu );
