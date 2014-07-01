/**
 * @file
 * @brief Misc monster poly/mimic functions.
**/

#include "AppHdr.h"
#include "mon-poly.h"

#include "artefact.h"
#include "attitude-change.h"
#include "delay.h"
#include "describe.h"
#include "dgn-overview.h"
#include "dungeon.h"
#include "env.h"
#include "exclude.h"
#include "godconduct.h"
#include "hints.h"
#include "itemprop.h"
#include "items.h"
#include "libutil.h"
#include "monster.h"
#include "mon-flags.h"
#include "mon-death.h"
#include "mon-message.h"
#include "mon-place.h"
#include "mon-util.h"
#include "notes.h"
#include "player.h"
#include "religion.h"
#include "state.h"
#include "terrain.h"
#include "traps.h"
#include "xom.h"

dungeon_feature_type get_mimic_feat(const monster* mimic)
{
    if (mimic->props.exists("feat_type"))
        return static_cast<dungeon_feature_type>(mimic->props["feat_type"].get_short());
    else
        return DNGN_FLOOR;
}

bool feature_mimic_at(const coord_def &c)
{
    return map_masked(c, MMT_MIMIC);
}

item_def* item_mimic_at(const coord_def &c)
{
    for (stack_iterator si(c); si; ++si)
        if (si->flags & ISFLAG_MIMIC)
            return &*si;
    return NULL;
}

bool mimic_at(const coord_def &c)
{
    return feature_mimic_at(c) || item_mimic_at(c);
}

// The default suitable() function for monster_drop_things().
bool is_any_item(const item_def& item)
{
    return true;
}

void monster_drop_things(monster* mons,
                          bool mark_item_origins,
                          bool (*suitable)(const item_def& item),
                          int owner_id)
{
    // Drop weapons and missiles last (i.e., on top), so others pick up.
    for (int i = NUM_MONSTER_SLOTS - 1; i >= 0; --i)
    {
        int item = mons->inv[i];

        if (item != NON_ITEM && suitable(mitm[item]))
        {
            const bool summoned_item =
                testbits(mitm[item].flags, ISFLAG_SUMMONED);
            if (summoned_item)
            {
                item_was_destroyed(mitm[item], mons->mindex());
                destroy_item(item);
            }
            else
            {
                if (mons->friendly() && mitm[item].defined())
                    mitm[item].flags |= ISFLAG_DROPPED_BY_ALLY;

                if (mark_item_origins && mitm[item].defined())
                    origin_set_monster(mitm[item], mons);

                if (mitm[item].props.exists("autoinscribe"))
                {
                    add_inscription(mitm[item],
                        mitm[item].props["autoinscribe"].get_string());
                    mitm[item].props.erase("autoinscribe");
                }

                // Unrands held by fixed monsters would give awfully redundant
                // messages ("Cerebov hits you with the Sword of Cerebov."),
                // thus delay identification until drop/death.
                autoid_unrand(mitm[item]);

                // If a monster is swimming, the items are ALREADY
                // underwater.
                move_item_to_grid(&item, mons->pos(), mons->swimming());
            }

            mons->inv[i] = NON_ITEM;
        }
    }
}

