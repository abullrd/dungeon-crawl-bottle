/**
 * @file
 * @brief Monster abilities.
**/

#ifndef MONABIL_H
#define MONABIL_H

class monster;
struct bolt;

bool mon_special_ability(monster* mons, bolt & beem);

void draconian_change_colour(monster* drac);

bool ugly_thing_mutate(monster* ugly, bool proximity = false);
bool slime_creature_polymorph(monster* slime);
void merge_ench_durations(monster* initial, monster* merge_to, bool usehd = false);

bool lost_soul_revive(monster* mons);

void treant_release_fauna(monster* mons);
void check_grasping_roots(actor* act, bool quiet = false);

void guardian_golem_bond(monster* mons);

#endif
