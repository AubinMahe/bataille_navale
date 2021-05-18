#pragma once

#include "bataille_navale.h"

#include <sys/types.h>

bool check_syscall( Jeu * jeu, const char * file, int line, const char * function, const char * call, ssize_t retCode );
bool check_value( Jeu * jeu, const char * file, int line, const char * function, ssize_t observee, size_t attendue );

#define CHECK_SYS( JEU, CALL )      check_syscall( JEU, __FILE__, __LINE__, __func__, #CALL, CALL )
#define CHECK_VAL( JEU, OBS, EXP )  check_value( JEU, __FILE__, __LINE__, __func__, OBS, EXP )