static bool _valid_morph(monster* mons, monster_type new_mclass)
{
    const dungeon_feature_type current_tile = grd(mons->pos());

    monster_type old_mclass = mons_base_type(mons);

    // Shapeshifters cannot polymorph into glowing shapeshifters or
    // vice versa.
    if ((new_mclass == MONS_GLOWING_SHAPESHIFTER
             && mons->has_ench(ENCH_SHAPESHIFTER))
         || (new_mclass == MONS_SHAPESHIFTER
             && mons->has_ench(ENCH_GLOWING_SHAPESHIFTER)))
    {
        return false;
    }

    // Various inappropriate polymorph targets.
    if (mons_class_holiness(new_mclass) != mons_class_holiness(old_mclass)
        || mons_class_flag(new_mclass, M_UNFINISHED)  // no unfinished monsters
        || mons_class_flag(new_mclass, M_CANT_SPAWN)  // no dummy monsters
        || mons_class_flag(new_mclass, M_NO_POLY_TO)  // explicitly disallowed
        || mons_class_flag(new_mclass, M_UNIQUE)      // no uniques
        || mons_class_flag(new_mclass, M_NO_EXP_GAIN) // not helpless
        || new_mclass == MONS_PROGRAM_BUG

        // 'morph targets are _always_ "base" classes, not derived ones.
        || new_mclass != mons_species(new_mclass)
        || new_mclass == mons_species(old_mclass)
        // They act as separate polymorph classes on their own.
        || mons_class_is_zombified(new_mclass)
        || mons_is_zombified(mons) && !mons_zombie_size(new_mclass)
        // Currently unused (no zombie shapeshifters, no polymorph).
        || mons->type == MONS_SKELETON && !mons_skeleton(new_mclass)
        || mons->type == MONS_ZOMBIE && !mons_zombifiable(new_mclass)

        // These require manual setting of the ghost demon struct to
        // indicate their characteristics, which we currently aren't
        // smart enough to handle.
        || mons_is_ghost_demon(new_mclass)

        // Other poly-unsuitable things.
        || mons_is_statue(new_mclass)
        || mons_is_projectile(new_mclass)
        || mons_is_tentacle_or_tentacle_segment(new_mclass)

        // Don't polymorph things without Gods into priests.
        || (mons_class_flag(new_mclass, MF_PRIEST) && mons->god == GOD_NO_GOD)
        // The spell on Prince Ribbit can't be broken so easily.
        || (new_mclass == MONS_HUMAN
            && (mons->type == MONS_PRINCE_RIBBIT
                || mons->mname == "Prince Ribbit")))
    {
        return false;
    }

    // Determine if the monster is happy on current tile.
    return monster_habitable_grid(new_mclass, current_tile);
}

static bool _is_poly_power_unsuitable(poly_power_type power,
                                       int src_pow, int tgt_pow, int relax)
{
    switch (power)
    {
    case PPT_LESS:
        return tgt_pow > src_pow - 3 + relax * 3 / 2
                || (power == PPT_LESS && tgt_pow < src_pow - relax / 2);
    case PPT_MORE:
        return tgt_pow < src_pow + 2 - relax
                || (power == PPT_MORE && tgt_pow > src_pow + relax);
    default:
    case PPT_SAME:
        return tgt_pow < src_pow - relax
                || tgt_pow > src_pow + relax * 3 / 2;
    }
}

static bool _jiyva_slime_target(monster_type targetc)
{
    return you_worship(GOD_JIYVA)
           && (targetc == MONS_DEATH_OOZE
              || targetc == MONS_OOZE
              || targetc == MONS_JELLY
              || targetc == MONS_SLIME_CREATURE
              || targetc == MONS_ACID_BLOB
              || targetc == MONS_AZURE_JELLY);
}

