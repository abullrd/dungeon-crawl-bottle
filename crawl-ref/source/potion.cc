/**
 * @file
 * @brief Potion and potion-like effects.
**/

#include "AppHdr.h"

#include "potion.h"

#include <stdio.h>
#include <string.h>

#include "food.h"
#include "godconduct.h"
#include "hints.h"
#include "itemname.h"
#include "itemprop.h"
#include "message.h"
#include "misc.h"
#include "mutation.h"
#include "player-stats.h"
#include "skill_menu.h"
#include "spl-goditem.h"
#include "stringutil.h"
#include "transform.h"
#include "xom.h"

/**
 * Apply the effect of a potion to the player.
 *
 * This is called when the player quaffs a potion, but also for some cards,
 * beams, god effects and miscasts.
 *
 * @param pot_eff       The potion type.
 * @param pow           The power of the effect. 40 for actual potions.
 * @param was_known     Whether the potion was already identified.
 *
 * @return If the potion was used.
 */
bool potion_effect(potion_type pot_eff, int pow, item_def *potion, bool was_known)
{
    pow = min(pow, 150);

    // Reduced effect for healing potions.
    int mut_factor = 3 - you.mutation[MUT_NO_DEVICE_HEAL];

    // Knowingly drinking bad potions is much less amusing.
    int xom_factor = 1;
    if (potion && was_known)
    {
        xom_factor *= 2;
        if (!player_in_a_dangerous_place())
            xom_factor *= 3;
    }

    switch (pot_eff)
    {
    case POT_CURING:
    {
        const bool ddoor = you.duration[DUR_DEATHS_DOOR];

        if ((!you.can_device_heal() || ddoor)
            && potion && was_known
            && you.duration[DUR_CONF] == 0
            && you.duration[DUR_POISONING] == 0
            && you.rotting == 0
            && you.disease == 0
            // The potion won't heal us, so don't count rot unless at max HP.
            && (!player_rotted() || you.hp != you.hp_max))
        {
            mprf("You have no ailments to cure%s.",
                 ddoor && you.can_device_heal()
                     ? ", and can't heal while in Death's door"
                     : "");
            return false;
        }

        if (you.can_device_heal() && !ddoor)
            inc_hp((5 + random2(7)) * mut_factor / 3);

        mprf("You feel %s.", ddoor ? "queasy" : "better");

        // Only fix rot when healed to full.
        if (you.hp == you.hp_max)
        {
            unrot_hp(1);
            set_hp(you.hp_max);
        }

        // need to redraw from yellow to green even if no hp was gained
        if (you.duration[DUR_POISONING])
            you.redraw_hit_points = true;

        you.duration[DUR_POISONING] = 0;
        you.rotting = 0;
        you.disease = 0;
        you.duration[DUR_CONF] = 0;
        break;
    }

    case POT_HEAL_WOUNDS:
        if (you.duration[DUR_DEATHS_DOOR])
        {
            if (potion && was_known)
            {
                mpr("You can't heal while in Death's door!");
                return false;
            }
            mpr("You feel queasy.");
            break;
        }

        if (!you.can_device_heal())
        {
            if (potion && was_known)
            {
                mpr("That would not heal you.");
                return false;
            }
            mpr("That seemed strangely inert.");
            break;
        }

        inc_hp((10 + random2avg(28, 3)) * mut_factor / 3);
        mpr("You feel much better.");

        // only fix rot when healed to full
        if (you.hp == you.hp_max)
        {
            unrot_hp(2 + random2avg(5, 2));
            set_hp(you.hp_max);
        }
        break;

    case POT_BLOOD:
#if TAG_MAJOR_VERSION == 34
    case POT_BLOOD_COAGULATED:
#endif
        if (you.species == SP_VAMPIRE)
        {
            // No healing anymore! (jpeg)
            int value = 840;
#if TAG_MAJOR_VERSION == 34
            if (pot_eff == POT_BLOOD)
            {
#endif
                mpr("Yummy - fresh blood!");
                value += 200;
#if TAG_MAJOR_VERSION == 34
            }
            else // Coagulated.
                mpr("This tastes delicious!");
#endif

            lessen_hunger(value, true);
        }
        else
        {
            const int value = pot_eff == POT_BLOOD ? 1040 : 840;
            const int herbivorous = player_mutation_level(MUT_HERBIVOROUS);

            if (herbivorous < 3 && player_likes_chunks())
            {
                // Likes it.
                mpr("This tastes like blood.");
                if (you.species == SP_GHOUL && player_rotted())
                {
                    mpr("You feel more resilient.");
                    unrot_hp(1);
                }
                lessen_hunger(value, true);
            }
            else
            {
                mpr("Yuck - this tastes like blood.");
                if (x_chance_in_y(herbivorous + 1, 4))
                {
                    // Full herbivores always become ill from blood.
                    you.sicken(50 + random2(100));
                    xom_is_stimulated(25 / xom_factor);
                }
                else
                    lessen_hunger(value, true);
            }
        }
        did_god_conduct(DID_DRINK_BLOOD, 1 + random2(3), was_known);
        break;

    case POT_HASTE:
        if (potion && was_known && you.stasis(false))
        {
            mpr("This potion can't work under stasis.");
            return false;
        }

        if (haste_player(40 + random2(pow)))
            did_god_conduct(DID_HASTY, 10, was_known);
        break;

    case POT_MIGHT:
    {
        const bool were_mighty = you.duration[DUR_MIGHT] > 0;

        mprf(MSGCH_DURATION, "You feel %s all of a sudden.",
             were_mighty ? "mightier" : "very mighty");

        you.increase_duration(DUR_MIGHT, 35 + random2(pow), 80);

        if (!were_mighty)
            notify_stat_change(STAT_STR, 5, true);
        break;
    }

    case POT_BRILLIANCE:
    {
        const bool were_brilliant = you.duration[DUR_BRILLIANCE] > 0;

        mprf(MSGCH_DURATION, "You feel %s all of a sudden.",
             were_brilliant ? "more clever" : "clever");

        you.increase_duration(DUR_BRILLIANCE, 35 + random2(pow), 80);

        if (!were_brilliant)
            notify_stat_change(STAT_INT, 5, true);
        break;
    }

    case POT_AGILITY:
    {
        const bool were_agile = you.duration[DUR_AGILITY] > 0;

        mprf(MSGCH_DURATION, "You feel %s all of a sudden.",
             were_agile ? "more agile" : "agile");

        you.increase_duration(DUR_AGILITY, 35 + random2(pow), 80);

        if (!were_agile)
            notify_stat_change(STAT_DEX, 5, true);
        break;
    }

#if TAG_MAJOR_VERSION == 34
    case POT_GAIN_STRENGTH:
        if (mutate(MUT_STRONG, "potion of gain strength", true, false, false, true))
            learned_something_new(HINT_YOU_MUTATED);
        break;

    case POT_GAIN_DEXTERITY:
        if (mutate(MUT_AGILE, "potion of gain dexterity", true, false, false, true))
            learned_something_new(HINT_YOU_MUTATED);
        break;

    case POT_GAIN_INTELLIGENCE:
        if (mutate(MUT_CLEVER, "potion of gain intelligence", true, false, false, true))
            learned_something_new(HINT_YOU_MUTATED);
        break;
#endif

    case POT_FLIGHT:
        if (!flight_allowed())
        {
            if (potion && was_known)
                return false;
            break;
        }

        you.attribute[ATTR_FLIGHT_UNCANCELLABLE] = 1;
        fly_player(pow);
        break;

    case POT_POISON:
#if TAG_MAJOR_VERSION == 34
    case POT_STRONG_POISON:
#endif
        if (player_res_poison() >= 1)
            mpr("You feel slightly nauseous.");
        else
        {
            mprf(MSGCH_WARN,
                 "That liquid tasted very nasty...");

            int amount = 10 + random2avg(15, 2);
            string msg = "a potion of poison";

            poison_player(amount, "", msg);
            xom_is_stimulated(100 / xom_factor);
        }
        break;

    // Potions of slowing no longer exist, but tons of other effects use
    // potion_effect(POT_SLOWING) so this code stays in.
    case POT_SLOWING:
        if (slow_player(10 + random2(pow)))
            xom_is_stimulated(50 / xom_factor);
        break;

    case POT_CANCELLATION:
        debuff_player();
        mpr("You feel magically purged.");
        break;

    case POT_CONFUSION:
        if (confuse_player(3 + random2(8)))
            xom_is_stimulated(100 / xom_factor);
        break;

    case POT_INVISIBILITY:
        if (you.backlit() || you.haloed())
        {
            // XXX: merge with item_use.cc:_dont_use_invis()
            if (potion && was_known && get_contamination_level() > 1)
            {
                mpr("You cannot become invisible while glowing.");
                return false;
            }

            vector<string> afflictions;
            if (you.haloed())
                afflictions.push_back("halo");
            if (get_contamination_level() > 1)
                afflictions.push_back("magical contamination");
            if (you.duration[DUR_CORONA])
                afflictions.push_back("corona");
            if (you.duration[DUR_LIQUID_FLAMES])
                afflictions.push_back("liquid flames");
            if (you.duration[DUR_QUAD_DAMAGE])
                afflictions.push_back("!!!QUAD DAMAGE!!!");

            mprf(MSGCH_DURATION,
                 "You become %stransparent, but the glow from %s "
                 "%s prevents you from becoming "
                 "completely invisible.",
                 you.duration[DUR_INVIS] ? "more " : "",
                 you.haloed() && you.halo_radius2() == -1 ? "the" : "your",
                 comma_separated_line(afflictions.begin(), afflictions.end()).c_str());
        }
        else
        {
            mprf(MSGCH_DURATION, !you.duration[DUR_INVIS]
                 ? "You fade into invisibility!"
                 : "You fade further into invisibility.");
        }

        // Now multiple invisiblity casts aren't as good. -- bwr
        if (!you.duration[DUR_INVIS])
            you.set_duration(DUR_INVIS, 15 + random2(pow), 100);
        else
            you.increase_duration(DUR_INVIS, random2(pow), 100);

        if (potion)
            you.attribute[ATTR_INVIS_UNCANCELLABLE] = 1;

        break;

    case POT_PORRIDGE:
        if (you.species == SP_VAMPIRE
            || player_mutation_level(MUT_CARNIVOROUS) == 3)
        {
            mpr("Blech - that potion was really gluggy!");
        }
        else
        {
            mpr("That potion was really gluggy!");
            lessen_hunger(6000, true);
        }
        break;

    case POT_DEGENERATION:
        if (potion)
            mpr("There was something very wrong with that liquid!");

        if (lose_stat(STAT_RANDOM, 1 + random2avg(4, 2), false,
                      "drinking a potion of degeneration"))
        {
            xom_is_stimulated(50 / xom_factor);
        }
        break;

    case POT_DECAY:
        if (you.rot(&you, 0, 3 + random2(3)))
            xom_is_stimulated(50 / xom_factor);
        break;

#if TAG_MAJOR_VERSION == 34
    case POT_WATER:
#endif
    case NUM_POTIONS:
        if (you.species == SP_VAMPIRE)
            mpr("Blech - this tastes like water.");
        else
            mpr("This tastes like water.");
        break;

    case POT_EXPERIENCE:
        if (you.experience_level < 27)
        {
            mpr("You feel more experienced!");
            adjust_level(1, true);

            // Deferred calling level_change() into item_use.cc:3919, after
            // dec_inv_item_quantity. This prevents using SIGHUP to get infinite
            // potions of experience. Confer Mantis #3245. [due]
        }
        else
            mpr("A flood of memories washes over you.");
        more();
        skill_menu(SKMF_EXPERIENCE_POTION, 750 * you.experience_level);
        break;

    case POT_MAGIC:
        // Allow repairing rot, disallow going through Death's Door.
#if TAG_MAJOR_VERSION == 34
        if (you.species == SP_DJINNI)
            return potion_effect(POT_HEAL_WOUNDS, pow, potion, was_known);
#endif

        inc_mp(10 + random2avg(28, 3));
        mpr("Magic courses through your body.");
        break;

    case POT_RESTORE_ABILITIES:
    {
        bool nothing_happens = true;
        if (you.duration[DUR_BREATH_WEAPON])
        {
            mprf(MSGCH_RECOVERY, "You have got your breath back.");
            you.duration[DUR_BREATH_WEAPON] = 0;
            nothing_happens = false;
        }

        // Give a message if no message otherwise.
        if (!restore_stat(STAT_ALL, 0, false) && nothing_happens)
            mpr("You feel refreshed.");
        break;
    }

    case POT_BERSERK_RAGE:
        if (potion && was_known && !you.can_go_berserk(true, potion, false))
            return false;

        if (you.species == SP_VAMPIRE && you.hunger_state <= HS_SATIATED)
        {
            mpr("You feel slightly irritated.");
            make_hungry(100, false);
        }
        else
        {
            if (you.go_berserk(was_known, true))
                xom_is_stimulated(50);
        }
        break;

    case POT_CURE_MUTATION:
        if (potion && was_known && undead_mutation_rot())
        {
            mpr(you.form == TRAN_LICH ? "You cannot mutate at present."
                                      : "You cannot mutate.");
            return false;
        }
        mpr("It has a very clean taste.");
        for (int i = 0; i < 7; i++)
            if (random2(9) >= i)
                delete_mutation(RANDOM_MUTATION, "potion of cure mutation", false);
        break;

    case POT_MUTATION:
        if (potion && was_known && undead_mutation_rot())
        {
            mpr(you.form == TRAN_LICH ? "You cannot mutate at present."
                                      : "You cannot mutate.");
            return false;
        }

        mpr("You feel extremely strange.");
        for (int i = 0; i < 3; i++)
            mutate(RANDOM_MUTATION, "potion of mutation", false);

        learned_something_new(HINT_YOU_MUTATED);
        did_god_conduct(DID_DELIBERATE_MUTATING, 10, was_known);
        break;

    case POT_BENEFICIAL_MUTATION:
        if (undead_mutation_rot())
        {
            if (potion && was_known)
            {
                mpr(you.form == TRAN_LICH ? "You cannot mutate at present."
                                          : "You cannot mutate.");
                return false;
            }

            mpr("You feel dead inside.");
            mutate(RANDOM_GOOD_MUTATION, "potion of beneficial mutation",
                true, false, false, true);
            break;
        }

        if (mutate(RANDOM_GOOD_MUTATION, "potion of beneficial mutation",
               true, false, false, true))
        {
            mpr("You feel fantastic!");
            did_god_conduct(DID_DELIBERATE_MUTATING, 10, was_known);
        }
        else
            mpr("You feel fantastic for a moment.");
        learned_something_new(HINT_YOU_MUTATED);
        break;

    case POT_RESISTANCE:
        mprf(MSGCH_DURATION, "You feel protected.");
        you.increase_duration(DUR_RESISTANCE, random2(pow) + 35);
        break;

    case POT_LIGNIFY:
        if (potion && was_known && !transform(0, TRAN_TREE, false, true))
            return false;

        if (transform(30, TRAN_TREE, !was_known))
        {
            you.transform_uncancellable = true;
            did_god_conduct(DID_CHAOS, 10, was_known);
        }
        else
            mpr("You feel woody for a moment.");
        break;
    }

    if (potion && !was_known)
    {
        set_ident_flags(*potion, ISFLAG_IDENT_MASK);
        set_ident_type(*potion, ID_KNOWN_TYPE);
        mprf("It was a %s.", potion->name(DESC_QUALNAME).c_str());
        identify_healing_pots();
    }

    return true;
}
