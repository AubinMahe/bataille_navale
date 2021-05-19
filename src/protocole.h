#pragma once

#include "bataille_navale.h"

#include <stddef.h>

typedef enum {
   PROTOCOLE_AUCUN,
   PROTOCOLE_POIGNEE_DE_MAIN,
   PROTOCOLE_TIRAGE_AU_SORT,
   PROTOCOLE_TORPILLE_POSEE,
   PROTOCOLE_DEGATS_OCCASIONNES,
   PROTOCOLE_PARTIE_ACHEVEE,
} Entete;

BN_API bool initialiser_le_protocole( Jeu * jeu, bool l_adversaire_est_l_ordinateur, unsigned delai_max );
BN_API void liberer_les_ressources_reseau( Jeu ** jeux );

bool envoyer_message( Jeu * jeu, Entete entete, const void * corps, size_t taille_du_corps );
bool requete_reponse(
   Jeu *        jeu,
   Entete       entete,
   const void * requete,
   size_t       taille_de_la_requete,
   void *       reponse,
   size_t       taille_de_la_reponse,
   unsigned     delai_max            );
bool lire_la_socket( Jeu * jeu, Entete entete, void * corps, size_t taille_du_corps, unsigned delai_max );