void change_monster_type(monster* mons, monster_type targetc)
{
    bool could_see     = you.can_see(mons);
    bool slimified = _jiyva_slime_target(targetc);

    // Quietly remove the old monster's invisibility before transforming
    // it.  If we don't do this, it'll stay invisible even after losing
    // the invisibility enchantment below.
    mons->del_ench(ENCH_INVIS, false, false);

    // Remove replacement tile, since it probably doesn't work for the
    // new monster.
    mons->props.erase("monster_tile_name");
    mons->props.erase("monster_tile");

    // Even if the monster transforms from one type that can behold the
    // player into a different type which can also behold the player,
    // the polymorph disrupts the beholding process.  Do this before
    // changing mons->type, since unbeholding can only happen while
    // the monster is still a mermaid/siren.
    you.remove_beholder(mons);
    you.remove_fearmonger(mons);

    if (mons_is_tentacle_head(mons_base_type(mons)))
        destroy_tentacles(mons);

    // Inform listeners that the original monster is gone.
    fire_monster_death_event(mons, KILL_MISC, NON_MONSTER, true);

    // the actual polymorphing:
    uint64_t flags =
        mons->flags & ~(MF_INTERESTING | MF_SEEN | MF_ATT_CHANGE_ATTEMPT
                           | MF_WAS_IN_VIEW | MF_BAND_MEMBER | MF_KNOWN_SHIFTER
                           | MF_MELEE_MASK | MF_SPELL_MASK);
    flags |= MF_POLYMORPHED;
    string name;

    // Preserve the names of uniques and named monsters.
    if (mons->type == MONS_ROYAL_JELLY
        || mons->mname == "shaped Royal Jelly")
    {
        name   = "shaped Royal Jelly";
        flags |= MF_INTERESTING | MF_NAME_SUFFIX;
    }
    else if (mons->type == MONS_LERNAEAN_HYDRA
             || mons->mname == "shaped Lernaean hydra")
    {
        name   = "shaped Lernaean hydra";
        flags |= MF_INTERESTING | MF_NAME_SUFFIX;
    }
    else if (mons->mons_species() == MONS_SERPENT_OF_HELL
             || mons->mname == "shaped Serpent of Hell")
    {
        name   = "shaped Serpent of Hell";
        flags |= MF_INTERESTING | MF_NAME_SUFFIX;
    }
    else if (!mons->mname.empty())
    {
        if (flags & MF_NAME_MASK)
        {
            // Remove the replacement name from the new monster
            flags = flags & ~(MF_NAME_MASK | MF_NAME_DESCRIPTOR
                              | MF_NAME_DEFINITE | MF_NAME_SPECIES
                              | MF_NAME_ZOMBIE | MF_NAME_NOCORPSE);
        }
        else
            name = mons->mname;
    }
    else if (mons_is_unique(mons->type))
    {
        flags |= MF_INTERESTING;

        name = mons->name(DESC_PLAIN, true);

        // "Blork the orc" and similar.
        const size_t the_pos = name.find(" the ");
        if (the_pos != string::npos)
            name = name.substr(0, the_pos);
    }

    const monster_type real_targetc =
        (mons->has_ench(ENCH_GLOWING_SHAPESHIFTER)) ? MONS_GLOWING_SHAPESHIFTER :
        (mons->has_ench(ENCH_SHAPESHIFTER))         ? MONS_SHAPESHIFTER
                                                    : targetc;

    const god_type god =
        (player_will_anger_monster(real_targetc)
            || (you_worship(GOD_BEOGH)
                && mons_genus(real_targetc) != MONS_ORC)) ? GOD_NO_GOD
                                                          : mons->god;

    if (god == GOD_NO_GOD)
        flags &= ~MF_GOD_GIFT;

    const int  old_hp             = mons->hit_points;
    const int  old_hp_max         = mons->max_hit_points;
    const bool old_mon_caught     = mons->caught();
    const char old_ench_countdown = mons->ench_countdown;

    // XXX: mons_is_unique should be converted to monster::is_unique, and that
    // function should be testing the value of props["original_was_unique"]
    // which would make things a lot simpler.
    // See also record_monster_defeat.
    bool old_mon_unique           = mons_is_unique(mons->type);
    if (mons->props.exists("original_was_unique")
        && mons->props["original_was_unique"].get_bool())
    {
        old_mon_unique = true;
    }

    mon_enchant abj       = mons->get_ench(ENCH_ABJ);
    mon_enchant fabj      = mons->get_ench(ENCH_FAKE_ABJURATION);
    mon_enchant charm     = mons->get_ench(ENCH_CHARM);
    mon_enchant shifter   = mons->get_ench(ENCH_GLOWING_SHAPESHIFTER,
                                           ENCH_SHAPESHIFTER);
    mon_enchant sub       = mons->get_ench(ENCH_SUBMERGED);
    mon_enchant summon    = mons->get_ench(ENCH_SUMMON);
    mon_enchant tp        = mons->get_ench(ENCH_TP);
    mon_enchant vines     = mons->get_ench(ENCH_AWAKEN_VINES);
    mon_enchant forest    = mons->get_ench(ENCH_AWAKEN_FOREST);

    monster_spells spl    = mons->spells;
    const bool need_save_spells
            = (!name.empty()
               && (!mons->can_use_spells() || mons->is_actual_spellcaster())
               && !slimified);

    mons->number       = 0;

    // Note: define_monster() will clear out all enchantments! - bwr
    if (mons_is_zombified(mons))
        define_zombie(mons, targetc, mons->type);
    else
    {
        mons->type         = targetc;
        mons->base_monster = MONS_NO_MONSTER;
        define_monster(mons);
    }

    mons->mname = name;
    mons->props["original_name"] = name;
    mons->props["original_was_unique"] = old_mon_unique;
    mons->god   = god;
    mons->props.erase("dbname");

    mons->flags = flags;
    // Line above might clear melee and/or spell flags; restore.
    mons->bind_melee_flags();
    mons->bind_spell_flags();

    // Forget various speech/shout Lua functions.
    mons->props.erase("speech_prefix");

    // Don't allow polymorphing monsters for hides.
    mons->props["no_hide"] = true;

    // Keep spells for named monsters, but don't override innate ones
    // for dragons and the like. This means that Sigmund polymorphed
    // into a goblin will still cast spells, but if he ends up as a
    // swamp drake he'll breathe fumes and, if polymorphed further,
    // won't remember his spells anymore.
    if (need_save_spells
        && (!mons->can_use_spells() || mons->is_actual_spellcaster()))
    {
        mons->spells = spl;
    }

    mons->add_ench(abj);
    mons->add_ench(fabj);
    mons->add_ench(charm);
    mons->add_ench(shifter);
    mons->add_ench(sub);
    mons->add_ench(summon);
    mons->add_ench(tp);
    mons->add_ench(vines);
    mons->add_ench(forest);

    // Allows for handling of submerged monsters which polymorph into
    // monsters that can't submerge on this square.
    if (mons->has_ench(ENCH_SUBMERGED)
        && !monster_can_submerge(mons, grd(mons->pos())))
    {
        mons->del_ench(ENCH_SUBMERGED);
    }

    mons->ench_countdown = old_ench_countdown;

    if (mons_class_flag(mons->type, M_INVIS))
        mons->add_ench(ENCH_INVIS);

    mons->hit_points = mons->max_hit_points * old_hp / old_hp_max
                       + random2(mons->max_hit_points);

    mons->hit_points = min(mons->max_hit_points, mons->hit_points);

    // Don't kill it.
    mons->hit_points = max(mons->hit_points, 1);

    mons->speed_increment = 67 + random2(6);

    monster_drop_things(mons);

    // New monster type might be interesting.
    mark_interesting_monst(mons);

    // If new monster is visible to player, then we've seen it.
    if (you.can_see(mons))
    {
        seen_monster(mons);
        // If the player saw both the beginning and end results of a
        // shifter changing, then s/he knows it must be a shifter.
        if (could_see && shifter.ench != ENCH_NONE)
            discover_shifter(mons);
    }

    if (old_mon_caught)
        check_net_will_hold_monster(mons);

    // Even if the new form can constrict, it might be with a different
    // body part.  Likewise, the new form might be too large for its
    // current constrictor.  Rather than trying to handle these as special
    // cases, just stop the constriction entirely.  The usual message about
    // evaporating and reforming justifies this behaviour.
    mons->stop_constricting_all(false);
    mons->stop_being_constricted();

    mons->check_clinging(false);
}

