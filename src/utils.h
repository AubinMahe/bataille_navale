#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "portability.h"

BN_API void     initialiser_le_generateur_de_nombre_aleatoire( void );
BN_API int      nombre_aleatoire_entre_zero_et( double max );
BN_API uint64_t heure_courante_en_ms( void );
BN_API void     sleep_ms( uint64_t ms );
