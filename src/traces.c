#include "traces.h"
#include "portability.h"
#include "utils.h"

#include <stdarg.h>
#include <string.h>

BN_API const char * etat_jeu_texte( Etat_du_jeu etat ) {
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

BN_API const char * action_texte( Action action ) {
   switch( action ) {
   case Aucune : return "Aucune";
   case Jouer  : return "Jouer";
   case Rejouer: return "Rejouer";
   case Quitter: return "Quitter";
   default     : return "???";
   }
}

BN_API const char * etat_torpille_texte( Etat_torpille etat ) {
   switch( etat ) {
   case et_Aucun     : return "Pose en cours";
   case et_Posee     : return "Posée";
   case et_Dans_l_eau: return "Dans l'eau";
   case et_Touche    : return "Touché";
   case et_Coule     : return "Coulé";
   default           : return "???";
   }
}

BN_API const char * etat_navire_texte( Etat_navire etat ) {
   switch( etat ) {
   case en_Aucun : return "Placement en cours";
   case en_Place : return "Placé et indemne";
   case en_Touche: return "Touché";
   case en_Coule : return "Coulé";
   default       : return "???";
   }
}

BN_API const char * entete_texte( Entete entete ) {
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
static void dump_line(
   FILE *       journal,
   const char * fichier,
   int          ligne,
   const char * fonction,
   char *       hexa,
   const char * ascii )
{
   hexa = strcat( hexa, "                                                                          " );
   hexa[6+( 3*8 )+2+( 3*8 )] = '\0';
   ajouter_une_entree_au_journal( journal, fichier, ligne, fonction, "%s - %s", hexa, ascii );
}

BN_API void dump_hexa(
   FILE *       journal,
   const char * fichier,
   int          ligne,
   const char * fonction,
   const char * octets,
   size_t       taille )
{
   char   hexStr[200];
   char   addStr[200];
   char   hexa  [200];
   char   ascii [200];
   memset( hexStr, 0, sizeof( hexStr ));
   memset( addStr, 0, sizeof( addStr ));
   memset( hexa  , 0, sizeof( hexa   ));
   memset( ascii , 0, sizeof( ascii  ));
   for( size_t i = 0; i < taille; ++i ) {
      char c = octets[i];
      if(( i % 16 ) == 0 ) {
         if( i > 0 ) {
            dump_line( journal, fichier, ligne, fonction, strcat( addStr, hexa ), ascii );
            memset( hexa , 0, sizeof( hexa  ));
            memset( ascii, 0, sizeof( ascii ));
         }
         snprintf( addStr, sizeof( addStr ), "%04X:", (unsigned int)i );
      }
      else if(( i % 8 ) == 0 ) {
         strcat( hexa, " -" );
      }
      sprintf( hexStr, " %02X", (unsigned char)c );
      strcat( hexa, hexStr );
      ascii[strlen(ascii)] = (( c > 32 ) && ( c < 127 )) ? (char)c : '.';
   }
   if( hexa[0] ) {
      dump_line( journal, fichier, ligne, fonction, strcat( addStr, hexa ), ascii );
   }
}

BN_API void ajouter_une_entree_au_journal(
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
   fprintf( journal, "%6"FMT_SIZE_T"u:%"FMT_SIZE_T"u:%s:%d:%s|%s\n",
      heure_courante_en_ms(), (uint64_t)pthread_self(), fichier, ligne, fonction, strtok( buffer, "\n\r" ));
   fflush( journal );
}