// If targetc == RANDOM_MONSTER, then relpower indicates the desired
// power of the new monster, relative to the current monster.
// Relaxation still takes effect when needed, no matter what relpower
// says.
bool monster_polymorph(monster* mons, monster_type targetc,
                       poly_power_type power,
                       bool force_beh)
{
    // Don't attempt to polymorph a monster that is busy using the stairs.
    if (mons->flags & MF_TAKING_STAIRS)
        return false;
    ASSERT(!(mons->flags & MF_BANISHED) || player_in_branch(BRANCH_ABYSS));

    int source_power, target_power, relax;
    int source_tier, target_tier;
    int tries = 1000;

    // Used to be mons_power, but that just returns hit_dice
    // for the monster class.  By using the current hit dice
    // the player gets the opportunity to use draining more
    // effectively against shapeshifters. - bwr
    source_power = mons->hit_dice;
    source_tier = mons_demon_tier(mons->type);

    // There's not a single valid target on the '&' demon tier, so unless we
    // make one, let's ban this outright.
    if (source_tier == -1)
    {
        return simple_monster_message(mons,
            "'s appearance momentarily alters.");
    }
    relax = 1;

    if (targetc == RANDOM_MONSTER)
    {
        do
        {
            // Pick a monster species that's guaranteed happy at this grid.
            targetc = random_monster_at_grid(mons->pos(), true);

            target_power = mons_power(targetc);
            // Can't compare tiers in valid_morph, since we want to affect only
            // random polymorphs, and not absolutely, too.
            target_tier = mons_demon_tier(targetc);

            if (one_chance_in(200))
                relax++;

            if (relax > 50)
                return simple_monster_message(mons, " shudders.");
        }
        while (tries-- && (!_valid_morph(mons, targetc)
                           || source_tier != target_tier && !x_chance_in_y(relax, 200)
                           || _is_poly_power_unsuitable(power, source_power,
                                                        target_power, relax)));
    }

    bool could_see = you.can_see(mons);
    bool need_note = (could_see && MONST_INTERESTING(mons));
    string old_name_a = mons->full_name(DESC_A);
    string old_name_the = mons->full_name(DESC_THE);
    monster_type oldc = mons->type;

    if (targetc == RANDOM_TOUGHER_MONSTER)
    {
        vector<monster_type> target_types;
        for (int mc = 0; mc < NUM_MONSTERS; ++mc)
        {
            const monsterentry *me = get_monster_data((monster_type) mc);
            int delta = (int) me->hpdice[0] - mons->hit_dice;
            if (delta != 1)
                continue;
            if (!_valid_morph(mons, (monster_type) mc))
                continue;
            target_types.push_back((monster_type) mc);
        }
        if (target_types.empty())
            return false;

        shuffle_array(target_types);
        targetc = target_types[0];
    }

    if (!_valid_morph(mons, targetc))
        return simple_monster_message(mons, " looks momentarily different.");

    change_monster_type(mons, targetc);

    bool can_see = you.can_see(mons);

    // Messaging
    bool player_messaged = true;
    if (could_see)
    {
        string verb = "";
        string obj = "";

        obj = can_see ? mons_type_name(targetc, DESC_A)
                      : "something you cannot see";

        if (oldc == MONS_OGRE && targetc == MONS_TWO_HEADED_OGRE)
        {
            verb = "grows a second head";
            obj = "";
        }
        else if (mons->is_shapeshifter())
            verb = "changes into ";
        else if (_jiyva_slime_target(targetc))
            verb = "quivers uncontrollably and liquefies into ";
        else
            verb = "evaporates and reforms as ";

        mprf("%s %s%s!", old_name_the.c_str(), verb.c_str(), obj.c_str());
    }
    else if (can_see)
    {
        mprf("%s appears out of thin air!", mons->name(DESC_A).c_str());
        autotoggle_autopickup(false);
    }
    else
        player_messaged = false;

    if (need_note || could_see && can_see && MONST_INTERESTING(mons))
    {
        string new_name = can_see ? mons->full_name(DESC_A)
                                  : "something unseen";

        take_note(Note(NOTE_POLY_MONSTER, 0, 0, old_name_a.c_str(),
                       new_name.c_str()));

        if (can_see)
            mons->flags |= MF_SEEN;
    }

    if (!force_beh)
        player_angers_monster(mons);

    // Xom likes watching monsters being polymorphed.
    if (can_see)
    {
        xom_is_stimulated(mons->is_shapeshifter()               ? 12 :
                          power == PPT_LESS || mons->friendly() ? 25 :
                          power == PPT_SAME                     ? 50
                                                                : 100);
    }

    return player_messaged;
}

