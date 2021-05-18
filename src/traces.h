#pragma once

#include "bataille_navale.h"
#include "protocole.h"

void initialiser_journal( Jeu * jeu, const char * chemin_du_journal );

const char * etat_texte         ( Etat etat );
const char * action_texte       ( Action action );
const char * etat_torpille_texte( Etat_torpille etat );
const char * entete_texte       ( Entete entete );

void dump( Jeu * jeu );

void ajouter_une_entree_au_journal(
   FILE *       journal,
   const char * fichier,
   int          ligne,
   const char * fonction,
   const char * format,
   ... );

#define TRACE(J,F,...) ajouter_une_entree_au_journal( J, __FILE__, __LINE__, __func__, F, __VA_ARGS__ )
#define ENTREE(J)      ajouter_une_entree_au_journal( J, __FILE__, __LINE__, __func__, "" )
#define ECHEC(J)       ajouter_une_entree_au_journal( J, __FILE__, __LINE__, __func__, "échec" )
