#pragma once

#include "bataille_navale.h"
#include "protocole.h"

BN_API void initialiser_journal( Jeu * jeu, const char * chemin_du_journal );

BN_API const char * etat_jeu_texte     ( Etat_du_jeu          etat   );
BN_API const char * action_texte       ( Action        action );
BN_API const char * etat_torpille_texte( Etat_torpille etat   );
BN_API const char * etat_navire_texte  ( Etat_navire   etat );
BN_API const char * entete_texte       ( Entete        entete );

void dump( Jeu * jeu );

BN_API void dump_hexa(
   FILE *       journal,
   const char * fichier,
   int          ligne,
   const char * fonction,
   const char * octets,
   size_t       taille );

BN_API void ajouter_une_entree_au_journal(
   FILE *       journal,
   const char * fichier,
   int          ligne,
   const char * fonction,
   const char * format,
   ... );

#define TRACE(J,F,...)     ajouter_une_entree_au_journal( J, __FILE__, __LINE__, __func__, F, __VA_ARGS__ )
#define ENTREE(J)          ajouter_une_entree_au_journal( J, __FILE__, __LINE__, __func__, "" )
#define ECHEC(J)           ajouter_une_entree_au_journal( J, __FILE__, __LINE__, __func__, "échec" )
#define DUMP_HEXA(J,O,T)   dump_hexa( J, __FILE__, __LINE__, __func__, O, T )