bool mon_can_be_slimified(monster* mons)
{
    const mon_holy_type holi = mons->holiness();

    return !mons->is_insubstantial()
           && !mons_is_tentacle_or_tentacle_segment(mons->type)
           && (holi == MH_UNDEAD
                || holi == MH_NATURAL && !mons_is_slime(mons));
}

void slimify_monster(monster* mon, bool hostile)
{
    monster_type target = MONS_JELLY;

    const int x = mon->hit_dice + (coinflip() ? 1 : -1) * random2(5);

    if (x < 3)
        target = MONS_OOZE;
    else if (x >= 3 && x < 5)
        target = MONS_JELLY;
    else if (x >= 5 && x <= 11)
        target = MONS_SLIME_CREATURE;
    else
    {
        if (coinflip())
            target = MONS_ACID_BLOB;
        else
            target = MONS_AZURE_JELLY;
    }

    if (feat_is_water(grd(mon->pos()))) // Pick something amphibious.
        target = (x < 7) ? MONS_JELLY : MONS_SLIME_CREATURE;

    if (mon->holiness() == MH_UNDEAD)
        target = MONS_DEATH_OOZE;

    // Bail out if jellies can't live here.
    if (!monster_habitable_grid(target, grd(mon->pos())))
    {
        simple_monster_message(mon, " quivers momentarily.");
        return;
    }

    monster_polymorph(mon, target);

    if (!mons_eats_items(mon))
        mon->add_ench(ENCH_EAT_ITEMS);

    if (!hostile)
        mon->attitude = ATT_STRICT_NEUTRAL;
    else
        mon->attitude = ATT_HOSTILE;

    mons_make_god_gift(mon, GOD_JIYVA);

    // Don't want shape-shifters to shift into non-slimes.
    mon->del_ench(ENCH_GLOWING_SHAPESHIFTER);
    mon->del_ench(ENCH_SHAPESHIFTER);

    mons_att_changed(mon);
}

