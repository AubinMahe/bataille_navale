#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "portability.h"

uint64_t heure_courante_en_ms( void );
void     sleep_ms( uint64_t ms );
