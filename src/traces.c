#include "traces.h"
#include "portability.h"
#include "utils.h"

#include <stdarg.h>
#include <stdio.h>

const char * etat_texte( Etat etat ) {
   switch( etat ) {
   case Debut_du_programme              : return "Début du programme";
   case Placement_du_porte_avion        : return "Placement du porte-avion";
   case Placement_du_croiseur           : return "Placement du croiseur";
   case Placement_du_contre_torpilleur_1: return "Placement du contre-torpilleur n°1";
   case Placement_du_contre_torpilleur_2: return "Placement du contre-torpilleur n°2";
   case Placement_du_torpilleur         : return "Placement du torpilleur";
   case Tirage_au_sort                  : return "Tirage au sort";
   case En_attente_de_l_autre_joueur    : return "En attente de l'autre joueur";
   case Placement_d_une_torpille        : return "Placement d'une torpille";
   case Attendre_decision_rejouer       : return "Attendre décision rejouer";
   case Partie_achevee                  : return "Partie achevée";
   case Fin_du_programme                : return "Fin du programme";
   default                              : return "???";
   }
}

const char * action_texte( Action action ) {
   switch( action ) {
   case Aucune : return "Aucune";
   case Jouer  : return "Jouer";
   case Rejouer: return "Rejouer";
   case Quitter: return "Quitter";
   default     : return "???";
   }
}

const char * etat_torpille_texte( Etat_torpille etat ) {
   switch( etat ) {
   case Etat_torpille_Aucun: return "Pose en cours";
   case Posee        : return "Posée";
   case Dans_l_eau   : return "Dans l'eau";
   case Touche       : return "Touché";
   case Coule        : return "Coulé";
   default           : return "???";
   }
}

const char * entete_texte( Entete entete ) {
   switch( entete ) {
   case PROTOCOLE_AUCUN             : return "Aucun";
   case PROTOCOLE_POIGNEE_DE_MAIN   : return "Poignée de main";
   case PROTOCOLE_TIRAGE_AU_SORT    : return "Tirage au sort";
   case PROTOCOLE_TORPILLE_POSEE    : return "Torpille posée";
   case PROTOCOLE_DEGATS_OCCASIONNES: return "Dégâts occasionnés";
   case PROTOCOLE_PARTIE_ACHEVEE    : return "Partie achevée";
   default                          : return "???";
   }
}

static void dump_navire( const Jeu * jeu, const Navire * navire ) {
   fprintf( jeu->journal, "%s\n", navire->nom );
   fprintf( jeu->journal, "\tligne  : %2d\n", navire->ligne );
   fprintf( jeu->journal, "\tcolonne: %2d\n", navire->colonne );
   fprintf( jeu->journal, "\torientation: %s\n", ( navire->orientation == Horizontale ) ? "Horizontale" : "Verticale" );
   fprintf( jeu->journal, "\tdégâts: { " );
   for( int i = 0; i < navire->taille; ++i ) {
      fprintf( jeu->journal, "%s%s", navire->degats[i] ? "true" : "false", ( i != navire->taille - 1 ) ? ", " : " }" );
   }
   fprintf( jeu->journal, "\n" );
}

static void dump_torpille( const Jeu * jeu, int i ) {
   const Torpille * torpille = jeu->torpilles + i;
   fprintf( jeu->journal, "Torpille n°%2d\n", i );
   fprintf( jeu->journal, "\tligne  : %2d\n", torpille->ligne );
   fprintf( jeu->journal, "\tcolonne: %2d\n", torpille->colonne );
   fprintf( jeu->journal, "\tetat   : %s\n", etat_torpille_texte( torpille->etat ));
}

void dump( Jeu * jeu ) {
   for( size_t i = 0; i < sizeof(jeu->navires)/sizeof(jeu->navires[0]); ++i ) {
      dump_navire( jeu, jeu->navires + i );
   }
   for( int i = 0; i < jeu->index_torpille; ++i ) {
      dump_torpille( jeu, i );
   }
}

void ajouter_une_entree_au_journal(
   FILE *       journal,
   const char * fichier,
   int          ligne,
   const char * fonction,
   const char * format, ... )
{
   va_list args;
   va_start( args, format );
   char buffer[200];
   vsnprintf( buffer, sizeof( buffer ), format, args );
   va_end( args );
   fprintf( journal, "%6"FMT_SIZE_T"u:%"FMT_SIZE_T"u:%s:%d:%s|%s\n", heure_courante_en_ms(), (uint64_t)pthread_self(), fichier, ligne, fonction, buffer );
   fflush( journal );
}