void seen_monster(monster* mons)
{
    // If the monster is in the auto_exclude list, automatically
    // set an exclusion.
    set_auto_exclude(mons);
    set_unique_annotation(mons);

    item_def* weapon = mons->weapon();
    if (weapon && is_range_weapon(*weapon))
        mons->flags |= MF_SEEN_RANGED;

    // Monster was viewed this turn
    mons->flags |= MF_WAS_IN_VIEW;

    if (mons->flags & MF_SEEN)
        return;

    // First time we've seen this particular monster.
    mons->flags |= MF_SEEN;

    // mark items as seen.
    for (int slot = MSLOT_WEAPON; slot <= MSLOT_LAST_VISIBLE_SLOT; slot++)
    {
        int item_id = mons->inv[slot];
        if (item_id != NON_ITEM)
            mitm[item_id].flags |= ISFLAG_SEEN;
    }

    if (!mons_is_mimic(mons->type))
    {
        if (crawl_state.game_is_hints())
            hints_monster_seen(*mons);

        if (MONST_INTERESTING(mons))
        {
            string name = mons->name(DESC_A, true);
            if (mons->type == MONS_PLAYER_GHOST)
            {
                name += make_stringf(" (%s)",
                        short_ghost_description(mons, true).c_str());
            }
            take_note(
                      Note(NOTE_SEEN_MONSTER, mons->type, 0,
                           name.c_str()));
        }
    }

    if (!(mons->flags & MF_TSO_SEEN))
    {
        if (!mons->has_ench(ENCH_ABJ)
            && !mons->has_ench(ENCH_FAKE_ABJURATION)
            && !testbits(mons->flags, MF_NO_REWARD)
            && !mons_class_flag(mons->type, M_NO_EXP_GAIN)
            && !crawl_state.game_is_arena())
        {
            did_god_conduct(DID_SEE_MONSTER, mons->hit_dice, true, mons);
        }
        mons->flags |= MF_TSO_SEEN;
    }

    if (mons_allows_beogh(mons))
        env.level_state |= LSTATE_BEOGH;
}