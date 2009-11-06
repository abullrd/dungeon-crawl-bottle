/*
 *  File:       godabil.h
 *  Summary:    God-granted abilities.
 */

#ifndef GODABIL_H
#define GODABILN_H

#include "enum.h"
#include "externs.h"

bool ponderousify_armour();
bool zin_sustenance(bool actual = true);
bool zin_remove_all_mutations();
bool yred_injury_mirror(bool actual = true);
bool jiyva_grant_jelly(bool actual = true);
bool jiyva_remove_bad_mutation();
bool beogh_water_walk();
void yred_make_enslaved_soul(monsters *mon, bool force_hostile = false,
                             bool quiet = false, bool unrestricted = false);
bool feawn_passthrough(const monsters * target);

bool vehumet_supports_spell(spell_type spell);

bool trog_burn_spellbooks();

void lugonu_bends_space();

int cheibriados_slouch(int pow);
void cheibriados_time_bend(int pow);
void cheibriados_time_step(int pow);
#endif
