#pragma once

#include "bataille_navale.h"

bool initialiser_l_ihm( Jeu ** jeux );
void afficher( Jeu * jeu );
bool interruption( void );
void obtenir_la_decision_du_joueur( Jeu * jeu );
void liberer_les_ressources_ihm( Jeu * jeu );
