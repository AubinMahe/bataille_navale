#include "bataille_navale.h"

#include "delais.h"
#include "errors.h"
#include "protocole.h"
#include "traces.h"
#include "utils.h"

#include <limits.h>     // PATH_MAX
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   // mkdir()

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

BN_API void initialiser( Jeu * jeu ) {
   initialiser_le_generateur_de_nombre_aleatoire();
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

BN_API void initialiser_le_journal( Jeu * jeu, const char * chemin_du_journal ) {
   if( chemin_du_journal[0] ) {
      make_dir( chemin_du_journal, 0777 );
      char fullpath[PATH_MAX+21+21];
      snprintf( fullpath, sizeof( fullpath ), "%s/%s.txt", chemin_du_journal, jeu->nom_du_joueur );
      jeu->journal = fopen( fullpath, "wt" );
   }
}

BN_API void reinitialiser( Jeu * jeu ) {
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

BN_API bool une_action_du_joueur_est_attendue( const Jeu * jeu ) {
   return( jeu->etat == Placement_du_porte_avion )
      || ( jeu->etat == Placement_du_croiseur )
      || ( jeu->etat == Placement_du_contre_torpilleur_1 )
      || ( jeu->etat == Placement_du_contre_torpilleur_2 )
      || ( jeu->etat == Placement_du_torpilleur )
      || ( jeu->etat == Placement_d_une_torpille )
      || ( jeu->etat == Attendre_decision_rejouer );
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

static void placer_navire( Jeu * jeu, Etat_du_jeu prochain ) {
   ENTREE( jeu->journal );
   if( controler_le_placement_du_navire( jeu )) {
      Navire * navire = jeu->navires + jeu->index_navire;
      navire->etat = en_Place;
      navire = jeu->navires + ++(jeu->index_navire);
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
   torpille->etat    = et_Aucun;
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
      torpille->etat = et_Posee;
      int action[] = {
         torpille->colonne,
         torpille->ligne
      };
      if(   envoyer_message( jeu, PROTOCOLE_TORPILLE_POSEE, action, sizeof( action ))
         && lire_la_socket( jeu, PROTOCOLE_DEGATS_OCCASIONNES, &torpille->etat, sizeof( torpille->etat ), 500 ))
      {
         TRACE( jeu->journal, "résultat : %s", etat_torpille_texte( torpille->etat ));
         initialiser_la_prochaine_torpille( jeu );
         unsigned compteur_de_bateaux_coules = 0;
         for( int i = 0; i < jeu->index_torpille; ++i ) {
            if( jeu->torpilles[i].etat == et_Coule ) {
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

static bool navire_est_coule( const Navire * navire ) {
   for( int i = 0; i < navire->taille; ++i ) {
      if( navire->degats[i] == false ) {
         return false;
      }
   }
   return true;
}

static Etat_torpille evaluation_du_coup_adverse( Jeu * jeu, int colonne, int ligne ) {
   Etat_torpille torpille_etat = et_Dans_l_eau;
   for( int i = 0; ( torpille_etat == et_Dans_l_eau )&&( i < jeu->index_navire ); ++i ) {
      Navire * navire = jeu->navires + i;
      size_t index_collision = 0;
      if( navire_est_touche( navire, colonne, ligne, &index_collision )) {
         navire->degats[index_collision] = true;
         if( navire_est_coule( navire )) {
            torpille_etat = et_Coule;
            navire-> etat = en_Coule;
         }
         else {
            torpille_etat = et_Touche;
            navire-> etat = en_Touche;
         }
      }
   }
   TRACE( jeu->journal, "résultat: %s", etat_torpille_texte( torpille_etat ));
   return torpille_etat;
}

static void attendre_le_coup_de_l_autre_joueur( Jeu * jeu ) {
   ENTREE( jeu->journal );
   int action[2];
   if( ! lire_la_socket( jeu,
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
         if( navire->etat == en_Coule ) {
            TRACE( jeu->journal, "Navire '%s' est coulé", navire->nom );
            ++coules;
         }
      }
      jeu->action = Jouer;
      if( coules == NAVIRE_MAX ) {
         TRACE( jeu->journal, "Tous les navires sont %s", "coulés." );
         jeu->partie_gagnee = false;
         jeu->etat          = Partie_achevee;
      }
      else {
         jeu->etat = Placement_d_une_torpille;
      }
   }
   TRACE( jeu->journal, "Nouvel état : %s", etat_jeu_texte( jeu->etat ));
}

BN_API void jouer( Jeu * jeu ) {
   TRACE( jeu->journal, "état d'entrée : %s, action: %s", etat_jeu_texte( jeu->etat ), action_texte( jeu->action ));
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
   TRACE( jeu->journal, "état de sortie : %s", etat_jeu_texte( jeu->etat ));
}

BN_API void liberer_les_ressources( Jeu * jeu ) {
   ENTREE( jeu->journal );
//   dump( jeu );
   if( jeu->journal != stderr ) {
      fclose( jeu->journal );
   }
   free( jeu );
}
