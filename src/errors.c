#include "errors.h"
#include "traces.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

bool check_syscall( Jeu * jeu, const char * file, int line, const char * function, const char * call, ssize_t retCode ) {
   if( retCode < 0 ) {
      ajouter_une_entree_au_journal( jeu->journal, file, line, function, "%s: %s", call, strerror( errno ));
      jeu->action = Quitter;
      jeu->etat   = Fin_du_programme;
      return false;
   }
   return true;
}

bool check_value( Jeu * jeu, const char * file, int line, const char * function, ssize_t observee, size_t attendue ) {
   if( observee != (ssize_t)attendue ) {
      ajouter_une_entree_au_journal( jeu->journal, file, line, function,
         "Valeur observée : %"FMT_SIZE_T"d, valeur attendue : %"FMT_SIZE_T"d\n", observee, attendue );
      jeu->action = Quitter;
      jeu->etat   = Fin_du_programme;
      return false;
   }
   return true;
}
