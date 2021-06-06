#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COUNT 10000000

int main() {
   initialiser_le_generateur_de_nombre_aleatoire();
   int repartition[6];
   memset( repartition, 0, sizeof( repartition ));
   for( unsigned i = 0; i < (unsigned)COUNT; ++i ) {
      int tirage = nombre_aleatoire_entre_zero_et( 4 );
      ++repartition[tirage];
   }
   printf( "0: %7d : %6.3f %%\n", repartition[0], 100.0*repartition[0]/(double)COUNT );
   printf( "1: %7d : %6.3f %%\n", repartition[1], 100.0*repartition[1]/(double)COUNT );
   printf( "2: %7d : %6.3f %%\n", repartition[2], 100.0*repartition[2]/(double)COUNT );
   printf( "3: %7d : %6.3f %%\n", repartition[3], 100.0*repartition[3]/(double)COUNT );
   printf( "4: %7d : %6.3f %%\n", repartition[4], 100.0*repartition[4]/(double)COUNT );
   printf( "5: %7d : %6.3f %%\n", repartition[5], 100.0*repartition[5]/(double)COUNT );
   return EXIT_SUCCESS;
}
