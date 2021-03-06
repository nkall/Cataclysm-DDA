#include "player.h"
#include "profession.h"
#include "bionics.h"
#include "mission.h"
#include "game.h"
#include "disease.h"
#include "addiction.h"
#include "moraledata.h"
#include "inventory.h"
#include "options.h"
#include <sstream>
#include <stdlib.h>
#include "weather.h"
#include "item.h"
#include "material.h"
#include "translations.h"
#include "name.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "get_version.h"
#include "crafting.h"
#include "item_factory.h"
#include "monstergenerator.h"
#include "help.h" // get_hint
#include "martialarts.h"
#include "output.h"
#include "overmapbuffer.h"
#include "messages.h"

//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

#include <ctime>
#include <algorithm>
#include <numeric>
#include <string>
#include <memory>
#include <array>

#include <fstream>

static void manage_fire_exposure(player& p, int fireStrength = 1);
static void handle_cough(player& p, int intensity = 1, int volume = 4);

std::map<std::string, trait> traits;
extern std::map<std::string, martialart> ma_styles;
std::vector<std::string> vStartingTraits[2];

std::string morale_data[NUM_MORALE_TYPES];

stats player_stats;

void game::init_morale()
{
    std::string tmp_morale_data[NUM_MORALE_TYPES] = {
    "This is a bug (moraledata.h:moraledata)",
    _("Enjoyed %i"),
    _("Enjoyed a hot meal"),
    _("Music"),
    _("Enjoyed honey"),
    _("Played Video Game"),
    _("Marloss Bliss"),
    _("Mutagenic Anticipation"),
    _("Good Feeling"),
    _("Supported"),

    _("Nicotine Craving"),
    _("Caffeine Craving"),
    _("Alcohol Craving"),
    _("Opiate Craving"),
    _("Speed Craving"),
    _("Cocaine Craving"),
    _("Crack Cocaine Craving"),
    _("Mutagen Craving"),
    _("Diazepam Craving"),

    _("Disliked %i"),
    _("Ate Human Flesh"),
    _("Ate Meat"),
    _("Ate Vegetables"),
    _("Ate Fruit"),
    _("Lactose Intolerance"),
    _("Ate Junk Food"),
    _("Wheat Allergy"),
    _("Ate Indigestible Food"),
    _("Wet"),
    _("Dried Off"),
    _("Cold"),
    _("Hot"),
    _("Bad Feeling"),
    _("Killed Innocent"),
    _("Killed Friend"),
    _("Guilty about Killing"),
    _("Chimerical Mutation"),
    _("Fey Mutation"),

    _("Moodswing"),
    _("Read %i"),
    _("Heard Disturbing Scream"),

    _("Masochism"),
    _("Hoarder"),
    _("Stylish"),
    _("Optimist"),
    _("Bad Tempered"),
    //~ You really don't like wearing the Uncomfy Gear
    _("Uncomfy Gear"),
    _("Found kitten <3")
    };
    for (int i = 0; i < NUM_MORALE_TYPES; ++i) {
        morale_data[i]=tmp_morale_data[i];
    }
}

player::player() : Character(), name("")
{
 posx = 0;
 posy = 0;
 id = 0; // Player is 0. NPCs are different.
 view_offset_x = 0;
 view_offset_y = 0;
 str_cur = 8;
 str_max = 8;
 dex_cur = 8;
 dex_max = 8;
 int_cur = 8;
 int_max = 8;
 per_cur = 8;
 per_max = 8;
 underwater = false;
 dodges_left = 1;
 blocks_left = 1;
 power_level = 0;
 max_power_level = 0;
 hunger = 0;
 thirst = 0;
 fatigue = 0;
 stim = 0;
 pain = 0;
 pkill = 0;
 radiation = 0;
 cash = 0;
 recoil = 0;
 driving_recoil = 0;
 scent = 500;
 health = 0;
 male = true;
 prof = profession::has_initialized() ? profession::generic() : NULL; //workaround for a potential structural limitation, see player::create
 start_location = "shelter";
 moves = 100;
 movecounter = 0;
 oxygen = 0;
 next_climate_control_check=0;
 last_climate_control_ret=false;
 active_mission = -1;
 in_vehicle = false;
 controlling_vehicle = false;
 grab_point.x = 0;
 grab_point.y = 0;
 grab_type = OBJECT_NONE;
 style_selected = "style_none";
 focus_pool = 100;
 last_item = itype_id("null");
 sight_max = 9999;
 sight_boost = 0;
 sight_boost_cap = 0;
 lastrecipe = NULL;
 lastconsumed = itype_id("null");
 next_expected_position.x = -1;
 next_expected_position.y = -1;

 for ( auto iter = traits.begin(); iter != traits.end(); ++iter) {
    my_traits.erase(iter->first);
    my_mutations.erase(iter->first);
 }

 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
      aSkill != Skill::skills.end(); ++aSkill) {
   skillLevel(*aSkill).level(0);
 }

 for (int i = 0; i < num_bp; i++) {
  temp_cur[i] = BODYTEMP_NORM;
  frostbite_timer[i] = 0;
  temp_conv[i] = BODYTEMP_NORM;
 }
 nv_cached = false;
 pda_cached = false;
 volume = 0;

 memorial_log.clear();
 player_stats.reset();

 mDrenchEffect[bp_eyes] = 1;
 mDrenchEffect[bp_mouth] = 1;
 mDrenchEffect[bp_head] = 7;
 mDrenchEffect[bp_legs] = 21;
 mDrenchEffect[bp_feet] = 6;
 mDrenchEffect[bp_arms] = 19;
 mDrenchEffect[bp_hands] = 5;
 mDrenchEffect[bp_torso] = 40;

 recalc_sight_limits();
}

player::player(const player &rhs): Character(rhs), JsonSerializer(), JsonDeserializer()
{
 *this = rhs;
}

player::~player()
{
}

player& player::operator= (const player & rhs)
{
 Character::operator=(rhs);
 id = rhs.id;
 posx = rhs.posx;
 posy = rhs.posy;
 view_offset_x = rhs.view_offset_x;
 view_offset_y = rhs.view_offset_y;

 in_vehicle = rhs.in_vehicle;
 controlling_vehicle = rhs.controlling_vehicle;
 grab_point = rhs.grab_point;
 activity = rhs.activity;
 backlog = rhs.backlog;

 active_missions = rhs.active_missions;
 completed_missions = rhs.completed_missions;
 failed_missions = rhs.failed_missions;
 active_mission = rhs.active_mission;

 name = rhs.name;
 male = rhs.male;
 prof = rhs.prof;
 start_location = rhs.start_location;

 sight_max = rhs.sight_max;
 sight_boost = rhs.sight_boost;
 sight_boost_cap = rhs.sight_boost_cap;

 my_traits = rhs.my_traits;
 my_mutations = rhs.my_mutations;
 mutation_category_level = rhs.mutation_category_level;

 my_bionics = rhs.my_bionics;

 power_level = rhs.power_level;
 max_power_level = rhs.max_power_level;

 hunger = rhs.hunger;
 thirst = rhs.thirst;
 fatigue = rhs.fatigue;
 health = rhs.health;

 underwater = rhs.underwater;
 oxygen = rhs.oxygen;
 next_climate_control_check=rhs.next_climate_control_check;
 last_climate_control_ret=rhs.last_climate_control_ret;
 known_traps = rhs.known_traps;

 recoil = rhs.recoil;
 driving_recoil = rhs.driving_recoil;
 scent = rhs.scent;
 dodges_left = rhs.dodges_left;
 blocks_left = rhs.blocks_left;

 stim = rhs.stim;
 pkill = rhs.pkill;
 radiation = rhs.radiation;

 cash = rhs.cash;
 movecounter = rhs.movecounter;

 for (int i = 0; i < num_hp_parts; i++)
  hp_cur[i] = rhs.hp_cur[i];

 for (int i = 0; i < num_hp_parts; i++)
  hp_max[i] = rhs.hp_max[i];

 for (int i = 0; i < num_bp; i++)
  temp_cur[i] = rhs.temp_cur[i];

 for (int i = 0 ; i < num_bp; i++)
  temp_conv[i] = rhs.temp_conv[i];

 for (int i = 0; i < num_bp; i++)
  frostbite_timer[i] = rhs.frostbite_timer[i];

 morale = rhs.morale;
 focus_pool = rhs.focus_pool;

 _skills = rhs._skills;

 learned_recipes = rhs.learned_recipes;

 inv.clear();
 for (int i = 0; i < rhs.inv.size(); i++)
  inv.add_stack(rhs.inv.const_stack(i));

 volume = rhs.volume;

 lastrecipe = rhs.lastrecipe;
 last_item = rhs.last_item;
 worn = rhs.worn;
 ma_styles = rhs.ma_styles;
 style_selected = rhs.style_selected;
 weapon = rhs.weapon;

 ret_null = rhs.ret_null;

 illness = rhs.illness;
 addictions = rhs.addictions;
 memorial_log = rhs.memorial_log;

 nv_cached = false;
 pda_cached = false;

 return (*this);
}

void player::normalize()
{
    Creature::normalize();

    ret_null = item("null", 0);
    weapon   = item("null", 0);
    style_selected = "style_none";

    recalc_hp();

    for (int i = 0 ; i < num_bp; i++)
        temp_conv[i] = BODYTEMP_NORM;
}

void player::pick_name()
{
    name = Name::generate(male);
}

std::string player::disp_name(bool possessive)
{
    if (!possessive) {
        if (is_player()) {
            return _("you");
        }
        return name;
    } else {
        if (is_player()) {
            return _("your");
        }
        return string_format(_("%s's"), name.c_str());
    }
}

std::string player::skin_name()
{
    //TODO: Return actual deflecting layer name
    return _("armor");
}

// just a shim for now since actual player death is handled in game::is_game_over
void player::die(Creature* nkiller)
{
    if( nkiller != NULL && !nkiller->is_fake() ) {
        killer = nkiller;
    }
}

void player::reset_stats()
{

    // Bionic buffs
    if (has_active_bionic("bio_hydraulics"))
        mod_str_bonus(20);
    if (has_bionic("bio_eye_enhancer"))
        mod_per_bonus(2);
    if (has_bionic("bio_str_enhancer"))
        mod_str_bonus(2);
    if (has_bionic("bio_int_enhancer"))
        mod_int_bonus(2);
    if (has_bionic("bio_dex_enhancer"))
        mod_dex_bonus(2);

    // Trait / mutation buffs
    if (has_trait("THICK_SCALES")) {
        mod_dex_bonus(-2);
    }
    if (has_trait("CHITIN2") || has_trait("CHITIN3")) {
        mod_dex_bonus(-1);
    }
    if (has_trait("COMPOUND_EYES") && !wearing_something_on(bp_eyes)) {
        mod_per_bonus(1);
    }
    if (has_trait("BIRD_EYE")) {
        mod_per_bonus(4);
    }
    if (has_trait("INSECT_ARMS")) {
        mod_dex_bonus(-2);
    }
    if (has_trait("INSECT_ARMS_OK")) {
        if (!wearing_something_on(bp_torso)) {
            mod_dex_bonus(1);
        }
        else {
            mod_dex_bonus(-1);
        }
    }
    if (has_trait("WEBBED")) {
        mod_dex_bonus(-1);
    }
    if (has_trait("ARACHNID_ARMS")) {
        mod_dex_bonus(-4);
    }
    if (has_trait("ARACHNID_ARMS_OK")) {
        if (!wearing_something_on(bp_torso)) {
            mod_dex_bonus(2);
        }
        else {
            mod_dex_bonus(-2);
        }
    }
    if (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
            has_trait("ARM_TENTACLES_8")) {
        mod_dex_bonus(1);
    }

    // Pain
    if (pain > pkill) {
        if (!(has_trait("CENOBITE"))) {
            mod_str_bonus(-int((pain - pkill) / 15));
            mod_dex_bonus(-int((pain - pkill) / 15));
        }
        mod_per_bonus(-int((pain - pkill) / 20));
        if (!(has_trait("INT_SLIME"))) {
            mod_int_bonus(-(1 + int((pain - pkill) / 25)));
        } else if (has_trait("INT_SLIME")) {
        // Having one's brain throughout one's body does have its downsides.
        // Be glad we don't assess permanent damage.
            mod_int_bonus(-(1 + int(pain - pkill)));
        }
    }
    // Morale
    if (abs(morale_level()) >= 100) {
        mod_str_bonus(int(morale_level() / 180));
        mod_dex_bonus(int(morale_level() / 200));
        mod_per_bonus(int(morale_level() / 125));
        mod_int_bonus(int(morale_level() / 100));
    }
    // Radiation
    if (radiation > 0) {
        mod_str_bonus(-int(radiation / 80));
        mod_dex_bonus(-int(radiation / 110));
        mod_per_bonus(-int(radiation / 100));
        mod_int_bonus(-int(radiation / 120));
    }
    // Stimulants
    mod_dex_bonus(int(stim / 10));
    mod_per_bonus(int(stim /  7));
    mod_int_bonus(int(stim /  6));
    if (stim >= 30) {
        mod_dex_bonus(-int(abs(stim - 15) /  8));
        mod_per_bonus(-int(abs(stim - 15) / 12));
        mod_int_bonus(-int(abs(stim - 15) / 14));
    }

    // Dodge-related effects
    mod_dodge_bonus( mabuff_dodge_bonus() - encumb(bp_legs)/2 - encumb(bp_torso) );
    if (has_trait("TAIL_LONG")) {
        mod_dodge_bonus(2);
    }
    if (has_trait("TAIL_CATTLE")) {
        mod_dodge_bonus(1);
    }
    if (has_trait("TAIL_RAT")) {
        mod_dodge_bonus(2);
    }
    if (has_trait("TAIL_THICK")) {
        mod_dodge_bonus(1);
    }
    if (has_trait("TAIL_RAPTOR")) {
        mod_dodge_bonus(3);
    }
    if (has_trait("TAIL_FLUFFY")) {
        mod_dodge_bonus(4);
    }
    if (has_trait("WHISKERS")) {
        mod_dodge_bonus(1);
    }
    if (has_trait("WINGS_BAT")) {
        mod_dodge_bonus(-3);
    }
    if (has_trait("WINGS_BUTTERFLY")) {
        mod_dodge_bonus(-4);
    }

    if (str_max >= 16) {mod_dodge_bonus(-1);} // Penalty if we're huge
    else if (str_max <= 5) {mod_dodge_bonus(1);} // Bonus if we're small

    // Hit-related effects
    mod_hit_bonus( mabuff_tohit_bonus() + weapon.type->m_to_hit - encumb(bp_torso) );

    // Apply static martial arts buffs
    ma_static_effects();

    if (int(calendar::turn) % 10 == 0) {
        update_mental_focus();
    }
    nv_cached = false;
    pda_cached = false;

    recalc_sight_limits();
    recalc_speed_bonus();

    Creature::reset_stats();

}

void player::process_turn()
{
    Creature::process_turn();

    // Didn't just pick something up
    last_item = itype_id("null");

    if (has_active_bionic("bio_metabolics") && power_level < max_power_level &&
            hunger < 100 && (int(calendar::turn) % 5 == 0)) {
        hunger += 2;
        power_level++;
    }

    suffer();

    // Set our scent towards the norm
    int norm_scent = 500;
    if (has_trait("WEAKSCENT")) {
        norm_scent = 300;
    }
    if (has_trait("SMELLY")) {
        norm_scent = 800;
    }
    if (has_trait("SMELLY2")) {
        norm_scent = 1200;
    }
    // Not so much that you don't have a scent
    // but that you smell like a plant, rather than
    // a human. When was the last time you saw a critter
    // attack a bluebell or an apple tree?
    if ( (has_trait("FLOWERS")) && (!(has_trait("CHLOROMORPH"))) ) {
        norm_scent -= 200;
    }
    // You *are* a plant.  Unless someone hunts triffids by scent,
    // you don't smell like prey.
    if (has_trait("CHLOROMORPH")) {
        norm_scent = 0;
    }

    // Scent increases fast at first, and slows down as it approaches normal levels.
    // Estimate it will take about norm_scent * 2 turns to go from 0 - norm_scent / 2
    // Without smelly trait this is about 1.5 hrs. Slows down significantly after that.
    if (scent < rng(0, norm_scent))
        scent++;

    // Unusually high scent decreases steadily until it reaches normal levels.
    if (scent > norm_scent)
        scent--;

    // We can dodge again! Assuming we can actually move...
    if (moves > 0) {
        blocks_left = get_num_blocks();
        dodges_left = get_num_dodges();
    }

}

void player::action_taken()
{
    nv_cached = false;
    pda_cached = false;
}

void player::update_morale()
{
    // Decay existing morale entries.
    for (int i = 0; i < morale.size(); i++)
    {
        // Age the morale entry by one turn.
        morale[i].age += 1;

        // If it's past its expiration date, remove it.
        if (morale[i].age >= morale[i].duration)
        {
            morale.erase(morale.begin() + i);
            i--;

            // Future-proofing.
            continue;
        }

        // We don't actually store the effective strength; it gets calculated when we
        // need it.
    }

    // We reapply persistent morale effects after every decay step, to keep them fresh.
    apply_persistent_morale();
}

void player::apply_persistent_morale()
{
    // Hoarders get a morale penalty if they're not carrying a full inventory.
    if (has_trait("HOARDER"))
    {
        int pen = int((volume_capacity()-volume_carried()) / 2);
        if (pen > 70)
        {
            pen = 70;
        }
        if (pen <= 0)
        {
            pen = 0;
        }
        if (has_disease("took_xanax"))
        {
            pen = int(pen / 7);
        }
        else if (has_disease("took_prozac"))
        {
            pen = int(pen / 2);
        }
        add_morale(MORALE_PERM_HOARDER, -pen, -pen, 5, 5, true);
    }

    // The stylish get a morale bonus for each body part covered in an item
    // with the FANCY or SUPER_FANCY tag.
    if (has_trait("STYLISH"))
    {
        int bonus = 0;
        std::string basic_flag = "FANCY";
        std::string bonus_flag = "SUPER_FANCY";

        unsigned char covered = 0; // body parts covered
        for( size_t i = 0; i < worn.size(); ++i ) {
            if(worn[i].has_flag(basic_flag) || worn[i].has_flag(bonus_flag) ) {
                it_armor* item_type = dynamic_cast<it_armor*>(worn[i].type);
                covered |= item_type->covers;
            }
            if(worn[i].has_flag(bonus_flag)) {
              bonus+=2;
            }
            if ((!(worn[i].has_flag(bonus_flag))) && (worn[i].has_flag(basic_flag))) {
                if (!covered) {
                    bonus += 1;
                }
            }
        }
        if(covered & mfb(bp_torso)) {
            bonus += 6;
        }
        if(covered & mfb(bp_legs)) {
            bonus += 4;
        }
        if(covered & mfb(bp_feet)) {
            bonus += 2;
        }
        if(covered & mfb(bp_hands)) {
            bonus += 2;
        }
        if(covered & mfb(bp_head)) {
            bonus += 3;
        }
        if(covered & mfb(bp_eyes)) {
            bonus += 2;
        }
        if(covered & mfb(bp_arms)) {
            bonus += 2;
        }
        if(covered & mfb(bp_mouth)) {
            bonus += 2;
        }

        if(bonus > 20)
            bonus = 20;

        if(bonus) {
            add_morale(MORALE_PERM_FANCY, bonus, bonus, 5, 5, true);
        }
    }

    // Floral folks really don't like having their flowers covered.
    if( has_trait("FLOWERS") && wearing_something_on(bp_head) ) {
        add_morale(MORALE_PERM_CONSTRAINED, -10, -10, 5, 5, true);
    }

    // The same applies to rooters and their feet; however, they don't take
    // too many problems from no-footgear.
    if( (has_trait("ROOTS") || has_trait("ROOTS2") || has_trait("ROOTS3") ) &&
        wearing_something_on(bp_feet) ) {
        add_morale(MORALE_PERM_CONSTRAINED, -10, -10, 5, 5, true);
    }

    // Masochists get a morale bonus from pain.
    if (has_trait("MASOCHIST") || has_trait("MASOCHIST_MED") ||  has_trait("CENOBITE")) {
        int bonus = pain / 2.5;
        // Advanced masochists really get a morale bonus from pain.
        // (It's not capped.)
        if (has_trait("MASOCHIST") && (bonus > 25)) {
            bonus = 25;
        }
        if (has_disease("took_prozac")) {
            bonus = int(bonus / 3);
        }
        if (bonus != 0) {
            add_morale(MORALE_PERM_MASOCHIST, bonus, bonus, 5, 5, true);
        }
    }

    // Optimist gives a base +4 to morale.
    // The +25% boost from optimist also applies here, for a net of +5.
    if (has_trait("OPTIMISTIC")) {
        add_morale(MORALE_PERM_OPTIMIST, 4, 4, 5, 5, true);
    }

    // And Bad Temper works just the same way.  But in reverse.  ):
    if (has_trait("BADTEMPER")) {
        add_morale(MORALE_PERM_BADTEMPER, -4, -4, 5, 5, true);
    }
}

void player::update_mental_focus()
{
    int focus_gain_rate = calc_focus_equilibrium() - focus_pool;

    // handle negative gain rates in a symmetric manner
    int base_change = 1;
    if (focus_gain_rate < 0)
    {
        base_change = -1;
        focus_gain_rate = -focus_gain_rate;
    }

    // for every 100 points, we have a flat gain of 1 focus.
    // for every n points left over, we have an n% chance of 1 focus
    int gain = focus_gain_rate / 100;
    if (rng(1, 100) <= (focus_gain_rate % 100))
    {
        gain++;
    }

    focus_pool += (gain * base_change);
}

// written mostly by FunnyMan3595 in Github issue #613 (DarklingWolf's repo),
// with some small edits/corrections by Soron
int player::calc_focus_equilibrium()
{
    // Factor in pain, since it's harder to rest your mind while your body hurts.
    int eff_morale = morale_level() - pain;
    // Cenobites don't mind, though
    if (has_trait("CENOBITE")) {
        eff_morale = eff_morale + pain;
    }
    int focus_gain_rate = 100;

    if (activity.type == ACT_READ) {
        item &book = i_at(activity.position);
        it_book* reading = dynamic_cast<it_book *>(book.type);
        if (reading != 0) {
            // apply a penalty when we're actually learning something
            if (skillLevel(reading->type) < (int)reading->level) {
                focus_gain_rate -= 50;
            }
        } else {
            activity.type = ACT_NULL;
        }
    }

    if (eff_morale < -99) {
        // At very low morale, focus goes up at 1% of the normal rate.
        focus_gain_rate = 1;
    } else if (eff_morale <= 50) {
        // At -99 to +50 morale, each point of morale gives 1% of the normal rate.
        focus_gain_rate += eff_morale;
    } else {
        /* Above 50 morale, we apply strong diminishing returns.
         * Each block of 50% takes twice as many morale points as the previous one:
         * 150% focus gain at 50 morale (as before)
         * 200% focus gain at 150 morale (100 more morale)
         * 250% focus gain at 350 morale (200 more morale)
         * ...
         * Cap out at 400% focus gain with 3,150+ morale, mostly as a sanity check.
         */

        int block_multiplier = 1;
        int morale_left = eff_morale;
        while (focus_gain_rate < 400) {
            if (morale_left > 50 * block_multiplier) {
                // We can afford the entire block.  Get it and continue.
                morale_left -= 50 * block_multiplier;
                focus_gain_rate += 50;
                block_multiplier *= 2;
            } else {
                // We can't afford the entire block.  Each block_multiplier morale
                // points give 1% focus gain, and then we're done.
                focus_gain_rate += morale_left / block_multiplier;
                break;
            }
        }
    }

    // This should be redundant, but just in case...
    if (focus_gain_rate < 1) {
        focus_gain_rate = 1;
    } else if (focus_gain_rate > 400) {
        focus_gain_rate = 400;
    }

    return focus_gain_rate;
}

/* Here lies the intended effects of body temperature

Assumption 1 : a naked person is comfortable at 19C/66.2F (31C/87.8F at rest).
Assumption 2 : a "lightly clothed" person is comfortable at 13C/55.4F (25C/77F at rest).
Assumption 3 : the player is always running, thus generating more heat.
Assumption 4 : frostbite cannot happen above 0C temperature.*
* In the current model, a naked person can get frostbite at 1C. This isn't true, but it's a compromise with using nice whole numbers.

Here is a list of warmth values and the corresponding temperatures in which the player is comfortable, and in which the player is very cold.

Warmth  Temperature (Comfortable)    Temperature (Very cold)    Notes
  0       19C /  66.2F               -11C /  12.2F               * Naked
 10       13C /  55.4F               -17C /   1.4F               * Lightly clothed
 20        7C /  44.6F               -23C /  -9.4F
 30        1C /  33.8F               -29C / -20.2F
 40       -5C /  23.0F               -35C / -31.0F
 50      -11C /  12.2F               -41C / -41.8F
 60      -17C /   1.4F               -47C / -52.6F
 70      -23C /  -9.4F               -53C / -63.4F
 80      -29C / -20.2F               -59C / -74.2F
 90      -35C / -31.0F               -65C / -85.0F
100      -41C / -41.8F               -71C / -95.8F
*/

void player::update_bodytemp()
{
    // NOTE : visit weather.h for some details on the numbers used
    // Converts temperature to Celsius/10(Wito plans on using degrees Kelvin later)
    int Ctemperature = 100*(g->get_temperature() - 32) * 5/9;
    // Temperature norms
    // Ambient normal temperature is lower while asleep
    int ambient_norm = (has_disease("sleep") ? 3100 : 1900);
    // This adjusts the temperature scale to match the bodytemp scale
    int adjusted_temp = (Ctemperature - ambient_norm);
    // This gets incremented in the for loop and used in the morale calculation
    int morale_pen = 0;
    const trap_id trap_at_pos = g->m.tr_at(posx, posy);
    const ter_id ter_at_pos = g->m.ter(posx, posy);
    const furn_id furn_at_pos = g->m.furn(posx, posy);
    // When the player is sleeping, he will use floor items for warmth
    int floor_item_warmth = 0;
    // When the player is sleeping, he will use floor bedding for warmth
    int floor_bedding_warmth = 0;
    // If the PC has fur, etc, that'll apply too
    int floor_mut_warmth = 0;
    if ( has_disease("sleep") ) {
        // Search the floor for items
        std::vector<item>& floor_item = g->m.i_at(posx, posy);
        it_armor* floor_armor = NULL;

        for ( std::vector<item>::iterator afloor_item = floor_item.begin() ;
        afloor_item != floor_item.end() ;
        ++afloor_item) {
            if ( !afloor_item->is_armor() ) {
                continue;
            }
            floor_armor = dynamic_cast<it_armor*>(afloor_item->type);
            // Items that are big enough and covers the torso are used to keep warm.
            // Smaller items don't do as good a job
            if ( floor_armor->volume > 1 &&
            ((floor_armor->covers & mfb(bp_torso)) ||
             (floor_armor->covers & mfb(bp_legs))) ) {
                floor_item_warmth += 60 * floor_armor->warmth * floor_armor->volume / 10;
            }
        }

        // Search the floor for bedding
        int vpart = -1;
        vehicle *veh = g->m.veh_at (posx, posy, vpart);
        if      (furn_at_pos == f_bed)
        {
            floor_bedding_warmth += 1000;
        }
        else if (furn_at_pos == f_makeshift_bed ||
                 furn_at_pos == f_armchair ||
                 furn_at_pos == f_sofa||
                 furn_at_pos == f_hay)
        {
            floor_bedding_warmth += 500;
        }
        else if (trap_at_pos == tr_cot)
        {
            floor_bedding_warmth -= 500;
        }
        else if (trap_at_pos == tr_rollmat)
        {
            floor_bedding_warmth -= 1000;
        }
        else if (trap_at_pos == tr_fur_rollmat)
        {
            floor_bedding_warmth += 0;
        }
        else if (veh && veh->part_with_feature (vpart, "SEAT") >= 0)
        {
            floor_bedding_warmth += 200;
        }
        else if (veh && veh->part_with_feature (vpart, "BED") >= 0)
        {
            floor_bedding_warmth += 300;
        }
        else
        {
            floor_bedding_warmth -= 2000;
        }
        // Fur, etc effects for sleeping here.
        // Full-power fur is about as effective as a makeshift bed
        if (has_trait("FUR") || has_trait("LUPINE_FUR") || has_trait("URSINE_FUR")) {
            floor_mut_warmth += 500;
        }
        // Feline fur, not quite as warm.  Cats do better in warmer spots.
        if (has_trait("FELINE_FUR")) {
            floor_mut_warmth += 300;
        }
        // Light fur's better than nothing!
        if (has_trait("LIGHTFUR")) {
            floor_mut_warmth += 100;
        }
        // Down helps too
        if (has_trait("DOWN")) {
            floor_mut_warmth += 250;
        }
        // DOWN doesn't provide floor insulation, though.
        // Non-light fur does.
        if ( (!(has_trait("DOWN"))) && (floor_mut_warmth >= 200)){
            if (floor_bedding_warmth < 0) {
                floor_bedding_warmth = 0;
            }
        }
    }
    // Current temperature and converging temperature calculations
    for (int i = 0 ; i < num_bp ; i++)
    {
        // Skip eyes
        if (i == bp_eyes) { continue; }
        // Represents the fact that the body generates heat when it is cold. TODO : should this increase hunger?
        float homeostasis_adjustement = (temp_cur[i] > BODYTEMP_NORM ? 30.0 : 60.0);
        int clothing_warmth_adjustement = homeostasis_adjustement * warmth(body_part(i));
        // Convergeant temperature is affected by ambient temperature, clothing warmth, and body wetness.
        temp_conv[i] = BODYTEMP_NORM + adjusted_temp + clothing_warmth_adjustement;
        // HUNGER
        temp_conv[i] -= hunger/6 + 100;
        // FATIGUE
        if (!has_disease("sleep")) { temp_conv[i] -= 1.5*fatigue; }
        // CONVECTION HEAT SOURCES (generates body heat, helps fight frostbite)
        int blister_count = 0; // If the counter is high, your skin starts to burn
        for (int j = -6 ; j <= 6 ; j++) {
            for (int k = -6 ; k <= 6 ; k++) {
                int heat_intensity = 0;

                int ffire = g->m.get_field_strength( point(posx + j, posy + k), fd_fire );
                if(ffire > 0) {
                    heat_intensity = ffire;
                } else if (g->m.tr_at(posx + j, posy + k) == tr_lava ) {
                    heat_intensity = 3;
                }
                if (heat_intensity > 0 && g->u_see(posx + j, posy + k)) {
                    // Ensure fire_dist >= 1 to avoid divide-by-zero errors.
                    int fire_dist = std::max(1, std::max(j, k));
                    if (frostbite_timer[i] > 0) {
                        frostbite_timer[i] -= heat_intensity - fire_dist / 2;
                    }
                    temp_conv[i] +=  300 * heat_intensity * heat_intensity / (fire_dist * fire_dist);
                    blister_count += heat_intensity / (fire_dist * fire_dist);
                }
            }
        }
        // TILES
        // Being on fire affects temp_cur (not temp_conv): this is super dangerous for the player
        if (has_effect("onfire")) {
            temp_cur[i] += 250;
        }
        if ( g->m.get_field_strength( point(posx, posy), fd_fire ) > 2 || trap_at_pos == tr_lava) {
            temp_cur[i] += 250;
        }
        // WEATHER
        if (g->weather == WEATHER_SUNNY && g->is_in_sunlight(posx, posy))
        {
            temp_conv[i] += 1000;
        }
        if (g->weather == WEATHER_CLEAR && g->is_in_sunlight(posx, posy))
        {
            temp_conv[i] += 500;
        }
        // DISEASES
        if (has_disease("flu") && i == bp_head) { temp_conv[i] += 1500; }
        if (has_disease("common_cold")) { temp_conv[i] -= 750; }
        // BIONICS
        // Bionic "Internal Climate Control" says it eases the effects of high and low ambient temps
        const int variation = BODYTEMP_NORM*0.5;
        if (in_climate_control()
            && temp_conv[i] < BODYTEMP_SCORCHING + variation
            && temp_conv[i] > BODYTEMP_FREEZING - variation)
        {
            if      (temp_conv[i] > BODYTEMP_SCORCHING)
            {
                temp_conv[i] = BODYTEMP_VERY_HOT;
            }
            else if (temp_conv[i] > BODYTEMP_VERY_HOT)
            {
                temp_conv[i] = BODYTEMP_HOT;
            }
            else if (temp_conv[i] > BODYTEMP_HOT)
            {
                temp_conv[i] = BODYTEMP_NORM;
            }
            else if (temp_conv[i] < BODYTEMP_FREEZING)
            {
                temp_conv[i] = BODYTEMP_VERY_COLD;
            }
            else if (temp_conv[i] < BODYTEMP_VERY_COLD)
            {
                temp_conv[i] = BODYTEMP_COLD;
            }
            else if (temp_conv[i] < BODYTEMP_COLD)
            {
                temp_conv[i] = BODYTEMP_NORM;
            }
        }
        // Bionic "Thermal Dissipation" says it prevents fire damage up to 2000F. 500 is picked at random...
        if ((has_bionic("bio_heatsink") || is_wearing("rm13_armor_on")) && blister_count < 500)
        {
            blister_count = (has_trait("BARK") ? -100 : 0);
        }
        // BLISTERS : Skin gets blisters from intense heat exposure.
        if (blister_count - 10*get_env_resist(body_part(i)) > 20)
        {
            add_disease("blisters", 1, false, 1, 1, 0, 1, (body_part)i, -1);
        }
        // BLOOD LOSS : Loss of blood results in loss of body heat
        int blood_loss = 0;
        if      (i == bp_legs)
        {
            blood_loss = (100 - 100*(hp_cur[hp_leg_l] + hp_cur[hp_leg_r]) / (hp_max[hp_leg_l] + hp_max[hp_leg_r]));
        }
        else if (i == bp_arms)
        {
            blood_loss = (100 - 100*(hp_cur[hp_arm_l] + hp_cur[hp_arm_r]) / (hp_max[hp_arm_l] + hp_max[hp_arm_r]));
        }
        else if (i == bp_torso)
        {
            blood_loss = (100 - 100* hp_cur[hp_torso] / hp_max[hp_torso]);
        }
        else if (i == bp_head)
        {
            blood_loss = (100 - 100* hp_cur[hp_head] / hp_max[hp_head]);
        }
        temp_conv[i] -= blood_loss*temp_conv[i]/200; // 1% bodyheat lost per 2% hp lost
        // EQUALIZATION
        switch (i)
        {
            case bp_torso :
                temp_equalizer(bp_torso, bp_arms);
                temp_equalizer(bp_torso, bp_legs);
                temp_equalizer(bp_torso, bp_head);
                break;
            case bp_head :
                temp_equalizer(bp_head, bp_torso);
                temp_equalizer(bp_head, bp_mouth);
                break;
            case bp_arms :
                temp_equalizer(bp_arms, bp_torso);
                temp_equalizer(bp_arms, bp_hands);
                break;
            case bp_legs :
                temp_equalizer(bp_legs, bp_torso);
                temp_equalizer(bp_legs, bp_feet);
                break;
            case bp_mouth : temp_equalizer(bp_mouth, bp_head); break;
            case bp_hands : temp_equalizer(bp_hands, bp_arms); break;
            case bp_feet  : temp_equalizer(bp_feet, bp_legs); break;
        }
        // MUTATIONS and TRAITS
        // Bark : lowers blister count to -100; harder to get blisters
        // Lightly furred
        if (has_trait("LIGHTFUR"))
        {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 250 : 500);
        }
        // Furry or Lupine/Ursine Fur
        if (has_trait("FUR") || has_trait("LUPINE_FUR") || has_trait("URSINE_FUR"))
        {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 750 : 1500);
        }
        // Feline fur
        if (has_trait("FELINE_FUR"))
        {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 500 : 1000);
        }
        // Feathers: minor means minor.
        if (has_trait("FEATHERS"))
        {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 50 : 100);
        }
        // Down; lets heat out more easily if needed but not as Warm
        // as full-blown fur.  So less miserable in Summer.
        if (has_trait("DOWN"))
        {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 300 : 800);
        }
        // Fat deposits don't hold in much heat, but don't shift for temp
        if (has_trait("FAT"))
        {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 200 : 200);
        }
        // Disintegration
        if (has_trait("ROT1")) { temp_conv[i] -= 250;}
        else if (has_trait("ROT2")) { temp_conv[i] -= 750;}
        else if (has_trait("ROT3")) { temp_conv[i] -= 1500;}
        // Radioactive
        if (has_trait("RADIOACTIVE1")) { temp_conv[i] += 250; }
        else if (has_trait("RADIOACTIVE2")) { temp_conv[i] += 750; }
        else if (has_trait("RADIOACTIVE3")) { temp_conv[i] += 1500; }
        // Chemical Imbalance
        // Added line in player::suffer()
        // FINAL CALCULATION : Increments current body temperature towards convergant.
        if ( has_disease("sleep") ) {
            int sleep_bonus = floor_bedding_warmth + floor_item_warmth + floor_mut_warmth;
            // Too warm, don't need items on the floor
            if ( temp_conv[i] > BODYTEMP_NORM ) {
                // Do nothing
            }
            // Intelligently use items on the floor; just enough to be comfortable
            else if ( (temp_conv[i] + sleep_bonus) > BODYTEMP_NORM ) {
                temp_conv[i] = BODYTEMP_NORM;
            }
            // Use all items on the floor -- there are not enough to keep comfortable
            else {
                temp_conv[i] += sleep_bonus;
            }
        }
        int temp_before = temp_cur[i];
        int temp_difference = temp_cur[i] - temp_conv[i]; // Negative if the player is warming up.
        // exp(-0.001) : half life of 60 minutes, exp(-0.002) : half life of 30 minutes,
        // exp(-0.003) : half life of 20 minutes, exp(-0.004) : half life of 15 minutes
        int rounding_error = 0;
        // If temp_diff is small, the player cannot warm up due to rounding errors. This fixes that.
        if (temp_difference < 0 && temp_difference > -600 )
        {
            rounding_error = 1;
        }
        if (temp_cur[i] != temp_conv[i])
        {
            if      ((ter_at_pos == t_water_sh || ter_at_pos == t_sewage)
                    && (i == bp_feet || i == bp_legs))
            {
                temp_cur[i] = temp_difference*exp(-0.004) + temp_conv[i] + rounding_error;
            }
            else if (ter_at_pos == t_water_dp)
            {
                temp_cur[i] = temp_difference*exp(-0.004) + temp_conv[i] + rounding_error;
            }
            else if (i == bp_torso || i == bp_head)
            {
                temp_cur[i] = temp_difference*exp(-0.003) + temp_conv[i] + rounding_error;
            }
            else
            {
                temp_cur[i] = temp_difference*exp(-0.002) + temp_conv[i] + rounding_error;
            }
        }
        int temp_after = temp_cur[i];
        // PENALTIES
        if      (temp_cur[i] < BODYTEMP_FREEZING)
        {
            add_disease("cold", 1, false, 3, 3, 0, 1, (body_part)i, -1);
            frostbite_timer[i] += 3;
        }
        else if (temp_cur[i] < BODYTEMP_VERY_COLD)
        {
            add_disease("cold", 1, false, 2, 3, 0, 1, (body_part)i, -1);
            frostbite_timer[i] += 2;
        }
        else if (temp_cur[i] < BODYTEMP_COLD)
        {
            // Frostbite timer does not go down if you are still cold.
            add_disease("cold", 1, false, 1, 3, 0, 1, (body_part)i, -1);
            frostbite_timer[i] += 1;
        }
        else if (temp_cur[i] > BODYTEMP_SCORCHING)
        {
            // If body temp rises over 15000, disease.cpp ("hot_head") acts weird and the player will die
            add_disease("hot",  1, false, 3, 3, 0, 1, (body_part)i, -1);
        }
        else if (temp_cur[i] > BODYTEMP_VERY_HOT)
        {
            add_disease("hot",  1, false, 2, 3, 0, 1, (body_part)i, -1);
        }
        else if (temp_cur[i] > BODYTEMP_HOT)
        {
            add_disease("hot",  1, false, 1, 3, 0, 1, (body_part)i, -1);
        }
        // MORALE : a negative morale_pen means the player is cold
        // Intensity multiplier is negative for cold, positive for hot
        int intensity_mult =
            - disease_intensity("cold", false, (body_part)i) +
            disease_intensity("hot", false, (body_part)i);
        if (has_disease("cold", (body_part)i) ||
            has_disease("hot", (body_part)i))
        {
            switch (i)
            {
                case bp_head :
                case bp_torso :
                case bp_mouth : morale_pen += 2*intensity_mult;
                case bp_arms :
                case bp_legs : morale_pen += 1*intensity_mult;
                case bp_hands:
                case bp_feet : morale_pen += 1*intensity_mult;
            }
        }
        // FROSTBITE (level 1 after 2 hours, level 2 after 4 hours)
        if      (frostbite_timer[i] >   0)
        {
            frostbite_timer[i]--;
        }
        if      (frostbite_timer[i] >= 240 && g->get_temperature() < 32)
        {
            add_disease("frostbite", 1, false, 2, 2, 0, 1, (body_part)i, -1);
            // Warning message for the player
            if (disease_intensity("frostbite", false, (body_part)i) < 2
                &&  (i == bp_mouth || i == bp_hands || i == bp_feet))
            {
                add_msg(m_bad, (i == bp_mouth ? _("Your %s hardens from the frostbite!") : _("Your %s harden from the frostbite!")), body_part_name(body_part(i), -1).c_str());
            }
            else if (frostbite_timer[i] >= 120 && g->get_temperature() < 32)
            {
                add_disease("frostbite", 1, false, 1, 2, 0, 1, (body_part)i, -1);
                // Warning message for the player
                if (!has_disease("frostbite", (body_part)i))
                {
                    add_msg(m_bad, _("You lose sensation in your %s."),
                        body_part_name(body_part(i), -1).c_str());
                }
            }
        }
        // Warn the player if condition worsens
        if  (temp_before > BODYTEMP_FREEZING && temp_after < BODYTEMP_FREEZING)
        {
            add_msg(m_warning, _("You feel your %s beginning to go numb from the cold!"),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before > BODYTEMP_VERY_COLD && temp_after < BODYTEMP_VERY_COLD)
        {
            add_msg(m_warning, _("You feel your %s getting very cold."),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before > BODYTEMP_COLD && temp_after < BODYTEMP_COLD)
        {
            add_msg(m_warning, _("You feel your %s getting chilly."),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before < BODYTEMP_SCORCHING && temp_after > BODYTEMP_SCORCHING)
        {
            add_msg(m_bad, _("You feel your %s getting red hot from the heat!"),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before < BODYTEMP_VERY_HOT && temp_after > BODYTEMP_VERY_HOT)
        {
            add_msg(m_warning, _("You feel your %s getting very hot."),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before < BODYTEMP_HOT && temp_after > BODYTEMP_HOT)
        {
            add_msg(m_warning, _("You feel your %s getting warm."),
                body_part_name(body_part(i), -1).c_str());
        }
    }
    // Morale penalties, updated at the same rate morale is
    if (morale_pen < 0 && int(calendar::turn) % 10 == 0)
    {
        add_morale(MORALE_COLD, -2, -abs(morale_pen), 10, 5, true);
    }
    if (morale_pen > 0 && int(calendar::turn) % 10 == 0)
    {
        add_morale(MORALE_HOT,  -2, -abs(morale_pen), 10, 5, true);
    }
}

void player::temp_equalizer(body_part bp1, body_part bp2)
{
    // Body heat is moved around.
    // Shift in one direction only, will be shifted in the other direction separately.
    int diff = (temp_cur[bp2] - temp_cur[bp1])*0.0001; // If bp1 is warmer, it will lose heat
    temp_cur[bp1] += diff;
}

void player::recalc_speed_bonus()
{
    // Minus some for weight...
    int carry_penalty = 0;
    if (weight_carried() > weight_capacity()) {
        carry_penalty = 25 * (weight_carried() - weight_capacity()) / (weight_capacity());
    }
    mod_speed_bonus(-carry_penalty);

    if (pain > pkill) {
        int pain_penalty = int((pain - pkill) * .7);
        // Cenobites aren't slowed nearly as much by pain
        if (has_trait("CENOBITE")) {
            pain_penalty /= 4;
        }
        if (pain_penalty > 60) {
            pain_penalty = 60;
        }
        mod_speed_bonus(-pain_penalty);
    }
    if (pkill >= 10) {
        int pkill_penalty = int(pkill * .1);
        if (pkill_penalty > 30) {
            pkill_penalty = 30;
        }
        mod_speed_bonus(-pkill_penalty);
    }

    if (abs(morale_level()) >= 100) {
        int morale_bonus = int(morale_level() / 25);
        if (morale_bonus < -10) {
            morale_bonus = -10;
        } else if (morale_bonus > 10) {
            morale_bonus = 10;
        }
        mod_speed_bonus(morale_bonus);
    }

    if (radiation >= 40) {
        int rad_penalty = radiation / 40;
        if (rad_penalty > 20) {
            rad_penalty = 20;
        }
        mod_speed_bonus(-rad_penalty);
    }

    if (thirst > 40) {
        mod_speed_bonus(-int((thirst - 40) / 10));
    }
    if (hunger > 100) {
        mod_speed_bonus(-int((hunger - 100) / 10));
    }

    mod_speed_bonus(stim > 40 ? 40 : stim);

    for (int i = 0; i < illness.size(); i++) {
        mod_speed_bonus(disease_speed_boost(illness[i]));
    }

    // add martial arts speed bonus
    mod_speed_bonus(mabuff_speed_bonus());

    // Not sure why Sunlight Dependent is here, but OK
    // Ectothermic/COLDBLOOD4 is intended to buff folks in the Summer
    // Threshold-crossing has its charms ;-)
    if (g != NULL) {
        if (has_trait("SUNLIGHT_DEPENDENT") && !g->is_in_sunlight(posx, posy)) {
            mod_speed_bonus(-(g->light_level() >= 12 ? 5 : 10));
        }
        if ((has_trait("COLDBLOOD4")) && g->get_temperature() > 60) {
            mod_speed_bonus(+int( (g->get_temperature() - 65) / 2));
        }
        if ((has_trait("COLDBLOOD3") || has_trait("COLDBLOOD4")) && g->get_temperature() < 60) {
            mod_speed_bonus(-int( (65 - g->get_temperature()) / 2));
        } else if (has_trait("COLDBLOOD2") && g->get_temperature() < 60) {
            mod_speed_bonus(-int( (65 - g->get_temperature()) / 3));
        } else if (has_trait("COLDBLOOD") && g->get_temperature() < 60) {
            mod_speed_bonus(-int( (65 - g->get_temperature()) / 5));
        }
    }

    if (has_artifact_with(AEP_SPEED_UP)) {
        mod_speed_bonus(20);
    }
    if (has_artifact_with(AEP_SPEED_DOWN)) {
        mod_speed_bonus(-20);
    }

    if (has_trait("QUICK")) { // multiply by 1.1
        set_speed_bonus(get_speed() * 1.10 - get_speed_base());
    }
    if (has_bionic("bio_speed")) { // multiply by 1.1
        set_speed_bonus(get_speed() * 1.10 - get_speed_base());
    }

    // Speed cannot be less than 25% of base speed, so minimal speed bonus is -75% base speed.
    const int min_speed_bonus = -0.75 * get_speed_base();
    if (get_speed_bonus() < min_speed_bonus) {
        set_speed_bonus(min_speed_bonus);
    }
}

int player::run_cost(int base_cost, bool diag)
{
    float movecost = float(base_cost);
    if (diag)
        movecost *= 0.7071f; // because everything here assumes 100 is base
    bool flatground = movecost < 105;
    const ter_id ter_at_pos = g->m.ter(posx, posy);
    // If your floor is hard, flat, and otherwise skateable, list it here
    bool offroading = ( flatground && (!((ter_at_pos == t_rock_floor) ||
      (ter_at_pos == t_pit_covered) || (ter_at_pos == t_metal_floor) ||
      (ter_at_pos == t_pit_spiked_covered) || (ter_at_pos == t_pavement) ||
      (ter_at_pos == t_pavement_y) || (ter_at_pos == t_sidewalk) ||
      (ter_at_pos == t_concrete) || (ter_at_pos == t_floor) ||
      (ter_at_pos == t_skylight) || (ter_at_pos == t_door_glass_o) ||
      (ter_at_pos == t_emergency_light_flicker) ||
      (ter_at_pos == t_emergency_light) || (ter_at_pos == t_utility_light) ||
      (ter_at_pos == t_door_o) || (ter_at_pos == t_rdoor_o) ||
      (ter_at_pos == t_door_frame) || (ter_at_pos == t_mdoor_frame) ||
      (ter_at_pos == t_fencegate_o) || (ter_at_pos == t_chaingate_o) ||
      (ter_at_pos == t_door_metal_o) || (ter_at_pos == t_door_bar_o) )) );

    if (has_trait("PARKOUR") && movecost > 100 ) {
        movecost *= .5f;
        if (movecost < 100)
            movecost = 100;
    }
    if (has_trait("BADKNEES") && movecost > 100 ) {
        movecost *= 1.25f;
        if (movecost < 100)
            movecost = 100;
    }

    if (hp_cur[hp_leg_l] == 0) {
        movecost += 50;
    }
    else if (hp_cur[hp_leg_l] < hp_max[hp_leg_l] * .40) {
        movecost += 25;
    }
    if (hp_cur[hp_leg_r] == 0) {
        movecost += 50;
    }
    else if (hp_cur[hp_leg_r] < hp_max[hp_leg_r] * .40) {
        movecost += 25;
    }

    if (has_trait("FLEET") && flatground) {
        movecost *= .85f;
    }
    if (has_trait("FLEET2") && flatground) {
        movecost *= .7f;
    }
    if (has_trait("SLOWRUNNER") && flatground) {
        movecost *= 1.15f;
    }
    if (has_trait("PADDED_FEET") && !wearing_something_on(bp_feet)) {
        movecost *= .9f;
    }
    if (has_trait("LIGHT_BONES")) {
        movecost *= .9f;
    }
    if (has_trait("HOLLOW_BONES")) {
        movecost *= .8f;
    }
    if (has_trait("WINGS_INSECT")) {
        movecost -= 15;
    }
    if (has_trait("WINGS_BUTTERFLY")) {
        movecost -= 10; // You can't fly, but you can make life easier on your legs
    }
    if (has_trait("LEG_TENTACLES")) {
        movecost += 20;
    }
    if (has_trait("FAT")) {
        movecost *= 1.05f;
    }
    if (has_trait("PONDEROUS1")) {
        movecost *= 1.1f;
    }
    if (has_trait("PONDEROUS2")) {
        movecost *= 1.2f;
    }
    if (has_trait("AMORPHOUS")) {
        movecost *= 1.25f;
    }
    if (has_trait("PONDEROUS3")) {
        movecost *= 1.3f;
    }
    if (is_wearing("swim_fins")) {
        movecost *= 1.5f;
    }
    if (is_wearing("roller_blades")) {
        if (offroading) {
            movecost *= 1.5f;
        } else if (flatground) {
            movecost *= 0.5f;
        } else {
            movecost *= 1.5f;
        }
    }

    movecost += encumb(bp_mouth) * 5 + encumb(bp_feet) * 5 + encumb(bp_legs) * 3;

    // ROOTS3 does slow you down as your roots are probing around for nutrients,
    // whether you want them to or not.  ROOTS1 is just too squiggly without shoes
    // to give you some stability.  Plants are a bit of a slow-mover.  Deal.
    if (!is_wearing_shoes() && !has_trait("PADDED_FEET") && !has_trait("HOOVES") &&
        !has_trait("TOUGH_FEET") && !has_trait("ROOTS2") ) {
        movecost += 15;
    }

    if( !wearing_something_on(bp_feet) && has_trait("ROOTS3") &&
        g->m.has_flag("DIGGABLE", posx, posy) ) {
        movecost += 10;
    }

    if (diag) {
        movecost *= 1.4142;
    }

    return int(movecost);
}

int player::swim_speed()
{
  int ret = 440 + weight_carried() / 60 - 50 * skillLevel("swimming");
  if (has_trait("PAWS")) {
      ret -= 20 + str_cur * 3;
  }
  if (has_trait("PAWS_LARGE")) {
      ret -= 20 + str_cur * 4;
  }
  if (is_wearing("swim_fins")) {
      ret -= (10 * str_cur) * 1.5;
  }
  if (has_trait("WEBBED")) {
      ret -= 60 + str_cur * 5;
  }
  if (has_trait("TAIL_FIN")) {
      ret -= 100 + str_cur * 10;
  }
  if (has_trait("SLEEK_SCALES")) {
      ret -= 100;
  }
  if (has_trait("LEG_TENTACLES")) {
      ret -= 60;
  }
  if (has_trait("FAT")) {
      ret -= 30;
  }
 ret += (50 - skillLevel("swimming") * 2) * encumb(bp_legs);
 ret += (80 - skillLevel("swimming") * 3) * encumb(bp_torso);
 if (skillLevel("swimming") < 10) {
  for (int i = 0; i < worn.size(); i++)
    ret += (worn[i].volume() * (10 - skillLevel("swimming"))) / 2;
 }
 ret -= str_cur * 6 + dex_cur * 4;
 if( worn_with_flag("FLOATATION") ) {
     ret = std::max(ret, 400);
     ret = std::min(ret, 200);
 }
// If (ret > 500), we can not swim; so do not apply the underwater bonus.
 if (underwater && ret < 500)
  ret -= 50;
 if (ret < 30)
  ret = 30;
 return ret;
}

bool player::digging() {
    return false;
}

bool player::is_on_ground()
{
    bool on_ground = false;
    if(has_effect("downed") || hp_cur[hp_leg_l] == 0 || hp_cur[hp_leg_r] == 0 ){
        on_ground = true;
    }
    return  on_ground;
}

bool player::is_underwater() const
{
    return underwater;
}

bool player::is_hallucination() const
{
    return false;
}

void player::set_underwater(bool u)
{
    if (underwater != u) {
        underwater = u;
        recalc_sight_limits();
    }
}


nc_color player::color()
{
 if (has_effect("onfire"))
  return c_red;
 if (has_effect("stunned"))
  return c_ltblue;
 if (has_disease("boomered"))
  return c_pink;
 if (underwater)
  return c_blue;
 if (has_active_bionic("bio_cloak") || has_artifact_with(AEP_INVISIBLE) ||
    (is_wearing("optical_cloak") && (has_active_item("UPS_on") ||
    has_active_item("adv_UPS_on"))) || has_trait("DEBUG_CLOAK"))
  return c_dkgray;
 return c_white;
}

void player::load_info(std::string data)
{
    std::stringstream dump;
    dump << data;

    char check = dump.peek();
    if ( check == ' ' ) {
        // sigh..
        check = data[1];
    }
    if ( check == '{' ) {
        JsonIn jsin(dump);
        try {
            deserialize(jsin);
        } catch (std::string jsonerr) {
            debugmsg("Bad player json\n%s", jsonerr.c_str() );
        }
        return;
    } else { // old save
        load_legacy(dump);
    }
}

std::string player::save_info()
{
    std::stringstream dump;
    dump << serialize(); // saves contents
    dump << std::endl;
    dump << dump_memorial();
    return dump.str();
}

void player::memorial( std::ofstream &memorial_file, std::string epitaph )
{
    //Size of indents in the memorial file
    const std::string indent = "  ";

    const std::string pronoun = male ? _("He") : _("She");

    //Avoid saying "a male unemployed" or similar
    std::stringstream profession_name;
    if(prof == prof->generic()) {
        if (male) {
            profession_name << _("an unemployed male");
        } else {
            profession_name << _("an unemployed female");
        }
    } else {
        profession_name << _("a ") << prof->gender_appropriate_name(male);
    }

    //Figure out the location
    const oter_id &cur_ter = overmap_buffer.ter(g->om_global_location());
    point cur_loc = g->om_location();
    std::string tername = otermap[cur_ter].name;

    //Were they in a town, or out in the wilderness?
    int city_index = g->cur_om->closest_city(cur_loc);
    std::stringstream city_name;
    if(city_index < 0) {
        city_name << _("in the middle of nowhere");
    } else {
        city nearest_city = g->cur_om->cities[city_index];
        //Give slightly different messages based on how far we are from the middle
        int distance_from_city = abs(g->cur_om->dist_from_city(cur_loc));
        if(distance_from_city > nearest_city.s + 4) {
            city_name << _("in the wilderness");
        } else if(distance_from_city >= nearest_city.s) {
            city_name << _("on the outskirts of ") << nearest_city.name;
        } else {
            city_name << _("in ") << nearest_city.name;
        }
    }

    //Header
    std::string version = string_format("%s", getVersionString());
    memorial_file << _("Cataclysm - Dark Days Ahead version ") << version << _(" memorial file") << "\n";
    memorial_file << "\n";
    memorial_file << _("In memory of: ") << name << "\n";
    if(epitaph.length() > 0) { //Don't record empty epitaphs
        memorial_file << "\"" << epitaph << "\"" << "\n\n";
    }
    memorial_file << pronoun << _(" was ") << profession_name.str()
                  << _(" when the apocalypse began.") << "\n";
    memorial_file << pronoun << _(" died on ") << season_name[calendar::turn.get_season()]
                  << _(" of year ") << (calendar::turn.years() + 1)
                  << _(", day ") << (calendar::turn.days() + 1)
                  << _(", at ") << calendar::turn.print_time() << ".\n";
    memorial_file << pronoun << _(" was killed in a ") << tername << " " << city_name.str() << ".\n";
    memorial_file << "\n";

    //Misc
    memorial_file << _("Cash on hand: ") << "$" << cash << "\n";
    memorial_file << "\n";

    //HP
    memorial_file << _("Final HP:") << "\n";
    memorial_file << indent << _(" Head: ") << hp_cur[hp_head] << "/" << hp_max[hp_head] << "\n";
    memorial_file << indent << _("Torso: ") << hp_cur[hp_torso] << "/" << hp_max[hp_torso] << "\n";
    memorial_file << indent << _("L Arm: ") << hp_cur[hp_arm_l] << "/" << hp_max[hp_arm_l] << "\n";
    memorial_file << indent << _("R Arm: ") << hp_cur[hp_arm_r] << "/" << hp_max[hp_arm_r] << "\n";
    memorial_file << indent << _("L Leg: ") << hp_cur[hp_leg_l] << "/" << hp_max[hp_leg_l] << "\n";
    memorial_file << indent << _("R Leg: ") << hp_cur[hp_leg_r] << "/" << hp_max[hp_leg_r] << "\n";
    memorial_file << "\n";

    //Stats
    memorial_file << _("Final Stats:") << "\n";
    memorial_file << indent << _("Str ") << str_cur << indent << _("Dex ") << dex_cur << indent
                  << _("Int ") << int_cur << indent << _("Per ") << per_cur << "\n";
    memorial_file << _("Base Stats:") << "\n";
    memorial_file << indent << _("Str ") << str_max << indent << _("Dex ") << dex_max << indent
                  << _("Int ") << int_max << indent << _("Per ") << per_max << "\n";
    memorial_file << "\n";

    //Last 20 messages
    memorial_file << _("Final Messages:") << "\n";
    std::vector<std::pair<std::string, std::string> > recent_messages = Messages::recent_messages(20);
    for( size_t i = 0; i < recent_messages.size(); ++i ) {
      memorial_file << indent << recent_messages[i].first << " " << recent_messages[i].second;
      memorial_file << "\n";
    }
    memorial_file << "\n";

    //Kill list
    memorial_file << _("Kills:") << "\n";

    int total_kills = 0;

    const std::map<std::string, mtype*> monids = MonsterGenerator::generator().get_all_mtypes();
    for (std::map<std::string, mtype*>::const_iterator mon = monids.begin(); mon != monids.end(); ++mon){
        if (g->kill_count(mon->first) > 0){
            memorial_file << "  " << mon->second->sym << " - " << string_format("%4d",g->kill_count(mon->first)) << " " << mon->second->nname(g->kill_count(mon->first)) << "\n";
            total_kills += g->kill_count(mon->first);
        }
    }
    if(total_kills == 0) {
      memorial_file << indent << _("No monsters were killed.") << "\n";
    } else {
      memorial_file << _("Total kills: ") << total_kills << "\n";
    }
    memorial_file << "\n";

    //Skills
    memorial_file << _("Skills:") << "\n";
    for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
      aSkill != Skill::skills.end(); ++aSkill) {
      SkillLevel next_skill_level = skillLevel(*aSkill);
      memorial_file << indent << (*aSkill)->name() << ": "
              << next_skill_level.level() << " (" << next_skill_level.exercise() << "%)\n";
    }
    memorial_file << "\n";

    //Traits
    memorial_file << _("Traits:") << "\n";
    bool had_trait = false;
    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
      if(has_trait(iter->first)) {
        had_trait = true;
        memorial_file << indent << traits[iter->first].name << "\n";
      }
    }
    if(!had_trait) {
      memorial_file << indent << _("(None)") << "\n";
    }
    memorial_file << "\n";

    //Effects (illnesses)
    memorial_file << _("Ongoing Effects:") << "\n";
    bool had_effect = false;
    for( size_t i = 0; i < illness.size(); ++i ) {
      disease next_illness = illness[i];
      if(dis_name(next_illness).size() > 0) {
        had_effect = true;
        memorial_file << indent << dis_name(next_illness) << "\n";
      }
    }
    //Various effects not covered by the illness list - from player.cpp
    if(morale_level() >= 100) {
      had_effect = true;
      memorial_file << indent << _("Elated") << "\n";
    }
    if(morale_level() <= -100) {
      had_effect = true;
      memorial_file << indent << _("Depressed") << "\n";
    }
    if(pain - pkill > 0) {
      had_effect = true;
      memorial_file << indent << _("Pain") << " (" << (pain - pkill) << ")";
    }
    if(stim > 0) {
      had_effect = true;
      int dexbonus = int(stim / 10);
      if (abs(stim) >= 30) {
        dexbonus -= int(abs(stim - 15) /  8);
      }
      if(dexbonus < 0) {
        memorial_file << indent << _("Stimulant Overdose") << "\n";
      } else {
        memorial_file << indent << _("Stimulant") << "\n";
      }
    } else if(stim < 0) {
      had_effect = true;
      memorial_file << indent << _("Depressants") << "\n";
    }
    if(!had_effect) {
      memorial_file << indent << _("(None)") << "\n";
    }
    memorial_file << "\n";

    //Bionics
    memorial_file << _("Bionics:") << "\n";
    int total_bionics = 0;
    for( size_t i = 0; i < my_bionics.size(); ++i ) {
      bionic_id next_bionic_id = my_bionics[i].id;
      memorial_file << indent << (i+1) << ": " << bionics[next_bionic_id]->name << "\n";
      total_bionics++;
    }
    if(total_bionics == 0) {
      memorial_file << indent << _("No bionics were installed.") << "\n";
    } else {
      memorial_file << _("Total bionics: ") << total_bionics << "\n";
    }
    memorial_file << _("Power: ") << power_level << "/" << max_power_level << "\n";
    memorial_file << "\n";

    //Equipment
    memorial_file << _("Weapon:") << "\n";
    memorial_file << indent << weapon.invlet << " - " << weapon.tname() << "\n";
    memorial_file << "\n";

    memorial_file << _("Equipment:") << "\n";
    for( size_t i = 0; i < worn.size(); ++i ) {
      item next_item = worn[i];
      memorial_file << indent << next_item.invlet << " - " << next_item.tname();
      if(next_item.charges > 0) {
        memorial_file << " (" << next_item.charges << ")";
      } else if (next_item.contents.size() == 1
              && next_item.contents[0].charges > 0) {
        memorial_file << " (" << next_item.contents[0].charges << ")";
      }
      memorial_file << "\n";
    }
    memorial_file << "\n";

    //Inventory
    memorial_file << _("Inventory:") << "\n";
    inv.restack(this);
    inv.sort();
    invslice slice = inv.slice();
    for( size_t i = 0; i < slice.size(); ++i ) {
      item& next_item = slice[i]->front();
      memorial_file << indent << next_item.invlet << " - " << next_item.tname();
      if(slice[i]->size() > 1) {
        memorial_file << " [" << slice[i]->size() << "]";
      }
      if(next_item.charges > 0) {
        memorial_file << " (" << next_item.charges << ")";
      } else if (next_item.contents.size() == 1
              && next_item.contents[0].charges > 0) {
        memorial_file << " (" << next_item.contents[0].charges << ")";
      }
      memorial_file << "\n";
    }
    memorial_file << "\n";

    //Lifetime stats
    memorial_file << _("Lifetime Stats") << "\n";
    memorial_file << indent << _("Distance Walked: ")
                       << player_stats.squares_walked << _(" Squares") << "\n";
    memorial_file << indent << _("Damage Taken: ")
                       << player_stats.damage_taken << _(" Damage") << "\n";
    memorial_file << indent << _("Damage Healed: ")
                       << player_stats.damage_healed << _(" Damage") << "\n";
    memorial_file << indent << _("Headshots: ")
                       << player_stats.headshots << "\n";
    memorial_file << "\n";

    //History
    memorial_file << _("Game History") << "\n";
    memorial_file << dump_memorial();

}

/**
 * Adds an event to the memorial log, to be written to the memorial file when
 * the character dies. The message should contain only the informational string,
 * as the timestamp and location will be automatically prepended.
 */
void player::add_memorial_log(const char* male_msg, const char* female_msg, ...)
{

    va_list ap;

    va_start(ap, female_msg);
    std::string msg;
    if(this->male) {
        msg = vstring_format(male_msg, ap);
    } else {
        msg = vstring_format(female_msg, ap);
    }
    va_end(ap);

    if(msg.empty()) {
        return;
    }

    std::stringstream timestamp;
    timestamp << _("Year") << " " << (calendar::turn.years() + 1) << ", "
              << season_name[calendar::turn.get_season()] << " "
              << (calendar::turn.days() + 1) << ", " << calendar::turn.print_time();

    const oter_id &cur_ter = overmap_buffer.ter(g->om_global_location());
    std::string location = otermap[cur_ter].name;

    std::stringstream log_message;
    log_message << "| " << timestamp.str() << " | " << location.c_str() << " | " << msg;

    memorial_log.push_back(log_message.str());

}

/**
 * Loads the data in a memorial file from the given ifstream. All the memorial
 * entry lines begin with a pipe (|).
 * @param fin The ifstream to read the memorial entries from.
 */
void player::load_memorial_file(std::ifstream &fin)
{
  std::string entry;
  memorial_log.clear();
  while(fin.peek() == '|') {
    getline(fin, entry);
    memorial_log.push_back(entry);
  }
}

/**
 * Concatenates all of the memorial log entries, delimiting them with newlines,
 * and returns the resulting string. Used for saving and for writing out to the
 * memorial file.
 */
std::string player::dump_memorial()
{

  std::stringstream output;

  for( size_t i = 0; i < memorial_log.size(); ++i ) {
    output << memorial_log[i] << "\n";
  }

  return output.str();

}

/**
 * Returns a pointer to the stat-tracking struct. Its fields should be edited
 * as necessary to track ongoing counters, which will be added to the memorial
 * file. For single events, rather than cumulative counters, see
 * add_memorial_log.
 * @return A pointer to the stats struct being used to track this player's
 *         lifetime stats.
 */
stats* player::lifetime_stats()
{
    return &player_stats;
}

// copy of stats, for saving
stats player::get_stats() const
{
    return player_stats;
}

void player::mod_stat( std::string stat, int modifier )
{
    if( stat == "hunger" ) {
        hunger += modifier;
    } else if( stat == "thirst" ) {
        thirst += modifier;
    } else if( stat == "fatigue" ) {
        fatigue += modifier;
    } else if( stat == "health" ) {
        health += modifier;
    } else if( stat == "oxygen" ) {
        oxygen += modifier;
    } else {
        // Fall through to the creature method.
        Creature::mod_stat( stat, modifier );
    }
}

inline bool skill_display_sort(const std::pair<Skill *, int> &a, const std::pair<Skill *, int> &b)
{
    int levelA = a.second;
    int levelB = b.second;
    return levelA > levelB || (levelA == levelB && a.first->name() < b.first->name());
}

void player::disp_info()
{
    int line;
    std::vector<std::string> effect_name;
    std::vector<std::string> effect_text;
    for (size_t i = 0; i < illness.size(); i++) {
        if (dis_name(illness[i]).size() > 0) {
            effect_name.push_back(dis_name(illness[i]));
            effect_text.push_back(dis_description(illness[i]));
        }
    }
    for( auto effect_it = effects.begin(); effect_it != effects.end(); ++effect_it) {
        effect_name.push_back( effect_it->second.disp_name() );
        effect_text.push_back( effect_it->second.get_effect_type()->get_desc() );
    }
    if (abs(morale_level()) >= 100) {
        bool pos = (morale_level() > 0);
        effect_name.push_back(pos ? _("Elated") : _("Depressed"));
        std::stringstream morale_text;
        if (abs(morale_level()) >= 200) {
            morale_text << _("Dexterity") << (pos ? " +" : " ") <<
                int(morale_level() / 200) << "   ";
        }
        if (abs(morale_level()) >= 180) {
            morale_text << _("Strength") << (pos ? " +" : " ") <<
                int(morale_level() / 180) << "   ";
        }
        if (abs(morale_level()) >= 125) {
            morale_text << _("Perception") << (pos ? " +" : " ") <<
                int(morale_level() / 125) << "   ";
        }
        morale_text << _("Intelligence") << (pos ? " +" : " ") <<
            int(morale_level() / 100) << "   ";
        effect_text.push_back(morale_text.str());
    }
    if (pain - pkill > 0) {
        effect_name.push_back(_("Pain"));
        std::stringstream pain_text;
        // Cenobites aren't markedly physically impaired by pain.
        if ((pain - pkill >= 15) && (!(has_trait("CENOBITE")))) {
            pain_text << "Strength" << " -" << int((pain - pkill) / 15) << "   " << _("Dexterity") << " -" <<
                int((pain - pkill) / 15) << "   ";
        }
        // They do find the sensations distracting though.
        // Pleasurable...but distracting.
        if (pain - pkill >= 20) {
            pain_text << _("Perception") << " -" << int((pain - pkill) / 15) << "   ";
        }
        pain_text << _("Intelligence") << " -" << 1 + int((pain - pkill) / 25);
        effect_text.push_back(pain_text.str());
    }
    if (stim > 0) {
        int dexbonus = int(stim / 10);
        int perbonus = int(stim /  7);
        int intbonus = int(stim /  6);
        if (abs(stim) >= 30) {
            dexbonus -= int(abs(stim - 15) /  8);
            perbonus -= int(abs(stim - 15) / 12);
            intbonus -= int(abs(stim - 15) / 14);
        }

        if (dexbonus < 0) {
            effect_name.push_back(_("Stimulant Overdose"));
        } else {
            effect_name.push_back(_("Stimulant"));
        }
        std::stringstream stim_text;
        stim_text << _("Speed") << " +" << stim << "   " << _("Intelligence") <<
            (intbonus > 0 ? " + " : " ") << intbonus << "   " << _("Perception") <<
            (perbonus > 0 ? " + " : " ") << perbonus << "   " << _("Dexterity")  <<
            (dexbonus > 0 ? " + " : " ") << dexbonus;
        effect_text.push_back(stim_text.str());
    } else if (stim < 0) {
        effect_name.push_back(_("Depressants"));
        std::stringstream stim_text;
        int dexpen = int(stim / 10);
        int perpen = int(stim /  7);
        int intpen = int(stim /  6);
        // Since dexpen etc. are always less than 0, no need for + signs
        stim_text << _("Speed") << " " << stim << "   " << _("Intelligence") << " " << intpen <<
            "   " << _("Perception") << " " << perpen << "   " << "Dexterity" << " " << dexpen;
        effect_text.push_back(stim_text.str());
    }

    if ((has_trait("TROGLO") && g->is_in_sunlight(posx, posy) &&
         g->weather == WEATHER_SUNNY) ||
        (has_trait("TROGLO2") && g->is_in_sunlight(posx, posy) &&
         g->weather != WEATHER_SUNNY)) {
        effect_name.push_back(_("In Sunlight"));
        effect_text.push_back(_("The sunlight irritates you.\n\
Strength - 1;    Dexterity - 1;    Intelligence - 1;    Perception - 1"));
    } else if (has_trait("TROGLO2") && g->is_in_sunlight(posx, posy)) {
        effect_name.push_back(_("In Sunlight"));
        effect_text.push_back(_("The sunlight irritates you badly.\n\
Strength - 2;    Dexterity - 2;    Intelligence - 2;    Perception - 2"));
    } else if (has_trait("TROGLO3") && g->is_in_sunlight(posx, posy)) {
        effect_name.push_back(_("In Sunlight"));
        effect_text.push_back(_("The sunlight irritates you terribly.\n\
Strength - 4;    Dexterity - 4;    Intelligence - 4;    Perception - 4"));
    }

    for (size_t i = 0; i < addictions.size(); i++) {
        if (addictions[i].sated < 0 &&
            addictions[i].intensity >= MIN_ADDICTION_LEVEL) {
            effect_name.push_back(addiction_name(addictions[i]));
            effect_text.push_back(addiction_text(addictions[i]));
        }
    }

    int maxy = TERMY;

    int infooffsetytop = 11;
    int infooffsetybottom = 15;
    std::vector<std::string> traitslist;

    for ( auto iter = my_mutations.begin(); iter != my_mutations.end(); ++iter) {
        traitslist.push_back(*iter);
    }

    int effect_win_size_y = 1 + effect_name.size();
    int trait_win_size_y = 1 + traitslist.size();
    int skill_win_size_y = 1 + Skill::skill_count();

    if (trait_win_size_y + infooffsetybottom > maxy) {
        trait_win_size_y = maxy - infooffsetybottom;
    }

    if (skill_win_size_y + infooffsetybottom > maxy) {
        skill_win_size_y = maxy - infooffsetybottom;
    }

    WINDOW* w_grid_top    = newwin(infooffsetybottom, FULL_SCREEN_WIDTH+1, VIEW_OFFSET_Y, VIEW_OFFSET_X);
    WINDOW* w_grid_skill  = newwin(skill_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X);
    WINDOW* w_grid_trait  = newwin(trait_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y, 27 + VIEW_OFFSET_X);
    WINDOW* w_grid_effect = newwin(effect_win_size_y+ 1, 28, infooffsetybottom + VIEW_OFFSET_Y, 53 + VIEW_OFFSET_X);

    WINDOW* w_tip     = newwin(1, FULL_SCREEN_WIDTH,  VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);
    WINDOW* w_stats   = newwin(9, 26,  1 + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);
    WINDOW* w_traits  = newwin(trait_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y,  27 + VIEW_OFFSET_X);
    WINDOW* w_encumb  = newwin(9, 26,  1 + VIEW_OFFSET_Y, 27 + VIEW_OFFSET_X);
    WINDOW* w_effects = newwin(effect_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y, 54 + VIEW_OFFSET_X);
    WINDOW* w_speed   = newwin(9, 26,  1 + VIEW_OFFSET_Y, 54 + VIEW_OFFSET_X);
    WINDOW* w_skills  = newwin(skill_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X);
    WINDOW* w_info    = newwin(3, FULL_SCREEN_WIDTH, infooffsetytop + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);

    for (int i = 0; i < FULL_SCREEN_WIDTH+1; i++) {
        //Horizontal line top grid
        mvwputch(w_grid_top, 10, i, BORDER_COLOR, LINE_OXOX);
        mvwputch(w_grid_top, 14, i, BORDER_COLOR, LINE_OXOX);

        //Vertical line top grid
        if (i <= infooffsetybottom) {
            mvwputch(w_grid_top, i, 26, BORDER_COLOR, LINE_XOXO);
            mvwputch(w_grid_top, i, 53, BORDER_COLOR, LINE_XOXO);
            mvwputch(w_grid_top, i, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXO);
        }

        //Horizontal line skills
        if (i <= 26) {
            mvwputch(w_grid_skill, skill_win_size_y, i, BORDER_COLOR, LINE_OXOX);
        }

        //Vertical line skills
        if (i <= skill_win_size_y) {
            mvwputch(w_grid_skill, i, 26, BORDER_COLOR, LINE_XOXO);
        }

        //Horizontal line traits
        if (i <= 26) {
            mvwputch(w_grid_trait, trait_win_size_y, i, BORDER_COLOR, LINE_OXOX);
        }

        //Vertical line traits
        if (i <= trait_win_size_y) {
            mvwputch(w_grid_trait, i, 26, BORDER_COLOR, LINE_XOXO);
        }

        //Horizontal line effects
        if (i <= 27) {
            mvwputch(w_grid_effect, effect_win_size_y, i, BORDER_COLOR, LINE_OXOX);
        }

        //Vertical line effects
        if (i <= effect_win_size_y) {
            mvwputch(w_grid_effect, i, 0, BORDER_COLOR, LINE_XOXO);
            mvwputch(w_grid_effect, i, 27, BORDER_COLOR, LINE_XOXO);
        }
    }

    //Intersections top grid
    mvwputch(w_grid_top, 14, 26, BORDER_COLOR, LINE_OXXX); // T
    mvwputch(w_grid_top, 14, 53, BORDER_COLOR, LINE_OXXX); // T
    mvwputch(w_grid_top, 10, 26, BORDER_COLOR, LINE_XXOX); // _|_
    mvwputch(w_grid_top, 10, 53, BORDER_COLOR, LINE_XXOX); // _|_
    mvwputch(w_grid_top, 10, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXX); // -|
    mvwputch(w_grid_top, 14, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXX); // -|
    wrefresh(w_grid_top);

    mvwputch(w_grid_skill, skill_win_size_y, 26, BORDER_COLOR, LINE_XOOX); // _|

    if (skill_win_size_y > trait_win_size_y)
        mvwputch(w_grid_skill, trait_win_size_y, 26, BORDER_COLOR, LINE_XXXO); // |-
    else if (skill_win_size_y == trait_win_size_y)
        mvwputch(w_grid_skill, trait_win_size_y, 26, BORDER_COLOR, LINE_XXOX); // _|_

    mvwputch(w_grid_trait, trait_win_size_y, 26, BORDER_COLOR, LINE_XOOX); // _|

    if (trait_win_size_y > effect_win_size_y) {
        mvwputch(w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXXO); // |-
    } else if (trait_win_size_y == effect_win_size_y) {
        mvwputch(w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXOX); // _|_
    } else if (trait_win_size_y < effect_win_size_y) {
        mvwputch(w_grid_trait, trait_win_size_y, 26, BORDER_COLOR, LINE_XOXX); // -|
        mvwputch(w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXOO); // |_
    }

    mvwputch(w_grid_effect, effect_win_size_y, 0, BORDER_COLOR, LINE_XXOO); // |_
    mvwputch(w_grid_effect, effect_win_size_y, 27, BORDER_COLOR, LINE_XOOX); // _|

    wrefresh(w_grid_skill);
    wrefresh(w_grid_effect);
    wrefresh(w_grid_trait);

    //-1 for header
    trait_win_size_y--;
    skill_win_size_y--;
    effect_win_size_y--;

    // Print name and header
    // Post-humanity trumps your pre-Cataclysm life.
    if (crossed_threshold()) {
        std::vector<std::string> traitslist;
        std::string race;
        for (size_t i = 0; i < traitslist.size(); i++) {
            if (mutation_data[traitslist[i]].threshold == true)
                race = traits[traitslist[i]].name;
        }
        //~ player info window: 1s - name, 2s - gender, 3s - Prof or Mutation name
        mvwprintw(w_tip, 0, 0, _("%1$s | %2$s | %3$s"), name.c_str(),
                  male ? _("Male") : _("Female"), race.c_str());
    } else if (prof == NULL || prof == prof->generic()) {
        // Regular person. Nothing interesting.
        //~ player info window: 1s - name, 2s - gender, '|' - field separator.
        mvwprintw(w_tip, 0, 0, _("%1$s | %2$s"), name.c_str(),
                  male ? _("Male") : _("Female"));
    } else {
        mvwprintw(w_tip, 0, 0, _("%1$s | %2$s | %3$s"), name.c_str(),
                  male ? _("Male") : _("Female"), prof->gender_appropriate_name(male).c_str());
    }

    input_context ctxt("PLAYER_INFO");
    ctxt.register_updown();
    ctxt.register_action("NEXT_TAB", _("Cycle to next category"));
    ctxt.register_action("QUIT");
    ctxt.register_action("CONFIRM", _("Toogle skill training"));
    ctxt.register_action("HELP_KEYBINDINGS");
    std::string action;

    std::string help_msg = string_format(_("Press %s for help."), ctxt.get_desc("HELP_KEYBINDINGS").c_str());
    mvwprintz(w_tip, 0, FULL_SCREEN_WIDTH - utf8_width(help_msg.c_str()), c_ltred, help_msg.c_str());
    help_msg.clear();
    wrefresh(w_tip);

    // First!  Default STATS screen.
    const char* title_STATS = _("STATS");
    mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, c_ltgray, title_STATS);
    mvwprintz(w_stats, 2, 1, c_ltgray, "                     ");
    mvwprintz(w_stats, 2, 1, c_ltgray, _("Strength:"));
    mvwprintz(w_stats, 2, 20, c_ltgray, str_max>9?"(%d)":" (%d)", str_max);
    mvwprintz(w_stats, 3, 1, c_ltgray, "                     ");
    mvwprintz(w_stats, 3, 1, c_ltgray, _("Dexterity:"));
    mvwprintz(w_stats, 3, 20, c_ltgray, dex_max>9?"(%d)":" (%d)", dex_max);
    mvwprintz(w_stats, 4, 1, c_ltgray, "                     ");
    mvwprintz(w_stats, 4, 1, c_ltgray, _("Intelligence:"));
    mvwprintz(w_stats, 4, 20, c_ltgray, int_max>9?"(%d)":" (%d)", int_max);
    mvwprintz(w_stats, 5, 1, c_ltgray, "                     ");
    mvwprintz(w_stats, 5, 1, c_ltgray, _("Perception:"));
    mvwprintz(w_stats, 5, 20, c_ltgray, per_max>9?"(%d)":" (%d)", per_max);

    nc_color status = c_white;

    if (str_cur <= 0)
        status = c_dkgray;
    else if (str_cur < str_max / 2)
        status = c_red;
    else if (str_cur < str_max)
        status = c_ltred;
    else if (str_cur == str_max)
        status = c_white;
    else if (str_cur < str_max * 1.5)
        status = c_ltgreen;
    else
        status = c_green;
    mvwprintz(w_stats,  2, (str_cur < 10 ? 17 : 16), status, "%d", str_cur);

    if (dex_cur <= 0)
        status = c_dkgray;
    else if (dex_cur < dex_max / 2)
        status = c_red;
    else if (dex_cur < dex_max)
        status = c_ltred;
    else if (dex_cur == dex_max)
        status = c_white;
    else if (dex_cur < dex_max * 1.5)
        status = c_ltgreen;
    else
        status = c_green;
    mvwprintz(w_stats,  3, (dex_cur < 10 ? 17 : 16), status, "%d", dex_cur);

    if (int_cur <= 0)
        status = c_dkgray;
    else if (int_cur < int_max / 2)
        status = c_red;
    else if (int_cur < int_max)
        status = c_ltred;
    else if (int_cur == int_max)
        status = c_white;
    else if (int_cur < int_max * 1.5)
        status = c_ltgreen;
    else
        status = c_green;
    mvwprintz(w_stats,  4, (int_cur < 10 ? 17 : 16), status, "%d", int_cur);

    if (per_cur <= 0)
        status = c_dkgray;
    else if (per_cur < per_max / 2)
        status = c_red;
    else if (per_cur < per_max)
        status = c_ltred;
    else if (per_cur == per_max)
        status = c_white;
    else if (per_cur < per_max * 1.5)
        status = c_ltgreen;
    else
        status = c_green;
    mvwprintz(w_stats,  5, (per_cur < 10 ? 17 : 16), status, "%d", per_cur);

    wrefresh(w_stats);

    // Next, draw encumberment.
    std::string asText[] = {_("Torso"), _("Head"), _("Eyes"), _("Mouth"),
                            _("Arms"), _("Hands"), _("Legs"), _("Feet")};
    body_part aBodyPart[] = {bp_torso, bp_head, bp_eyes, bp_mouth, bp_arms, bp_hands, bp_legs, bp_feet};
    int iEnc, iArmorEnc, iWarmth;
    double iLayers;

    const char *title_ENCUMB = _("ENCUMBRANCE AND WARMTH");
    mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB) / 2, c_ltgray, title_ENCUMB);
    for (int i = 0; i < 8; i++) {
        iLayers = iArmorEnc = 0;
        iWarmth = warmth(body_part(i));
        iEnc = encumb(aBodyPart[i], iLayers, iArmorEnc);
        mvwprintz(w_encumb, i + 1, 1, c_ltgray, "%s:", asText[i].c_str());
        mvwprintz(w_encumb, i + 1, 8, c_ltgray, "(%d)", iLayers);
        mvwprintz(w_encumb, i + 1, 11, c_ltgray, "%*s%d%s%d=", (iArmorEnc < 0 || iArmorEnc > 9 ? 1 : 2),
                  " ", iArmorEnc, "+", iEnc - iArmorEnc);
        wprintz(w_encumb, encumb_color(iEnc), "%s%d", (iEnc < 0 || iEnc > 9 ? "" : " ") , iEnc);
        wprintz(w_encumb, bodytemp_color(i), " (%3d)", iWarmth);
    }
    wrefresh(w_encumb);

    // Next, draw traits.
    const char *title_TRAITS = _("TRAITS");
    mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, c_ltgray, title_TRAITS);
    std::sort(traitslist.begin(), traitslist.end(), trait_display_sort);
    for (int i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
        if (mutation_data[traitslist[i]].threshold == true)
            status = c_white;
        else if (traits[traitslist[i]].mixed_effect == true)
            status = c_pink;
        else if (traits[traitslist[i]].points > 0)
            status = c_ltgreen;
        else if (traits[traitslist[i]].points < 0)
            status = c_ltred;
        else
            status = c_yellow;
        mvwprintz(w_traits, i+1, 1, status, traits[traitslist[i]].name.c_str());
    }
    wrefresh(w_traits);

    // Next, draw effects.
    const char *title_EFFECTS = _("EFFECTS");
    mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, c_ltgray, title_EFFECTS);
    for (int i = 0; i < effect_name.size() && i < effect_win_size_y; i++) {
        mvwprintz(w_effects, i+1, 0, c_ltgray, "%s", effect_name[i].c_str());
    }
    wrefresh(w_effects);

    // Next, draw skills.
    line = 1;
    std::vector<Skill*> skillslist;
    const char *title_SKILLS = _("SKILLS");
    mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, c_ltgray, title_SKILLS);

    std::vector<std::pair<Skill *, int> > sorted;
    int num_skills = Skill::skills.size();
    for (int i = 0; i < num_skills; i++) {
        Skill *s = Skill::skills[i];
        SkillLevel &sl = skillLevel(s);
        sorted.push_back(std::pair<Skill *, int>(s, sl.level() * 100 + sl.exercise()));
    }
    std::sort(sorted.begin(), sorted.end(), skill_display_sort);
    for (std::vector<std::pair<Skill *, int> >::iterator i = sorted.begin(); i != sorted.end(); ++i) {
        skillslist.push_back((*i).first);
    }

    for (auto aSkill = skillslist.begin(); aSkill != skillslist.end(); ++aSkill) {
        SkillLevel level = skillLevel(*aSkill);

        // Default to not training and not rusting
        nc_color text_color = c_blue;
        bool training = level.isTraining();
        bool rusting = level.isRusting();

        if(training && rusting) {
            text_color = c_ltred;
        } else if (training) {
            text_color = c_ltblue;
        } else if (rusting) {
            text_color = c_red;
        }

        int level_num = (int)level;
        int exercise = level.exercise();

        if (has_active_bionic("bio_cqb") &&
        ((*aSkill)->ident() == "melee" || (*aSkill)->ident() == "unarmed" ||
         (*aSkill)->ident() == "cutting" || (*aSkill)->ident() == "bashing" ||
         (*aSkill)->ident() == "stabbing")) {
            level_num = 5;
            exercise = 0;
            text_color = c_yellow;
        }

        if (line < skill_win_size_y + 1) {
            mvwprintz(w_skills, line, 1, text_color, "%s:", (*aSkill)->name().c_str());
            mvwprintz(w_skills, line, 19, text_color, "%-2d(%2d%%)", level_num,
                      (exercise <  0 ? 0 : exercise));
            line++;
        }
    }
    wrefresh(w_skills);

    // Finally, draw speed.
    const char *title_SPEED = _("SPEED");
    mvwprintz(w_speed, 0, 13 - utf8_width(title_SPEED)/2, c_ltgray, title_SPEED);
    mvwprintz(w_speed, 1,  1, c_ltgray, _("Base Move Cost:"));
    mvwprintz(w_speed, 2,  1, c_ltgray, _("Current Speed:"));
    int newmoves = get_speed();
    int pen = 0;
    line = 3;
    if (weight_carried() > weight_capacity()) {
        pen = 25 * (weight_carried() - weight_capacity()) / (weight_capacity());
        mvwprintz(w_speed, line, 1, c_red, _("Overburdened        -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    pen = int(morale_level() / 25);
    if (abs(pen) >= 4) {
        if (pen > 10)
            pen = 10;
        else if (pen < -10)
            pen = -10;
        if (pen > 0)
            mvwprintz(w_speed, line, 1, c_green, _("Good mood           +%s%d%%"),
                      (pen < 10 ? " " : ""), pen);
        else
            mvwprintz(w_speed, line, 1, c_red, _("Depressed           -%s%d%%"),
                      (abs(pen) < 10 ? " " : ""), abs(pen));
        line++;
    }
    pen = int((pain - pkill) * .7);
    if (has_trait("CENOBITE")) {
        pen /= 4;
    }
    if (pen > 60)
        pen = 60;
    if (pen >= 1) {
        mvwprintz(w_speed, line, 1, c_red, _("Pain                -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if (pkill >= 10) {
        pen = int(pkill * .1);
        mvwprintz(w_speed, line, 1, c_red, _("Painkillers         -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if (stim != 0) {
        pen = stim;
        if (pen > 0)
            mvwprintz(w_speed, line, 1, c_green, _("Stimulants          +%s%d%%"),
                      (pen < 10 ? " " : ""), pen);
        else
            mvwprintz(w_speed, line, 1, c_red, _("Depressants         -%s%d%%"),
                      (abs(pen) < 10 ? " " : ""), abs(pen));
        line++;
    }
    if (thirst > 40) {
        pen = int((thirst - 40) / 10);
        mvwprintz(w_speed, line, 1, c_red, _("Thirst              -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if (hunger > 100) {
        pen = int((hunger - 100) / 10);
        mvwprintz(w_speed, line, 1, c_red, _("Hunger              -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if (has_trait("SUNLIGHT_DEPENDENT") && !g->is_in_sunlight(posx, posy)) {
        pen = (g->light_level() >= 12 ? 5 : 10);
        mvwprintz(w_speed, line, 1, c_red, _("Out of Sunlight     -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if (has_trait ("COLDBLOOD4") && g->get_temperature() > 65) {
        pen = int( (g->get_temperature() - 65) / 2);
        mvwprintz(w_speed, line, 1, c_green, _("Cold-Blooded        +%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if ((has_trait("COLDBLOOD") || has_trait("COLDBLOOD2") ||
         has_trait("COLDBLOOD3") || has_trait("COLDBLOOD4")) &&
        g->get_temperature() < 65) {
        if (has_trait("COLDBLOOD3") || has_trait("COLDBLOOD4")) {
            pen = int( (65 - g->get_temperature()) / 2);
        } else if (has_trait("COLDBLOOD2")) {
            pen = int( (65 - g->get_temperature()) / 3);
        } else {
            pen = int( (65 - g->get_temperature()) / 2);
        }
        mvwprintz(w_speed, line, 1, c_red, _("Cold-Blooded        -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }

    std::map<std::string, int> speed_effects;
    std::string dis_text = "";
    for (size_t i = 0; i < illness.size(); i++) {
        int move_adjust = disease_speed_boost(illness[i]);
        if (move_adjust != 0) {
            if (dis_combined_name(illness[i]) == "") {
                dis_text = dis_name(illness[i]);
            } else {
                dis_text = dis_combined_name(illness[i]);
            }
            speed_effects[dis_text] += move_adjust;
        }
    }

    for (auto it = speed_effects.begin(); it != speed_effects.end(); ++it) {
        nc_color col = (it->second > 0 ? c_green : c_red);
        mvwprintz(w_speed, line,  1, col, "%s", it->first.c_str());
        mvwprintz(w_speed, line, 21, col, (it->second > 0 ? "+" : "-"));
        mvwprintz(w_speed, line, (abs(it->second) >= 10 ? 22 : 23), col, "%d%%",
                  abs(it->second));
        line++;
    }

    if (has_trait("QUICK")) {
        pen = int(newmoves * .1);
        mvwprintz(w_speed, line, 1, c_green, _("Quick               +%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
    }
    if (has_bionic("bio_speed")) {
        pen = int(newmoves * .1);
        mvwprintz(w_speed, line, 1, c_green, _("Bionic Speed        +%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
    }
    int runcost = run_cost(100);
    nc_color col = (runcost <= 100 ? c_green : c_red);
    mvwprintz(w_speed, 1, (runcost  >= 100 ? 21 : (runcost  < 10 ? 23 : 22)), col,
              "%d", runcost);
    col = (newmoves >= 100 ? c_green : c_red);
    mvwprintz(w_speed, 2, (newmoves >= 100 ? 21 : (newmoves < 10 ? 23 : 22)), col,
              "%d", newmoves);
    wrefresh(w_speed);

    refresh();

    int curtab = 1;
    int min, max;
    line = 0;
    bool done = false;
    int half_y = 0;

    // Initial printing is DONE.  Now we give the player a chance to scroll around
    // and "hover" over different items for more info.
    do {
        werase(w_info);
        switch (curtab) {
            case 1: // Stats tab
                mvwprintz(w_stats, 0, 0, h_ltgray, _("                          "));
                mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, h_ltgray, title_STATS);
                if (line == 0) {
                    // display player current STR effects
                    mvwprintz(w_stats, 2, 1, h_ltgray, _("Strength:"));
                    mvwprintz(w_stats, 6, 1, c_magenta, _("Base HP: %d              "), hp_max[1]);
                    mvwprintz(w_stats, 7, 1, c_magenta, _("Carry weight: %.1f %s     "),
                              convert_weight(weight_capacity(false)),
                              OPTIONS["USE_METRIC_WEIGHTS"] == "kg"?_("kg"):_("lbs"));
                    mvwprintz(w_stats, 8, 1, c_magenta, _("Melee damage: %d         "),
                              base_damage(false));

                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Strength affects your melee damage, the amount of weight you can carry, your total HP, \
your resistance to many diseases, and the effectiveness of actions which require brute force."));
                } else if (line == 1) {
                    // display player current DEX effects
                    mvwprintz(w_stats, 3, 1, h_ltgray, _("Dexterity:"));
                    mvwprintz(w_stats, 6, 1, c_magenta, _("Melee to-hit bonus: +%d                      "),
                              base_to_hit(false));
                    mvwprintz(w_stats, 7, 1, c_magenta, "                                            ");
                    mvwprintz(w_stats, 7, 1, c_magenta, _("Ranged penalty: -%d"),
                              abs(ranged_dex_mod(false)));
                    mvwprintz(w_stats, 8, 1, c_magenta, "                                            ");
                    if (throw_dex_mod(false) <= 0) {
                        mvwprintz(w_stats, 8, 1, c_magenta, _("Throwing bonus: +%d"),
                        abs(throw_dex_mod(false)));
                    } else {
                        mvwprintz(w_stats, 8, 1, c_magenta, _("Throwing penalty: -%d"),
                        abs(throw_dex_mod(false)));
                    }
                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Dexterity affects your chance to hit in melee combat, helps you steady your \
gun for ranged combat, and enhances many actions that require finesse."));
                } else if (line == 2) {
                    // display player current INT effects
                    mvwprintz(w_stats, 4, 1, h_ltgray, _("Intelligence:"));
                    mvwprintz(w_stats, 6, 1, c_magenta, _("Read times: %d%%           "), read_speed(false));
                    mvwprintz(w_stats, 7, 1, c_magenta, _("Skill rust: %d%%           "), rust_rate(false));
                    mvwprintz(w_stats, 8, 1, c_magenta, _("Crafting Bonus: %d          "), int_cur);

                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Intelligence is less important in most situations, but it is vital for more complex tasks like \
electronics crafting. It also affects how much skill you can pick up from reading a book."));
                } else if (line == 3) {
                    // display player current PER effects
                    mvwprintz(w_stats, 5, 1, h_ltgray, _("Perception:"));
                    mvwprintz(w_stats, 6, 1,  c_magenta, _("Ranged penalty: -%d"),
                              abs(ranged_per_mod(false)),"          ");
                    mvwprintz(w_stats, 7, 1, c_magenta, _("Trap detection level: %d       "), per_cur);
                    mvwprintz(w_stats, 8, 1, c_magenta, "                             ");
                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Perception is the most important stat for ranged combat. It's also used for \
detecting traps and other things of interest."));
                }
                wrefresh(w_stats);
                wrefresh(w_info);

                action = ctxt.handle_input();
                if (action == "DOWN") {
                    line++;
                    if (line == 4)
                        line = 0;
                } else if (action == "UP") {
                    line--;
                    if (line == -1)
                        line = 3;
                } else if (action == "NEXT_TAB") {
                    mvwprintz(w_stats, 0, 0, c_ltgray, _("                          "));
                    mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, c_ltgray, title_STATS);
                    wrefresh(w_stats);
                    line = 0;
                    curtab++;
                } else if (action == "QUIT") {
                    done = true;
                }
            mvwprintz(w_stats, 2, 1, c_ltgray, _("Strength:"));
            mvwprintz(w_stats, 3, 1, c_ltgray, _("Dexterity:"));
            mvwprintz(w_stats, 4, 1, c_ltgray, _("Intelligence:"));
            mvwprintz(w_stats, 5, 1, c_ltgray, _("Perception:"));
            wrefresh(w_stats);
            break;
        case 2: // Encumberment tab
        {
            mvwprintz(w_encumb, 0, 0, h_ltgray,  _("                          "));
            mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB)/2, h_ltgray, title_ENCUMB);
            std::string s;
            if (line == 0) {
                mvwprintz(w_encumb, 1, 1, h_ltgray, _("Torso"));
                s = _("Melee skill %+d;");
                s+= _(" Dodge skill %+d;\n");
                s+= ngettext("Swimming costs %+d movement point;\n",
                             "Swimming costs %+d movement points;\n",
                             encumb(bp_torso) * (80 - skillLevel("swimming") * 3));
                s+= ngettext("Melee and thrown attacks cost %+d movement point.",
                             "Melee and thrown attacks cost %+d movement points.",
                             encumb(bp_torso) * 20);
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, s.c_str(),
                               -encumb(bp_torso), -encumb(bp_torso),
                               encumb(bp_torso) * (80 - skillLevel("swimming") * 3),
                               encumb(bp_torso) * 20);
            } else if (line == 1) {
                mvwprintz(w_encumb, 2, 1, h_ltgray, _("Head"));
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Head encumbrance has no effect; it simply limits how much you can put on."));
            } else if (line == 2) {
                mvwprintz(w_encumb, 3, 1, h_ltgray, _("Eyes"));
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Perception %+d when checking traps or firing ranged weapons;\n\
Perception %+.1f when throwing items."),
                               -encumb(bp_eyes),
                               double(double(-encumb(bp_eyes)) / 2));
            } else if (line == 3) {
                mvwprintz(w_encumb, 4, 1, h_ltgray, _("Mouth"));
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, ngettext("\
Running costs %+d movement point.", "Running costs %+d movement points.", encumb(bp_mouth) * 5), encumb(bp_mouth) * 5);
            } else if (line == 4) {
                mvwprintz(w_encumb, 5, 1, h_ltgray, _("Arms"));
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Arm encumbrance affects your accuracy with ranged weapons."));
            } else if (line == 5) {
                mvwprintz(w_encumb, 6, 1, h_ltgray, _("Hands"));
                s = ngettext("Reloading costs %+d movement point; ",
                             "Reloading costs %+d movement points; ",
                             encumb(bp_hands) * 30);
                s+= _("Dexterity %+d when throwing items.");
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                               s.c_str() , encumb(bp_hands) * 30, -encumb(bp_hands));
            } else if (line == 6) {
                mvwprintz(w_encumb, 7, 1, h_ltgray, _("Legs"));
                s = ngettext("Running costs %+d movement point; ",
                             "Running costs %+d movement points; ",
                             encumb(bp_legs) * 3);
                s+= ngettext("Swimming costs %+d movement point;\n",
                             "Swimming costs %+d movement points;\n",
                             encumb(bp_legs) *(50 - skillLevel("swimming") * 2));
                s+= _("Dodge skill %+.1f.");
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                               s.c_str(), encumb(bp_legs) * 3,
                               encumb(bp_legs) *(50 - skillLevel("swimming") * 2),
                               double(double(-encumb(bp_legs)) / 2));
            } else if (line == 7) {
                mvwprintz(w_encumb, 8, 1, h_ltgray, _("Feet"));
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                               ngettext("Running costs %+d movement point.", 
                                        "Running costs %+d movement points.",
                                        encumb(bp_feet) * 5),
                               encumb(bp_feet) * 5);
            }
            wrefresh(w_encumb);
            wrefresh(w_info);

            action = ctxt.handle_input();
            if (action == "DOWN") {
                line++;
                if (line == 8)
                    line = 0;
            } else if (action == "UP") {
                line--;
                if (line == -1)
                    line = 7;
            } else if (action == "NEXT_TAB") {
                mvwprintz(w_encumb, 0, 0, c_ltgray,  _("                          "));
                mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB)/2, c_ltgray, title_ENCUMB);
                wrefresh(w_encumb);
                line = 0;
                curtab++;
            } else if (action == "QUIT") {
                done = true;
            }
            mvwprintz(w_encumb, 1, 1, c_ltgray, _("Torso"));
            mvwprintz(w_encumb, 2, 1, c_ltgray, _("Head"));
            mvwprintz(w_encumb, 3, 1, c_ltgray, _("Eyes"));
            mvwprintz(w_encumb, 4, 1, c_ltgray, _("Mouth"));
            mvwprintz(w_encumb, 5, 1, c_ltgray, _("Arms"));
            mvwprintz(w_encumb, 6, 1, c_ltgray, _("Hands"));
            mvwprintz(w_encumb, 7, 1, c_ltgray, _("Legs"));
            mvwprintz(w_encumb, 8, 1, c_ltgray, _("Feet"));
            wrefresh(w_encumb);
            break;
        }
        case 4: // Traits tab
            mvwprintz(w_traits, 0, 0, h_ltgray,  _("                          "));
            mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, h_ltgray, title_TRAITS);
            if (line <= (trait_win_size_y-1)/2) {
                min = 0;
                max = trait_win_size_y;
                if (traitslist.size() < max)
                    max = traitslist.size();
            } else if (line >= traitslist.size() - (trait_win_size_y+1)/2) {
                min = (traitslist.size() < trait_win_size_y ? 0 : traitslist.size() - trait_win_size_y);
                max = traitslist.size();
            } else {
                min = line - (trait_win_size_y-1)/2;
                max = line + (trait_win_size_y+1)/2;
                if (traitslist.size() < max)
                    max = traitslist.size();
                if (min < 0)
                    min = 0;
            }

            for (int i = min; i < max; i++) {
                mvwprintz(w_traits, 1 + i - min, 1, c_ltgray, "                         ");
                if (i > traits.size())
                    status = c_ltblue;
                else if (mutation_data[traitslist[i]].threshold == true)
                    status = c_white;
                else if (traits[traitslist[i]].mixed_effect == true)
                    status = c_pink;
                else if (traits[traitslist[i]].points > 0)
                    status = c_ltgreen;
                else if (traits[traitslist[i]].points < 0)
                    status = c_ltred;
                else
                    status = c_yellow;
                if (i == line) {
                    mvwprintz(w_traits, 1 + i - min, 1, hilite(status), "%s",
                              traits[traitslist[i]].name.c_str());
                } else {
                    mvwprintz(w_traits, 1 + i - min, 1, status, "%s",
                              traits[traitslist[i]].name.c_str());
                }
            }
            if (line >= 0 && line < traitslist.size()) {
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta,
                               traits[traitslist[line]].description);
            }
            wrefresh(w_traits);
            wrefresh(w_info);

            action = ctxt.handle_input();
            if (action == "DOWN") {
                if (line < traitslist.size() - 1)
                    line++;
                break;
            } else if (action == "UP") {
                if (line > 0)
                    line--;
            } else if (action == "NEXT_TAB") {
                mvwprintz(w_traits, 0, 0, c_ltgray,  _("                          "));
                mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, c_ltgray, title_TRAITS);
                for (int i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
                    mvwprintz(w_traits, i + 1, 1, c_black, "                         ");
                    if (mutation_data[traitslist[i]].threshold == true)
                        status = c_white;
                    else if (traits[traitslist[i]].mixed_effect == true)
                        status = c_pink;
                    else if (traits[traitslist[i]].points > 0)
                        status = c_ltgreen;
                    else if (traits[traitslist[i]].points < 0)
                        status = c_ltred;
                    else
                        status = c_yellow;
                    mvwprintz(w_traits, i + 1, 1, status, "%s", traits[traitslist[i]].name.c_str());
                }
                wrefresh(w_traits);
                line = 0;
                curtab++;
            } else if (action == "QUIT") {
                done = true;
            }
            break;

        case 5: // Effects tab
            mvwprintz(w_effects, 0, 0, h_ltgray,  _("                          "));
            mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, h_ltgray, title_EFFECTS);
            half_y = effect_win_size_y / 2;
            if (line <= half_y) {
                min = 0;
                max = effect_win_size_y;
            if (effect_name.size() < max)
                max = effect_name.size();
            } else if (line >= effect_name.size() - half_y) {
                min = (effect_name.size() < effect_win_size_y ? 0 : effect_name.size() - effect_win_size_y);
                max = effect_name.size();
            } else {
                min = line - half_y;
                max = line - half_y + effect_win_size_y;
                if (effect_name.size() < max)
                    max = effect_name.size();
                if (min < 0)
                    min = 0;
            }

            for (int i = min; i < max; i++) {
                if (i == line)
                    mvwprintz(w_effects, 1 + i - min, 0, h_ltgray, "%s", effect_name[i].c_str());
                else
                    mvwprintz(w_effects, 1 + i - min, 0, c_ltgray, "%s", effect_name[i].c_str());
            }
            if (line >= 0 && line < effect_text.size()) {
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta, effect_text[line]);
            }
            wrefresh(w_effects);
            wrefresh(w_info);

            action = ctxt.handle_input();
            if (action == "DOWN") {
                if (line < effect_name.size() - 1)
                    line++;
                break;
            } else if (action == "UP") {
                if (line > 0)
                    line--;
            } else if (action == "NEXT_TAB") {
                mvwprintz(w_effects, 0, 0, c_ltgray,  _("                          "));
                mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, c_ltgray, title_EFFECTS);
                for (int i = 0; i < effect_name.size() && i < 7; i++) {
                    mvwprintz(w_effects, i + 1, 0, c_ltgray, "%s", effect_name[i].c_str());
                }
                wrefresh(w_effects);
                line = 0;
                curtab = 1;
            } else if (action == "QUIT") {
                done = true;
            }
            break;

        case 3: // Skills tab
            mvwprintz(w_skills, 0, 0, h_ltgray,  _("                          "));
            mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, h_ltgray, title_SKILLS);
            half_y = skill_win_size_y / 2;
            if (line <= half_y) {
                min = 0;
                max = skill_win_size_y;
                if (skillslist.size() < max)
                    max = skillslist.size();
            } else if (line >= skillslist.size() - half_y) {
                min = (skillslist.size() < skill_win_size_y ? 0 : skillslist.size() - skill_win_size_y);
                max = skillslist.size();
            } else {
                min = line - half_y;
                max = line - half_y + skill_win_size_y;
                if (skillslist.size() < max)
                    max = skillslist.size();
                    if (min < 0)
                        min = 0;
            }

            Skill *selectedSkill = NULL;

            for (int i = min; i < max; i++) {
                Skill *aSkill = skillslist[i];
                SkillLevel level = skillLevel(aSkill);

                bool isLearning = level.isTraining();
                bool rusting = level.isRusting();
                int exercise = level.exercise();

                if (i == line) {
                    selectedSkill = aSkill;
                    if (exercise >= 100)
                        status = isLearning ? h_pink : h_magenta;
                    else if (rusting)
                        status = isLearning ? h_ltred : h_red;
                    else
                        status = isLearning ? h_ltblue : h_blue;
                } else {
                    if (rusting)
                        status = isLearning ? c_ltred : c_red;
                    else
                        status = isLearning ? c_ltblue : c_blue;
                }
                mvwprintz(w_skills, 1 + i - min, 1, c_ltgray, "                         ");
                mvwprintz(w_skills, 1 + i - min, 1, status, "%s:", aSkill->name().c_str());
                mvwprintz(w_skills, 1 + i - min,19, status, "%-2d(%2d%%)", (int)level, (exercise <  0 ? 0 : exercise));
            }

            //Draw Scrollbar
            draw_scrollbar(w_skills, line, skill_win_size_y, skillslist.size(), 1);

            werase(w_info);

            if (line >= 0 && line < skillslist.size()) {
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta, selectedSkill->description());
            }
            wrefresh(w_skills);
            wrefresh(w_info);

            action = ctxt.handle_input();
            if (action == "DOWN") {
                if (line < skillslist.size() - 1)
                    line++;
            } else if (action == "UP") {
                if (line > 0)
                    line--;
            } else if (action == "NEXT_TAB") {
                werase(w_skills);
                mvwprintz(w_skills, 0, 0, c_ltgray,  _("                          "));
                mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, c_ltgray, title_SKILLS);
                for (int i = 0; i < skillslist.size() && i < skill_win_size_y; i++) {
                    Skill *thisSkill = skillslist[i];
                    SkillLevel level = skillLevel(thisSkill);
                    bool isLearning = level.isTraining();
                    bool rusting = level.isRusting();

                    if (rusting)
                        status = isLearning ? c_ltred : c_red;
                    else
                        status = isLearning ? c_ltblue : c_blue;

                    mvwprintz(w_skills, i + 1,  1, status, "%s:", thisSkill->name().c_str());
                    mvwprintz(w_skills, i + 1, 19, status, "%-2d(%2d%%)", (int)level,
                              (level.exercise() <  0 ? 0 : level.exercise()));
                }
                wrefresh(w_skills);
                line = 0;
                curtab++;
            } else if (action == "CONFIRM") {
                skillLevel(selectedSkill).toggleTraining();
            } else if (action == "QUIT") {
                done = true;
            }
        }
    } while (!done);

    werase(w_info);
    werase(w_tip);
    werase(w_stats);
    werase(w_encumb);
    werase(w_traits);
    werase(w_effects);
    werase(w_skills);
    werase(w_speed);
    werase(w_info);
    werase(w_grid_top);
    werase(w_grid_effect);
    werase(w_grid_skill);
    werase(w_grid_trait);

    delwin(w_info);
    delwin(w_tip);
    delwin(w_stats);
    delwin(w_encumb);
    delwin(w_traits);
    delwin(w_effects);
    delwin(w_skills);
    delwin(w_speed);
    delwin(w_grid_top);
    delwin(w_grid_effect);
    delwin(w_grid_skill);
    delwin(w_grid_trait);
}

void player::disp_morale()
{
    // Ensure the player's persistent morale effects are up-to-date.
    apply_persistent_morale();

    // Create and draw the window itself.
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                        (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                        (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
    draw_border(w);

    // Figure out how wide the name column needs to be.
    int name_column_width = 18;
    for (int i = 0; i < morale.size(); i++)
    {
        int length = morale[i].name(morale_data).length();
        if ( length > name_column_width)
        {
            name_column_width = length;
        }
    }

    // If it's too wide, truncate.
    if (name_column_width > 72)
    {
        name_column_width = 72;
    }

    // Start printing the number right after the name column.
    // We'll right-justify it later.
    int number_pos = name_column_width + 1;

    // Header
    mvwprintz(w, 1,  1, c_white, _("Morale Modifiers:"));
    mvwprintz(w, 2,  1, c_ltgray, _("Name"));
    mvwprintz(w, 2, name_column_width+2, c_ltgray, _("Value"));

    // Print out the morale entries.
    for (int i = 0; i < morale.size(); i++)
    {
        std::string name = morale[i].name(morale_data);
        int bonus = net_morale(morale[i]);

        // Trim the name if need be.
        if (name.length() > name_column_width)
        {
            name = name.erase(name_column_width-3, std::string::npos) + "...";
        }

        // Print out the name.
        mvwprintz(w, i + 3,  1, (bonus < 0 ? c_red : c_green), name.c_str());

        // Print out the number, right-justified.
        mvwprintz(w, i + 3, number_pos, (bonus < 0 ? c_red : c_green),
                  "% 6d", bonus);
    }

    // Print out the total morale, right-justified.
    int mor = morale_level();
    mvwprintz(w, 20, 1, (mor < 0 ? c_red : c_green), _("Total:"));
    mvwprintz(w, 20, number_pos, (mor < 0 ? c_red : c_green), "% 6d", mor);

    // Print out the focus gain rate, right-justified.
    double gain = (calc_focus_equilibrium() - focus_pool) / 100.0;
    mvwprintz(w, 22, 1, (gain < 0 ? c_red : c_green), _("Focus gain:"));
    mvwprintz(w, 22, number_pos-3, (gain < 0 ? c_red : c_green), _("%6.2f per minute"), gain);

    // Make sure the changes are shown.
    wrefresh(w);

    // Wait for any keystroke.
    getch();

    // Close the window.
    werase(w);
    delwin(w);
}

void player::disp_status(WINDOW *w, WINDOW *w2)
{
    bool sideStyle = use_narrow_sidebar();
    WINDOW *weapwin = sideStyle ? w2 : w;

    // Print current weapon, or attachment if active.
    item* gunmod = weapon.active_gunmod();
    std::string mode = "";
    std::stringstream attachment;
    if (gunmod != NULL)
    {
        attachment << gunmod->type->nname(1).c_str();
        for (int i = 0; i < weapon.contents.size(); i++)
                if (weapon.contents[i].is_gunmod() &&
                        weapon.contents[i].has_flag("MODE_AUX"))
                    attachment << " (" << weapon.contents[i].charges << ")";
        mvwprintz(weapwin, sideStyle ? 1 : 0, 0, c_ltgray, _("%s (Mod)"), attachment.str().c_str());
    }
    else
    {
        if (weapon.mode == "MODE_BURST")
                mvwprintz(weapwin, sideStyle ? 1 : 0, 0, c_ltgray, _("%s (Burst)"), weapname().c_str());
        else
            mvwprintz(weapwin, sideStyle ? 1 : 0, 0, c_ltgray, _("%s"), weapname().c_str());
    }

    if (weapon.is_gun()) {
        int adj_recoil = recoil + driving_recoil;
        if (adj_recoil > 0) {
            nc_color c = c_ltgray;
            if (adj_recoil >= 36)
                c = c_red;
            else if (adj_recoil >= 20)
                c = c_ltred;
            else if (adj_recoil >= 4)
                c = c_yellow;
            int y = sideStyle ? 1 : 0;
            int x = sideStyle ? (getmaxx(weapwin) - 6) : 34;
            mvwprintz(weapwin, y, x, c, _("Recoil"));
        }
    }

    // Print currently used style or weapon mode.
    std::string style = "";
    if (is_armed()) {
        // Show normal if no martial style is selected,
        // or if the currently selected style does nothing for your weapon.
        if (style_selected == "style_none" ||
            (!can_melee() && !martialarts[style_selected].has_weapon(weapon.type->id))) {
            style = _("Normal");
        } else {
            style = martialarts[style_selected].name;
        }

        int x = sideStyle ? (getmaxx(weapwin) - 13) : 0;
        mvwprintz(weapwin, 1, x, c_red, style.c_str());
    } else {
        if (style_selected == "style_none") {
            style = _("No Style");
        } else {
            style = martialarts[style_selected].name;
        }
        if (style != "") {
            int x = sideStyle ? (getmaxx(weapwin) - 13) : 0;
            mvwprintz(weapwin, 1, x, c_blue, style.c_str());
        }
    }

    wmove(w, sideStyle ? 1 : 2, 0);
    if (hunger > 2800)
        wprintz(w, c_red,    _("Starving!"));
    else if (hunger > 1400)
        wprintz(w, c_ltred,  _("Near starving"));
    else if (hunger > 300)
        wprintz(w, c_ltred,  _("Famished"));
    else if (hunger > 100)
        wprintz(w, c_yellow, _("Very hungry"));
    else if (hunger > 40)
        wprintz(w, c_yellow, _("Hungry"));
    else if (hunger < 0)
        wprintz(w, c_green,  _("Full"));
    else if (hunger < -20)
        wprintz(w, c_green,  _("Sated"));
    else if (hunger < -60)
        wprintz(w, c_green,  _("Engorged"));

 // Find hottest/coldest bodypart
 int min = 0, max = 0;
 for (int i = 0; i < num_bp ; i++ ){
  if      (temp_cur[i] > BODYTEMP_HOT  && temp_cur[i] > temp_cur[max]) max = i;
  else if (temp_cur[i] < BODYTEMP_COLD && temp_cur[i] < temp_cur[min]) min = i;
 }
 // Compare which is most extreme
 int print;
 if (temp_cur[max] - BODYTEMP_NORM > BODYTEMP_NORM + temp_cur[min]) print = max;
 else print = min;
 // Assign zones to temp_cur and temp_conv for comparison
 int cur_zone = 0;
 if      (temp_cur[print] >  BODYTEMP_SCORCHING) cur_zone = 7;
 else if (temp_cur[print] >  BODYTEMP_VERY_HOT)  cur_zone = 6;
 else if (temp_cur[print] >  BODYTEMP_HOT)       cur_zone = 5;
 else if (temp_cur[print] >  BODYTEMP_COLD)      cur_zone = 4;
 else if (temp_cur[print] >  BODYTEMP_VERY_COLD) cur_zone = 3;
 else if (temp_cur[print] >  BODYTEMP_FREEZING)  cur_zone = 2;
 else if (temp_cur[print] <= BODYTEMP_FREEZING)  cur_zone = 1;
 int conv_zone = 0;
 if      (temp_conv[print] >  BODYTEMP_SCORCHING) conv_zone = 7;
 else if (temp_conv[print] >  BODYTEMP_VERY_HOT)  conv_zone = 6;
 else if (temp_conv[print] >  BODYTEMP_HOT)       conv_zone = 5;
 else if (temp_conv[print] >  BODYTEMP_COLD)      conv_zone = 4;
 else if (temp_conv[print] >  BODYTEMP_VERY_COLD) conv_zone = 3;
 else if (temp_conv[print] >  BODYTEMP_FREEZING)  conv_zone = 2;
 else if (temp_conv[print] <= BODYTEMP_FREEZING)  conv_zone = 1;
 // delta will be positive if temp_cur is rising
 int delta = conv_zone - cur_zone;
 // Decide if temp_cur is rising or falling
 const char *temp_message = "Error";
 if      (delta >   2) temp_message = _(" (Rising!!)");
 else if (delta ==  2) temp_message = _(" (Rising!)");
 else if (delta ==  1) temp_message = _(" (Rising)");
 else if (delta ==  0) temp_message = "";
 else if (delta == -1) temp_message = _(" (Falling)");
 else if (delta == -2) temp_message = _(" (Falling!)");
 else if (delta <  -2) temp_message = _(" (Falling!!)");
 // Print the hottest/coldest bodypart, and if it is rising or falling in temperature

    wmove(w, sideStyle ? 6 : 1, sideStyle ? 0 : 9);
    if      (temp_cur[print] >  BODYTEMP_SCORCHING)
        wprintz(w, c_red,   _("Scorching!%s"), temp_message);
    else if (temp_cur[print] >  BODYTEMP_VERY_HOT)
        wprintz(w, c_ltred, _("Very hot!%s"), temp_message);
    else if (temp_cur[print] >  BODYTEMP_HOT)
        wprintz(w, c_yellow,_("Warm%s"), temp_message);
    else if (temp_cur[print] >  BODYTEMP_COLD) // If you're warmer than cold, you are comfortable
        wprintz(w, c_green, _("Comfortable%s"), temp_message);
    else if (temp_cur[print] >  BODYTEMP_VERY_COLD)
        wprintz(w, c_ltblue,_("Chilly%s"), temp_message);
    else if (temp_cur[print] >  BODYTEMP_FREEZING)
        wprintz(w, c_cyan,  _("Very cold!%s"), temp_message);
    else if (temp_cur[print] <= BODYTEMP_FREEZING)
        wprintz(w, c_blue,  _("Freezing!%s"), temp_message);

    int x = sideStyle ? 37 : 32;
    int y = sideStyle ?  0 :  1;
    if(is_deaf()) {
        mvwprintz(sideStyle ? w2 : w, y, x, c_red, _("Deaf!"), volume);
    } else {
        mvwprintz(sideStyle ? w2 : w, y, x, c_yellow, _("Sound %d"), volume);
    }
    volume = 0;

    wmove(w, 2, sideStyle ? 0 : 15);
    if (thirst > 520)
        wprintz(w, c_ltred,  _("Parched"));
    else if (thirst > 240)
        wprintz(w, c_ltred,  _("Dehydrated"));
    else if (thirst > 80)
        wprintz(w, c_yellow, _("Very thirsty"));
    else if (thirst > 40)
        wprintz(w, c_yellow, _("Thirsty"));
    else if (thirst < 0)
        wprintz(w, c_green,  _("Slaked"));
    else if (thirst < -20)
        wprintz(w, c_green,  _("Hydrated"));
    else if (thirst < -60)
        wprintz(w, c_green,  _("Turgid"));

    wmove(w, sideStyle ? 3 : 2, sideStyle ? 0 : 30);
    if (fatigue > 575)
        wprintz(w, c_red,    _("Exhausted"));
    else if (fatigue > 383)
        wprintz(w, c_ltred,  _("Dead tired"));
    else if (fatigue > 191)
        wprintz(w, c_yellow, _("Tired"));

    wmove(w, sideStyle ? 4 : 2, sideStyle ? 0 : 41);
    wprintz(w, c_white, _("Focus"));
    nc_color col_xp = c_dkgray;
    if (focus_pool >= 100)
        col_xp = c_white;
    else if (focus_pool >  0)
        col_xp = c_ltgray;
    wprintz(w, col_xp, " %d", focus_pool);

    nc_color col_pain = c_yellow;
    if (pain - pkill >= 60)
        col_pain = c_red;
    else if (pain - pkill >= 40)
        col_pain = c_ltred;
    if (pain - pkill > 0)
        mvwprintz(w, sideStyle ? 0 : 3, 0, col_pain, _("Pain %d"), pain - pkill);

    int morale_cur = morale_level ();
    nc_color col_morale = c_white;
    if (morale_cur >= 10)
        col_morale = c_green;
    else if (morale_cur <= -10)
        col_morale = c_red;
    const char *morale_str;
    if      (morale_cur >= 200) morale_str = "8D";
    else if (morale_cur >= 100) morale_str = ":D";
    else if (morale_cur >= 10)  morale_str = ":)";
    else if (morale_cur > -10)  morale_str = ":|";
    else if (morale_cur > -100) morale_str = "):";
    else if (morale_cur > -200) morale_str = "D:";
    else                        morale_str = "D8";
    mvwprintz(w, sideStyle ? 0 : 3, sideStyle ? 11 : 10, col_morale, morale_str);

 vehicle *veh = g->m.veh_at (posx, posy);
 if (in_vehicle && veh) {
  veh->print_fuel_indicator(w, sideStyle ? 2 : 3, sideStyle ? getmaxx(w) - 5 : 49);
  nc_color col_indf1 = c_ltgray;

  float strain = veh->strain();
  nc_color col_vel = strain <= 0? c_ltblue :
                     (strain <= 0.2? c_yellow :
                     (strain <= 0.4? c_ltred : c_red));

  bool has_turrets = false;
  for (int p = 0; p < veh->parts.size(); p++) {
   if (veh->part_flag (p, "TURRET")) {
    has_turrets = true;
    break;
   }
  }

  if (has_turrets) {
   mvwprintz(w, 3, sideStyle ? 16 : 25, col_indf1, "Gun:");
   wprintz(w, veh->turret_mode ? c_ltred : c_ltblue,
              veh->turret_mode ? "auto" : "off ");
  }

  //
  // Draw the speedometer.
  //

  int speedox = sideStyle ? 0 : 33;
  int speedoy = sideStyle ? 5 :  3;

  bool metric = OPTIONS["USE_METRIC_SPEEDS"] == "km/h";
  const char *units = metric ? _("km/h") : _("mph");
  int velx    = metric ?  5 : 4; // strlen(units) + 1
  int cruisex = metric ? 10 : 9; // strlen(units) + 6
  float conv  = metric ? 0.0161f : 0.01f;

  if (0 == sideStyle) {
    if (!veh->cruise_on) speedox += 2;
    if (!metric)         speedox++;
  }

  const char *speedo = veh->cruise_on ? "{%s....>....}" : "{%s....}";
  mvwprintz(w, speedoy, speedox,        col_indf1, speedo, units);
  mvwprintz(w, speedoy, speedox + velx, col_vel,   "%4d", int(veh->velocity * conv));
  if (veh->cruise_on)
    mvwprintz(w, speedoy, speedox + cruisex, c_ltgreen, "%4d", int(veh->cruise_velocity * conv));


  if (veh->velocity != 0) {
   nc_color col_indc = veh->skidding? c_red : c_green;
   int dfm = veh->face.dir() - veh->move.dir();
   wmove(w, sideStyle ? 5 : 3, getmaxx(w) - 3);
   wprintz(w, col_indc, dfm <  0 ? "L" : ".");
   wprintz(w, col_indc, dfm == 0 ? "0" : ".");
   wprintz(w, col_indc, dfm >  0 ? "R" : ".");
  }
 } else {  // Not in vehicle
  nc_color col_str = c_white, col_dex = c_white, col_int = c_white,
           col_per = c_white, col_spd = c_white, col_time = c_white;
  int str_bonus = get_str_bonus();
  int dex_bonus = get_dex_bonus();
  int int_bonus = get_int_bonus();
  int per_bonus = get_per_bonus();
  int spd_bonus = get_speed_bonus();
  if (str_bonus < 0)
   col_str = c_red;
  if (str_bonus > 0)
   col_str = c_green;
  if (dex_bonus  < 0)
   col_dex = c_red;
  if (dex_bonus  > 0)
   col_dex = c_green;
  if (int_bonus  < 0)
   col_int = c_red;
  if (int_bonus  > 0)
   col_int = c_green;
  if (per_bonus  < 0)
   col_per = c_red;
  if (per_bonus  > 0)
   col_per = c_green;
  if (spd_bonus < 0)
   col_spd = c_red;
  if (spd_bonus > 0)
   col_spd = c_green;

    int x  = sideStyle ? 18 : 13;
    int y  = sideStyle ?  0 :  3;
    int dx = sideStyle ?  0 :  7;
    int dy = sideStyle ?  1 :  0;
    mvwprintz(w, y + dy * 0, x + dx * 0, col_str, _("Str %2d"), get_str());
    mvwprintz(w, y + dy * 1, x + dx * 1, col_dex, _("Dex %2d"), get_dex());
    mvwprintz(w, y + dy * 2, x + dx * 2, col_int, _("Int %2d"), get_int());
    mvwprintz(w, y + dy * 3, x + dx * 3, col_per, _("Per %2d"), get_per());

    int spdx = sideStyle ?  0 : x + dx * 4;
    int spdy = sideStyle ?  5 : y + dy * 4;
    mvwprintz(w, spdy, spdx, col_spd, _("Spd %2d"), get_speed());
    if (this->weight_carried() > this->weight_capacity()) {
        col_time = h_black;
    }
    if (this->volume_carried() > this->volume_capacity() - 2) {
        if (this->weight_carried() > this->weight_capacity()) {
            col_time = c_dkgray_magenta;
        } else {
            col_time = c_dkgray_red;
        }
    }
    wprintz(w, col_time, "  %d", movecounter);
 }
}

bool player::has_trait(const std::string &flag) const
{
    // Look for active mutations and traits
    return (flag != "") && (my_mutations.find(flag) != my_mutations.end());
}

bool player::has_base_trait(const std::string &flag) const
{
    // Look only at base traits
    return (flag != "") && (my_traits.find(flag) != my_traits.end());
}

bool player::has_conflicting_trait(const std::string &flag) const
{
    return (has_opposite_trait(flag) || has_lower_trait(flag) || has_higher_trait(flag));
}

bool player::has_opposite_trait(const std::string &flag) const
{
    if (!mutation_data[flag].cancels.empty()) {
        std::vector<std::string> cancels = mutation_data[flag].cancels;
        for (int i = 0; i < cancels.size(); i++) {
            if (has_trait(cancels[i])) {
                return true;
            }
        }
    }
    return false;
}

bool player::has_lower_trait(const std::string &flag) const
{
    if (!mutation_data[flag].prereqs.empty()) {
        std::vector<std::string> prereqs = mutation_data[flag].prereqs;
        for (int i = 0; i < prereqs.size(); i++) {
            if (has_trait(prereqs[i]) || has_lower_trait(prereqs[i])) {
                return true;
            }
        }
    }
    return false;
}

bool player::has_higher_trait(const std::string &flag) const
{
    if (!mutation_data[flag].replacements.empty()) {
        std::vector<std::string> replacements = mutation_data[flag].replacements;
        for (int i = 0; i < replacements.size(); i++) {
            if (has_trait(replacements[i]) || has_higher_trait(replacements[i])) {
                return true;
            }
        }
    }
    return false;
}

bool player::crossed_threshold()
{
  std::vector<std::string> traitslist;
  for( auto iter = my_mutations.begin(); iter != my_mutations.end(); ++iter ) {
        traitslist.push_back(*iter);
    }
  for (int i = 0; i < traitslist.size(); i++) {
      if (mutation_data[traitslist[i]].threshold == true)
      return true;
  }
 return false;
}

bool player::purifiable(const std::string &flag) const
{
    if(mutation_data[flag].purifiable) {
        return true;
    }
    return false;
}

void toggle_str_set( std::unordered_set< std::string > &set, const std::string &str )
{
    auto i = set.find(str);
    if (i == set.end()) {
        set.insert(str);
    } else {
        set.erase(i);
    }
}

void player::toggle_trait(const std::string &flag)
{
    toggle_str_set(my_traits, flag); //Toggles a base trait on the player
    toggle_str_set(my_mutations, flag); //Toggles corresponding trait in mutations list as well.
    recalc_sight_limits();
}

void player::toggle_mutation(const std::string &flag)
{
    toggle_str_set(my_mutations, flag); //Toggles a mutation on the player
    recalc_sight_limits();
}

void player::set_cat_level_rec(const std::string &sMut)
{
    if (!has_base_trait(sMut)) { //Skip base traits
        for (int i = 0; i < mutation_data[sMut].category.size(); i++) {
            mutation_category_level[mutation_data[sMut].category[i]] += 8;
        }

        for (int i = 0; i < mutation_data[sMut].prereqs.size(); i++) {
            set_cat_level_rec(mutation_data[sMut].prereqs[i]);
        }

        for (int i = 0; i < mutation_data[sMut].prereqs2.size(); i++) {
            set_cat_level_rec(mutation_data[sMut].prereqs2[i]);
        }
    }
}

void player::set_highest_cat_level()
{
    mutation_category_level.clear();

    // Loop through our mutations
    for( auto iter = my_mutations.begin(); iter != my_mutations.end(); ++iter ) {
        set_cat_level_rec(*iter);
    }
}

std::string player::get_highest_category() const // Returns the mutation category with the highest strength
{
    int iLevel = 0;
    std::string sMaxCat = "";

    for (std::map<std::string, int>::const_iterator iter = mutation_category_level.begin(); iter != mutation_category_level.end(); ++iter) {
        if (iter->second > iLevel) {
            sMaxCat = iter->first;
            iLevel = iter->second;
        } else if (iter->second == iLevel) {
            sMaxCat = "";  // no category on ties
        }
    }
    return sMaxCat;
}

std::string player::get_category_dream(const std::string &cat, int strength) const // Returns a randomly selected dream
{
    std::string message;
    std::vector<dream> valid_dreams;
    dream selected_dream;
    for (int i = 0; i < dreams.size(); i++) { //Pull the list of dreams
        if ((dreams[i].category == cat) && (dreams[i].strength == strength)) { //Pick only the ones matching our desired category and strength
            valid_dreams.push_back(dreams[i]); // Put the valid ones into our list
        }
    }

    int index = rng(0, valid_dreams.size() - 1); // Randomly select a dream from the valid list
    selected_dream = valid_dreams[index];
    index = rng(0, selected_dream.messages.size() - 1); // Randomly selected a message from the chosen dream
    message = selected_dream.messages[index];
    return message;
}

bool player::in_climate_control()
{
    bool regulated_area=false;
    // Check
    if(has_active_bionic("bio_climate")) { return true; }
    if ((is_wearing("rm13_armor_on")) || (is_wearing_power_armor() &&
        (has_active_item("UPS_on") || has_active_item("adv_UPS_on") || has_active_bionic("bio_power_armor_interface") || has_active_bionic("bio_power_armor_interface_mkII"))))
    {
        return true;
    }
    if(int(calendar::turn) >= next_climate_control_check)
    {
        next_climate_control_check=int(calendar::turn)+20;  // save cpu and similate acclimation.
        int vpart = -1;
        vehicle *veh = g->m.veh_at(posx, posy, vpart);
        if(veh)
        {
            regulated_area=(
                veh->is_inside(vpart) &&    // Already checks for opened doors
                veh->total_power(true) > 0  // Out of gas? No AC for you!
            );  // TODO: (?) Force player to scrounge together an AC unit
        }
        // TODO: AC check for when building power is implemented
        last_climate_control_ret=regulated_area;
        if(!regulated_area) { next_climate_control_check+=40; }  // Takes longer to cool down / warm up with AC, than it does to step outside and feel cruddy.
    }
    else
    {
        return ( last_climate_control_ret ? true : false );
    }
    return regulated_area;
}

std::list<item *> player::get_radio_items()
{
    std::list<item *> rc_items;
    const invslice & stacks = inv.slice();
    for( size_t x  = 0; x  < stacks.size(); ++x  ) {
        item &itemit = stacks[x]->front();
        item *stack_iter = &itemit;
        if (stack_iter->active && stack_iter->has_flag("RADIO_ACTIVATION")) {
            rc_items.push_back( stack_iter );
        }
    }

    for (size_t i = 0; i < worn.size(); i++) {
        if ( worn[i].active  && worn[i].has_flag("RADIO_ACTIVATION")) {
            rc_items.push_back( &worn[i] );
        }
    }

    if (!weapon.is_null()) {
        if ( weapon.active  && weapon.has_flag("RADIO_ACTIVATION")) {
            rc_items.push_back( &weapon );
        }
    }
    return rc_items;
}



bool player::has_bionic(const bionic_id & b) const
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return true;
 }
 return false;
}

bool player::has_active_optcloak() const {
  static const std::string str_UPS_on("UPS_on");
  static const std::string str_adv_UPS_on("adv_UPS_on");
  static const std::string str_optical_cloak("optical_cloak");

  if ((has_active_item(str_UPS_on) || has_active_item(str_adv_UPS_on))
      && is_wearing(str_optical_cloak)) {
    return true;
  } else {
    return false;
  }
}

bool player::has_active_bionic(const bionic_id & b) const
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return (my_bionics[i].powered);
 }
 return false;
}

void player::add_bionic( bionic_id b )
{
    if( has_bionic( b ) ) {
        debugmsg( "Tried to install bionic %s that is already installed!", b.c_str() );
        return;
    }
    char newinv = ' ';
    for( size_t i = 0; i < inv_chars.size(); i++ ) {
        if( bionic_by_invlet( inv_chars[i] ) == nullptr ) {
            newinv = inv_chars[i];
            break;
        }
    }
    my_bionics.push_back( bionic( b, newinv ) );
    recalc_sight_limits();
}

void player::remove_bionic(bionic_id b) {
    std::vector<bionic> new_my_bionics;
    for(int i = 0; i < my_bionics.size(); i++) {
        if (!(my_bionics[i].id == b)) {
            new_my_bionics.push_back(bionic(my_bionics[i].id, my_bionics[i].invlet));
        }
    }
    my_bionics = new_my_bionics;
    recalc_sight_limits();
}

int player::num_bionics() const
{
    return my_bionics.size();
}

bionic& player::bionic_at_index(int i)
{
    return my_bionics[i];
}

bionic* player::bionic_by_invlet(char ch) {
    for (size_t i = 0; i < my_bionics.size(); i++) {
        if (my_bionics[i].invlet == ch) {
            return &my_bionics[i];
        }
    }
    return 0;
}

// Returns true if a bionic was removed.
bool player::remove_random_bionic() {
    const int numb = num_bionics();
    if (numb) {
        int rem = rng(0, num_bionics() - 1);
        my_bionics.erase(my_bionics.begin() + rem);
        recalc_sight_limits();
    }
    return numb;
}

void player::charge_power(int amount)
{
 power_level += amount;
 if (power_level > max_power_level)
  power_level = max_power_level;
 if (power_level < 0)
  power_level = 0;
}


/*
 * Calculate player brightness based on the brightest active item, as
 * per itype tag LIGHT_* and optional CHARGEDIM ( fade starting at 20% charge )
 * item.light.* is -unimplemented- for the moment, as it is a custom override for
 * applying light sources/arcs with specific angle and direction.
 */
float player::active_light()
{
    float lumination = 0;

    int maxlum = 0;
    const invslice & stacks = inv.slice();
    for( size_t x  = 0; x  < stacks.size(); ++x  ) {
        item &itemit = stacks[x]->front();
        item * stack_iter = &itemit;
        if (stack_iter->active && stack_iter->charges > 0) {
            int lumit = stack_iter->getlight_emit(true);
            if ( maxlum < lumit ) {
                maxlum = lumit;
            }
        }
    }

    for (size_t i = 0; i < worn.size(); i++) {
        if ( worn[i].active  && worn[i].charges > 0) {
            int lumit = worn[i].getlight_emit(true);
            if ( maxlum < lumit ) {
                maxlum = lumit;
            }
        }
    }

    if (!weapon.is_null()) {
        if ( weapon.active  && weapon.charges > 0) {
            int lumit = weapon.getlight_emit(true);
            if ( maxlum < lumit ) {
                maxlum = lumit;
            }
        }
    }

    lumination = (float)maxlum;

    if ( lumination < 60 && has_active_bionic("bio_flashlight") ) {
        lumination = 60;
    } else if ( lumination < 25 && has_artifact_with(AEP_GLOW) ) {
        lumination = 25;
    }

    return lumination;
}

point player::pos()
{
    return point(posx, posy);
}

int player::sight_range(int light_level) const
{
    // Apply the sight boost (night vision).
    if (light_level < sight_boost_cap) {
        light_level = std::min(light_level + sight_boost, sight_boost_cap);
    }

    // Clamp to sight_max.
    return std::min(light_level, sight_max);
}

// This must be called when any of the following change:
// - diseases
// - bionics
// - traits
// - underwater
// - clothes
// With the exception of clothes, all changes to these player attributes must
// occur through a function in this class which calls this function. Clothes are
// typically added/removed with wear() and takeoff(), but direct access to the
// 'wears' vector is still allowed due to refactor exhaustion.
void player::recalc_sight_limits()
{
    sight_max = 9999;
    sight_boost = 0;
    sight_boost_cap = 0;

    // Set sight_max.
    if (has_effect("blind")) {
        sight_max = 0;
    } else if (has_disease("in_pit") ||
            (has_disease("boomered") && (!(has_trait("PER_SLIME_OK")))) ||
            (underwater && !has_bionic("bio_membrane") &&
                !has_trait("MEMBRANE") && !worn_with_flag("SWIM_GOGGLES") &&
                (!(has_trait("PER_SLIME_OK"))))) {
        sight_max = 1;
    } else if ( (has_trait("MYOPIC") || has_trait("URSINE_EYE")) &&
            !is_wearing("glasses_eye") && !is_wearing("glasses_monocle") &&
            !is_wearing("glasses_bifocal") && !has_disease("contacts")) {
        sight_max = 4;
    } else if (has_trait("PER_SLIME")) {
        sight_max = 6;
    }

    // Set sight_boost and sight_boost_cap, based on night vision.
    // (A player will never have more than one night vision trait.)
    sight_boost_cap = 12;
    // Debug-only NV, by vache's request
    if (has_trait("DEBUG_NIGHTVISION")) {
        sight_boost = 59;
        sight_boost_cap = 59;
    } else if (has_nv() || has_trait("NIGHTVISION3") || has_trait("ELFA_FNV") || is_wearing("rm13_armor_on")) {
        // Yes, I'm breaking the cap. I doubt the reality bubble shrinks at night.
        // BIRD_EYE represents excellent fine-detail vision so I think it works.
        if (has_trait("BIRD_EYE")) {
            sight_boost = 13;
        }
        else {
        sight_boost = sight_boost_cap;
        }
    } else if (has_trait("ELFA_NV")) {
        sight_boost = 6; // Elf-a and Bird eyes shouldn't coexist
    } else if (has_trait("NIGHTVISION2") || has_trait("FEL_NV") || has_trait("URSINE_EYE")) {
        if (has_trait("BIRD_EYE")) {
            sight_boost = 5;
        }
         else {
            sight_boost = 4;
         }
    } else if (has_trait("NIGHTVISION")) {
        if (has_trait("BIRD_EYE")) {
            sight_boost = 2;
        }
        else {
            sight_boost = 1;
        }
    }
}

int player::unimpaired_range()
{
 int ret = DAYLIGHT_LEVEL;
 if (has_trait("PER_SLIME")) {
    ret = 6;
 }
 if (has_disease("in_pit")) {
    ret = 1;
  }
 if (has_effect("blind")) {
    ret = 0;
  }
 return ret;
}

bool player::overmap_los(int omtx, int omty, int sight_points)
{
    const tripoint ompos = g->om_global_location();
    if (omtx < ompos.x - sight_points || omtx > ompos.x + sight_points ||
        omty < ompos.y - sight_points || omty > ompos.y + sight_points) {
        // Outside maximum sight range
        return false;
    }

    const std::vector<point> line = line_to(ompos.x, ompos.y, omtx, omty, 0);
    for (size_t i = 0; i < line.size() && sight_points >= 0; i++) {
        const oter_id &ter = overmap_buffer.ter(line[i].x, line[i].y, ompos.z);
        const int cost = otermap[ter].see_cost;
        sight_points -= cost;
        if (sight_points < 0)
            return false;
    }
    return true;
}

int player::overmap_sight_range(int light_level)
{
    int sight = sight_range(light_level);
    if( sight < SEEX ) {
        return 0;
    }
    if( sight <= SEEX * 4) {
        return (sight / (SEEX / 2) );
    }
    if ((has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        -1 != weapon.has_gunmod("rifle_scope") ) && !has_trait("EAGLEEYED"))  {
        if (has_trait("BIRD_EYE")) {
            return 25;
        }
        return 20;
    }
    else if (!(has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        -1 != weapon.has_gunmod("rifle_scope") ) && has_trait("EAGLEEYED"))  {
        if (has_trait("BIRD_EYE")) {
            return 25;
        }
        return 20;
    }
    else if ((has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        -1 != weapon.has_gunmod("rifle_scope") ) && has_trait("EAGLEEYED"))  {
        if (has_trait("BIRD_EYE")) {
            return 30;
        }
        return 25;
    }

    return 10;
}

int player::clairvoyance()
{
 if (has_artifact_with(AEP_CLAIRVOYANCE))
  return 3;
 if (has_artifact_with(AEP_SUPER_CLAIRVOYANCE))
  return 40;
 return 0;
}

bool player::sight_impaired()
{
 return ((has_disease("boomered") && (!(has_trait("PER_SLIME_OK")))) ||
  (underwater && !has_bionic("bio_membrane") && !has_trait("MEMBRANE")
              && !worn_with_flag("SWIM_GOGGLES") && !(has_trait("PER_SLIME_OK"))) ||
  ((has_trait("MYOPIC") || has_trait("URSINE_EYE") ) &&
                        !is_wearing("glasses_eye") &&
                        !is_wearing("glasses_monocle") &&
                        !is_wearing("glasses_bifocal") &&
                        !has_disease("contacts")) ||
   has_trait("PER_SLIME"));
}

bool player::has_two_arms() const
{
 if (has_bionic("bio_blaster") || hp_cur[hp_arm_l] < 10 || hp_cur[hp_arm_r] < 10)
  return false;
 return true;
}

bool player::avoid_trap(trap* tr, int x, int y)
{
  int myroll = dice(3, int(dex_cur + skillLevel("dodge") * 1.5));
 int traproll;
    if (tr->can_see(*this, x, y)) {
        traproll = dice(3, tr->get_avoidance());
    } else {
        traproll = dice(6, tr->get_avoidance());
    }
 if (has_trait("LIGHTSTEP"))
  myroll += dice(2, 6);
 if (has_trait("CLUMSY"))
  myroll -= dice(2, 6);
 if (myroll >= traproll)
  return true;
 return false;
}

bool player::has_nv()
{
    static bool nv = false;

    if( !nv_cached ) {
        nv_cached = true;
        nv = (worn_with_flag("GNV_EFFECT") ||
              has_active_bionic("bio_night_vision"));
    }

    return nv;
}

bool player::has_pda()
{
    static bool pda = false;
    if ( !pda_cached ) {
      pda_cached = true;
      pda = has_amount("pda", 1);
    }

    return pda;
}

void player::pause()
{
    moves = 0;
    if (recoil > 0) {
        if (str_cur + 2 * skillLevel("gun") >= recoil) {
            recoil = 0;
        } else {
            recoil -= str_cur + 2 * skillLevel("gun");
            recoil = int(recoil / 2);
        }
    }

    //Web Weavers...weave web
    if (has_trait("WEB_WEAVER") && !in_vehicle) {
      g->m.add_field(posx, posy, fd_web, 1); //this adds density to if its not already there.
      add_msg("You spin some webbing.");
     }

    // Meditation boost for Toad Style, obsolete
    if (weapon.type->id == "style_toad" && activity.type == ACT_NULL) {
        int arm_amount = 1 + (int_cur - 6) / 3 + (per_cur - 6) / 3;
        int arm_max = (int_cur + per_cur) / 2;
        if (arm_amount > 3) {
            arm_amount = 3;
        }
        if (arm_max > 20) {
            arm_max = 20;
        }
        add_disease("armor_boost", 2, false, arm_amount, arm_max);
    }

    // Train swimming if underwater
    if (underwater) {
        practice( "swimming", 1 );
        if (g->temperature <= 50) {
            drench(100, mfb(bp_legs)|mfb(bp_torso)|mfb(bp_arms)|mfb(bp_head)|
                           mfb(bp_eyes)|mfb(bp_mouth)|mfb(bp_feet)|mfb(bp_hands));
        } else {
            drench(100, mfb(bp_legs)|mfb(bp_torso)|mfb(bp_arms)|mfb(bp_head)|
                           mfb(bp_eyes)|mfb(bp_mouth));
        }
    }

    VehicleList vehs = g->m.get_vehicles();
    vehicle* veh = NULL;
    for (int v = 0; v < vehs.size(); ++v) {
        veh = vehs[v].v;
        if (veh && veh->velocity != 0 && veh->player_in_control(this)) {
            if (one_in(10)) {
                practice( "driving", 1 );
            }
            break;
        }
    }

    search_surroundings();
}

void player::search_surroundings()
{
    if (controlling_vehicle) {
        return;
    }
    // Search for traps in a larger area than before because this is the only
    // way we can "find" traps that aren't marked as visible.
    // Detection formula takes care of likelihood of seeing within this range.
    for (size_t i = 0; i < 121; i++) {
        const int x = posx + i / 11 - 5;
        const int y = posy + i % 11 - 5;
        const trap_id trid = g->m.tr_at(x, y);
        if (trid == tr_null || (x == posx && y == posy)) {
            continue;
        }
        const trap *tr = traplist[trid];
        if (tr->name.empty() || tr->can_see(*this, x, y)) {
            // Already seen, or has no name -> can never be seen
            continue;
        }
        // Chance to detect traps we haven't yet seen.
        if (tr->detect_trap(*this, x, y)) {
            if( tr->get_visibility() > 0 ) {
                // Only bug player about traps that aren't trivial to spot.
                const std::string direction = direction_name(direction_from(posx, posy, x, y));
                add_msg_if_player(_("You've spotted a %s to the %s!"),
                                  tr->name.c_str(), direction.c_str());
            }
            add_known_trap(x, y, tr->id);
        }
    }
}

int player::throw_range(int pos)
{
 item tmp;
 if (pos == -1)
  tmp = weapon;
 else if (pos == INT_MIN)
  return -1;
 else
  tmp = inv.find_item(pos);

 if (tmp.count_by_charges() && tmp.charges > 1)
  tmp.charges = 1;

 if ((tmp.weight() / 113) > int(str_cur * 15))
  return 0;
 // Increases as weight decreases until 150 g, then decreases again
 int ret = (str_cur * 8) / (tmp.weight() >= 150 ? tmp.weight() / 113 : 10 - int(tmp.weight() / 15));
 ret -= int(tmp.volume() / 4);
 if (has_active_bionic("bio_railgun") && (tmp.made_of("iron") || tmp.made_of("steel")))
    ret *= 2;
 if (ret < 1)
  return 1;
// Cap at double our strength + skill
 if (ret > str_cur * 1.5 + skillLevel("throw"))
   return str_cur * 1.5 + skillLevel("throw");
 return ret;
}

int player::ranged_dex_mod(bool return_stat_effect)
{
  // Stat window shows stat effects on based on current stat
    const int dex = (return_stat_effect ? get_dex() : get_dex());

    if (dex >= 12) { return 0; }
    return 12 - dex;
}

int player::ranged_per_mod(bool return_stat_effect)
{
  // Stat window shows stat effects on based on current stat
 const int per = (return_stat_effect ? get_per() : get_per());

 if (per >= 12) { return 0; }
 return 12 - per;
}

int player::throw_dex_mod(bool return_stat_effect)
{
  // Stat window shows stat effects on based on current stat
 int dex = (return_stat_effect ? get_dex() : get_dex());
 if (dex == 8 || dex == 9)
  return 0;
 if (dex >= 10)
  return (return_stat_effect ? 0 - rng(0, dex - 9) : 9 - dex);

 int deviation = 0;
 if (dex < 4)
  deviation = 4 * (8 - dex);
 else if (dex < 6)
  deviation = 3 * (8 - dex);
 else
  deviation = 2 * (8 - dex);

 // return_stat_effect actually matters here
 return (return_stat_effect ? rng(0, deviation) : deviation);
}

int player::read_speed(bool return_stat_effect)
{
  // Stat window shows stat effects on based on current stat
 int intel = (return_stat_effect ? get_int() : get_int());
 int ret = 1000 - 50 * (intel - 8);
 if (has_trait("FASTREADER"))
  ret *= .8;
 if (has_trait("SLOWREADER"))
  ret *= 1.3;
 if (ret < 100)
  ret = 100;
 // return_stat_effect actually matters here
 return (return_stat_effect ? ret : ret / 10);
}

int player::rust_rate(bool return_stat_effect)
{
    if (OPTIONS["SKILL_RUST"] == "off") {
        return 0;
    }

    // Stat window shows stat effects on based on current stat
    int intel = (return_stat_effect ? get_int() : get_int());
    int ret = ((OPTIONS["SKILL_RUST"] == "vanilla" || OPTIONS["SKILL_RUST"] == "capped") ? 500 : 500 - 35 * (intel - 8));

    if (has_trait("FORGETFUL")) {
        ret *= 1.33;
    }

    if (has_trait("GOODMEMORY")) {
        ret *= .66;
    }

    if (ret < 0) {
        ret = 0;
    }

    // return_stat_effect actually matters here
    return (return_stat_effect ? ret : ret / 10);
}

int player::talk_skill()
{
    int ret = get_int() + get_per() + skillLevel("speech") * 3;
    if (has_trait("SAPIOVORE")) {
        ret -= 20; // Friendly convo with your prey? unlikely
    } else if (has_trait("UGLY")) {
        ret -= 3;
    } else if (has_trait("DEFORMED")) {
        ret -= 6;
    } else if (has_trait("DEFORMED2")) {
        ret -= 12;
    } else if (has_trait("DEFORMED3")) {
        ret -= 18;
    } else if (has_trait("PRETTY")) {
        ret += 1;
    } else if (has_trait("BEAUTIFUL")) {
        ret += 2;
    } else if (has_trait("BEAUTIFUL2")) {
        ret += 4;
    } else if (has_trait("BEAUTIFUL3")) {
        ret += 6;
    }
    return ret;
}

int player::intimidation()
{
    int ret = get_str() * 2;
    if (weapon.is_gun()) {
        ret += 10;
    }
    if (weapon.damage_bash() >= 12 || weapon.damage_cut() >= 12) {
        ret += 5;
    }
    if (has_trait("SAPIOVORE")) {
        ret += 5; // Scaring one's prey, on the other claw...
    } else if (has_trait("DEFORMED2")) {
        ret += 3;
    } else if (has_trait("DEFORMED3")) {
        ret += 6;
    } else if (has_trait("PRETTY")) {
        ret -= 1;
    } else if (has_trait("BEAUTIFUL") || has_trait("BEAUTIFUL2") || has_trait("BEAUTIFUL3")) {
        ret -= 4;
    }
    if (stim > 20) {
        ret += 2;
    }
    if (has_disease("drunk")) {
        ret -= 4;
    }

    return ret;
}

bool player::is_dead_state() {
    return hp_cur[hp_head] <= 0 || hp_cur[hp_torso] <= 0;
}

void player::on_gethit(Creature *source, body_part bp_hit, damage_instance &) {
    bool u_see = g->u_see(this);
    if (source != NULL) {
        if (has_active_bionic("bio_ods")) {
            if (is_player()) {
                add_msg(m_good, _("Your offensive defense system shocks %s in mid-attack!"),
                                source->disp_name().c_str());
            } else if (u_see) {
                add_msg(_("%s's offensive defense system shocks %s in mid-attack!"),
                            disp_name().c_str(),
                            source->disp_name().c_str());
            }
            damage_instance ods_shock_damage;
            ods_shock_damage.add_damage(DT_ELECTRIC, rng(10,40));
            source->deal_damage(this, bp_torso, 3, ods_shock_damage);
        }
        if ((!(wearing_something_on(bp_hit))) && (has_trait("SPINES") || has_trait("QUILLS"))) {
            int spine = rng(1, (has_trait("QUILLS") ? 20 : 8));
            if (!is_player()) {
                if( u_see ) {
                    add_msg(_("%1$s's %2$s puncture %s in mid-attack!"), name.c_str(),
                                (has_trait("QUILLS") ? _("quills") : _("spines")),
                                source->disp_name().c_str());
                }
            } else {
                add_msg(m_good, _("Your %s puncture %s in mid-attack!"),
                                (has_trait("QUILLS") ? _("quills") : _("spines")),
                                source->disp_name().c_str());
            }
            damage_instance spine_damage;
            spine_damage.add_damage(DT_STAB, spine);
            source->deal_damage(this, bp_torso, 3, spine_damage);
        }
        if ((!(wearing_something_on(bp_hit))) && (has_trait("THORNS")) && (!(source->has_weapon()))) {
            if (!is_player()) {
                if( u_see ) {
                    add_msg(_("%1$s's %2$s scrape %s in mid-attack!"), name.c_str(),
                                (_("thorns"), source->disp_name().c_str()));
                }
            } else {
                add_msg(m_good, _("Your thorns scrape %s in mid-attack!"), source->disp_name().c_str());
            }
            int thorn = rng(1, 4);
            damage_instance thorn_damage;
            thorn_damage.add_damage(DT_CUT, thorn);
            // In general, critters don't have separate limbs
            // so safer to target the torso
            source->deal_damage(this, bp_torso, 3, thorn_damage);
        }
    }
}

dealt_damage_instance player::deal_damage(Creature* source, body_part bp,
                                          int side, const damage_instance& d) {

    dealt_damage_instance dealt_dams = Creature::deal_damage(source, bp, side, d); //damage applied here
    int dam = dealt_dams.total_damage(); //block reduction should be by applied this point

    if (has_disease("sleep")) {
        wake_up(_("You wake up!"));
    } else if (has_disease("lying_down")) {
        rem_disease("lying_down");
    }

    if (is_player()) {
        g->cancel_activity_query(_("You were hurt!"));
    }

    // TODO: Pre or post blit hit tile onto "this"'s location here
    if(g->u_see(this->posx, this->posy)) {
        g->draw_hit_player(this, dam);

        if (dam > 0 && is_player() && source) {
            //monster hits player melee
            nc_color color;
            std::string health_bar = "";
            get_HP_Bar(dam, this->get_hp_max(bodypart_to_hp_part(bp, side)), color, health_bar);

            SCT.add(this->xpos(),
                    this->ypos(),
                    direction_from(0, 0, this->xpos() - source->xpos(), this->ypos() - source->ypos()),
                    health_bar.c_str(), m_bad,
                    body_part_name(bp, side), m_neutral);
        }
    }

    // handle snake artifacts
    if (has_artifact_with(AEP_SNAKES) && dam >= 6) {
        int snakes = int(dam / 6);
        std::vector<point> valid;
        for (int x = posx - 1; x <= posx + 1; x++) {
            for (int y = posy - 1; y <= posy + 1; y++) {
                if (g->is_empty(x, y)) {
                    valid.push_back( point(x, y) );
                }
            }
        }
        if (snakes > valid.size()) {
            snakes = valid.size();
        }
        if (snakes == 1) {
            add_msg(m_warning, _("A snake sprouts from your body!"));
        } else if (snakes >= 2) {
            add_msg(m_warning, _("Some snakes sprout from your body!"));
        }
        monster snake(GetMType("mon_shadow_snake"));
        for (int i = 0; i < snakes; i++) {
            int index = rng(0, valid.size() - 1);
            point sp = valid[index];
            valid.erase(valid.begin() + index);
            snake.spawn(sp.x, sp.y);
            snake.friendly = -1;
            g->add_zombie(snake);
        }
    }

    // And slimespawners too
    if ((has_trait("SLIMESPAWNER")) && (dam >= 10) && one_in(20 - dam)) {
        std::vector<point> valid;
        for (int x = posx - 1; x <= posx + 1; x++) {
            for (int y = posy - 1; y <= posy + 1; y++) {
                if (g->is_empty(x, y)) {
                    valid.push_back( point(x, y) );
                }
            }
        }
        add_msg(m_warning, _("Slime is torn from you, and moves on its own!"));
        int numslime = 1;
        monster slime(GetMType("mon_player_blob"));
        for (int i = 0; i < numslime; i++) {
            int index = rng(0, valid.size() - 1);
            point sp = valid[index];
            valid.erase(valid.begin() + index);
            slime.spawn(sp.x, sp.y);
            slime.friendly = -1;
            g->add_zombie(slime);
        }
    }

    if (has_trait("ADRENALINE") && !has_disease("adrenaline") &&
        (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15)) {
        add_disease("adrenaline", 200);
    }

    int cut_dam = dealt_dams.type_damage(DT_CUT);
    switch (bp) {
    case bp_eyes:
        if (dam > 5 || cut_dam > 0) {
            int minblind = int((dam + cut_dam) / 10);
            if (minblind < 1) {
                minblind = 1;
            }
            int maxblind = int((dam + cut_dam) /  4);
            if (maxblind > 5) {
                maxblind = 5;
            }
            add_effect("blind", rng(minblind, maxblind));
        }

    /*
        It almost looks like damage may be getting applied twice in some cases.
     */
    case bp_mouth: // Fall through to head damage
    case bp_head:
        hp_cur[hp_head] -= dam; //this looks like an extra damage hit, as is applied in apply_damage from player::apply_damage()
        if (hp_cur[hp_head] < 0) {
            lifetime_stats()->damage_taken+=hp_cur[hp_head];
            hp_cur[hp_head] = 0;
        }
        break;
    case bp_torso:
        // getting hit throws off our shooting
        recoil += int(dam / 5);
        break;
    case bp_hands: // Fall through to arms
    case bp_arms:
        // getting hit in the arms throws off our shooting
        if (side == 1 || weapon.is_two_handed(this)) {
            recoil += int(dam / 3);
        }
        break;
    case bp_feet: // Fall through to legs
    case bp_legs:
        break;
    default:
        debugmsg("Wacky body part hit!");
    }
    //looks like this should be based off of dealtdams, not d as d has no damage reduction applied.
    // Skip all this if the damage isn't from a creature. e.g. an explosion.
    if( source != NULL ) {
        if (dealt_dams.total_damage() > 0 && source->has_flag(MF_VENOM)) {
            add_msg_if_player(m_bad, _("You're poisoned!"));
            add_disease("poison", 30, false, 1, 20, 100);
            add_effect("poison", 30);
        }
        else if (dealt_dams.total_damage() > 0 && source->has_flag(MF_BADVENOM)) {
            add_msg_if_player(m_bad, _("You feel poison flood your body, wracking you with pain..."));
            add_disease("badpoison", 40, false, 1, 20, 100);
            add_effect("badpoison", 40);
        }
        else if (dealt_dams.total_damage() > 0 && source->has_flag(MF_PARALYZE)) {
            add_msg_if_player(m_bad, _("You feel poison enter your body!"));
            add_disease("paralyzepoison", 100, false, 1, 20, 100);
            add_effect("paralyzepoison", 100);
        }

        if (source->has_flag(MF_BLEED) && dealt_dams.total_damage() > 6 &&
            dealt_dams.type_damage(DT_CUT) > 0) {
            // Maybe should only be if DT_CUT > 6... Balance question
            add_msg_if_player(m_bad, _("You're Bleeding!"));
            // Only place bleed effect added to player in code
            add_disease("bleed", 60, false, 1, 3, 120, 1, bp, side, true);
        }

        if ( source->has_flag(MF_GRABS)) {
            add_msg_player_or_npc(m_bad, _("%s grabs you!"), _("%s grabs <npcname>!"),
                                  source->disp_name().c_str());
            if (has_grab_break_tec() && get_grab_resist() > 0 && get_dex() > get_str() ?
                dice(get_dex(), 10) : dice(get_str(), 10) > dice(source->get_dex(), 10)) {
                add_msg_player_or_npc(m_good, _("You break the grab!"),
                                      _("<npcname> breaks the grab!"));
            } else {
                add_disease("grabbed", 1, false, 1, 3, 1, 1);
            }
        }
    }

    return dealt_damage_instance(dealt_dams);
}
/*
    Where damage to player is actually applied to hit body parts
    Might be where to put bleed stuff rather than in player::deal_damage()
 */
void player::apply_damage(Creature *, body_part bp, int side, int dam) {
    if (is_dead_state()) {
        // don't do any more damage if we're already dead
        return;
    }

    switch (bp) {
    case bp_eyes: // Fall through to head damage
    case bp_mouth: // Fall through to head damage
    case bp_head:
        hp_cur[hp_head] -= dam;
        if (hp_cur[hp_head] < 0) {
            lifetime_stats()->damage_taken+=hp_cur[hp_head];
            hp_cur[hp_head] = 0;
        }
        break;
    case bp_torso:
        hp_cur[hp_torso] -= dam;
        if (hp_cur[hp_torso] < 0) {
            lifetime_stats()->damage_taken+=hp_cur[hp_torso];
            hp_cur[hp_torso] = 0;
        }
        break;
    case bp_hands: // Fall through to arms
    case bp_arms:
        if (side == 0) {
            hp_cur[hp_arm_l] -= dam;
            if (hp_cur[hp_arm_l] < 0) {
                lifetime_stats()->damage_taken+=hp_cur[hp_arm_l];
                hp_cur[hp_arm_l] = 0;
            }
        }
        if (side == 1) {
            hp_cur[hp_arm_r] -= dam;
            if (hp_cur[hp_arm_r] < 0) {
                lifetime_stats()->damage_taken+=hp_cur[hp_arm_r];
                hp_cur[hp_arm_r] = 0;
            }
        }
        break;
    case bp_feet: // Fall through to legs
    case bp_legs:
        if (side == 0) {
            hp_cur[hp_leg_l] -= dam;
            if (hp_cur[hp_leg_l] < 0) {
                lifetime_stats()->damage_taken+=hp_cur[hp_leg_l];
                hp_cur[hp_leg_l] = 0;
            }
        }
        if (side == 1) {
            hp_cur[hp_leg_r] -= dam;
            if (hp_cur[hp_leg_r] < 0) {
                lifetime_stats()->damage_taken+=hp_cur[hp_leg_r];
                hp_cur[hp_leg_r] = 0;
            }
        }
        break;
    default:
        debugmsg("Wacky body part hurt!");
    }
    lifetime_stats()->damage_taken+=dam;

}

void player::mod_pain(int npain) {
    if ((has_trait("NOPAIN"))) {
        return;
    }
    if (has_trait("PAINRESIST") && npain > 1) {
        // if it's 1 it'll just become 0, which is bad
        npain = npain * 4 / rng(4,8);
    }
    // Dwarves get better pain-resist, what with mining and all
    if (has_trait("PAINRESIST_TROGLO") && npain > 1) {
        npain = npain * 4 / rng(6,9);
    }
    Creature::mod_pain(npain);
}

void player::hurt(body_part hurt, int side, int dam)
{
    hp_part hurtpart;
    switch (hurt) {
        case bp_eyes: // Fall through to head damage
        case bp_mouth: // Fall through to head damage
        case bp_head:
            hurtpart = hp_head;
            break;
        case bp_torso:
            hurtpart = hp_torso;
            break;
        case bp_hands:
            // Shouldn't happen, but fall through to arms
            debugmsg("Hurt against hands!");
        case bp_arms:
            if (side == 0) {
                hurtpart = hp_arm_l;
            } else {
                hurtpart = hp_arm_r;
            }
            break;
        case bp_feet:
            // Shouldn't happen, but fall through to legs
            debugmsg("Hurt against feet!");
        case bp_legs:
            if (side == 0) {
                hurtpart = hp_leg_l;
            } else {
                hurtpart = hp_leg_r;
            }
            break;
        default:
            debugmsg("Wacky body part hurt!");
            hurtpart = hp_torso;
    }
    if (has_disease("sleep") && rng(0, dam) > 2) {
        wake_up(_("You wake up!"));
    } else if (has_disease("lying_down")) {
        rem_disease("lying_down");
    }

    if (dam <= 0) {
        return;
    }

    if (!is_npc()) {
        g->cancel_activity_query(_("You were hurt!"));
    }

    mod_pain( dam /2 );

    if (has_trait("ADRENALINE") && !has_disease("adrenaline") &&
        (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15)) {
        add_disease("adrenaline", 200);
    }
    hp_cur[hurtpart] -= dam;
    if (hp_cur[hurtpart] < 0) {
        lifetime_stats()->damage_taken += hp_cur[hurt];
        hp_cur[hurtpart] = 0;
    }
    lifetime_stats()->damage_taken += dam;
}

void player::hurt(hp_part hurt, int dam)
{
    if (has_disease("sleep") && rng(0, dam) > 2) {
        wake_up(_("You wake up!"));
    } else if (has_disease("lying_down")) {
        rem_disease("lying_down");
    }

    if (dam <= 0) {
        return;
    }

    if (!is_npc()) {
        g->cancel_activity_query(_("You were hurt!"));
    }

    mod_pain( dam / 2 );

    hp_cur[hurt] -= dam;
    if (hp_cur[hurt] < 0) {
        lifetime_stats()->damage_taken += hp_cur[hurt];
        hp_cur[hurt] = 0;
    }
    lifetime_stats()->damage_taken += dam;
}

void player::heal(body_part healed, int side, int dam)
{
    hp_part healpart;
    switch (healed) {
        case bp_eyes: // Fall through to head damage
        case bp_mouth: // Fall through to head damage
        case bp_head:
            healpart = hp_head;
            break;
        case bp_torso:
            healpart = hp_torso;
            break;
        case bp_hands:
            // Shouldn't happen, but fall through to arms
            debugmsg("Heal against hands!");
        case bp_arms:
            if (side == 0) {
                healpart = hp_arm_l;
            } else {
                healpart = hp_arm_r;
            }
            break;
        case bp_feet:
            // Shouldn't happen, but fall through to legs
            debugmsg("Heal against feet!");
        case bp_legs:
            if (side == 0) {
                healpart = hp_leg_l;
            } else {
                healpart = hp_leg_r;
            }
            break;
        default:
            debugmsg("Wacky body part healed!");
            healpart = hp_torso;
    }
    hp_cur[healpart] += dam;
    if (hp_cur[healpart] > hp_max[healpart]) {
        lifetime_stats()->damage_healed -= hp_cur[healpart] - hp_max[healpart];
        hp_cur[healpart] = hp_max[healpart];
    }
    lifetime_stats()->damage_healed+=dam;
}

void player::heal(hp_part healed, int dam)
{
    hp_cur[healed] += dam;
    if (hp_cur[healed] > hp_max[healed]) {
        lifetime_stats()->damage_healed -= hp_cur[healed] - hp_max[healed];
        hp_cur[healed] = hp_max[healed];
    }
    lifetime_stats()->damage_healed += dam;
}

void player::healall(int dam)
{
    for (int i = 0; i < num_hp_parts; i++) {
        if (hp_cur[i] > 0) {
            hp_cur[i] += dam;
            if (hp_cur[i] > hp_max[i]) {
                lifetime_stats()->damage_healed -= hp_cur[i] - hp_max[i];
                hp_cur[i] = hp_max[i];
            }
            lifetime_stats()->damage_healed += dam;
        }
    }
}

void player::hurtall(int dam)
{
    for (int i = 0; i < num_hp_parts; i++) {
        hp_cur[i] -= dam;
        if (hp_cur[i] < 0) {
            lifetime_stats()->damage_taken += hp_cur[i];
            hp_cur[i] = 0;
        }
        mod_pain( dam / 2 );
        lifetime_stats()->damage_taken += dam;
    }
}

void player::hitall(int dam, int vary)
{
    if (has_disease("sleep")) {
        wake_up(_("You wake up!"));
    } else if (has_disease("lying_down")) {
        rem_disease("lying_down");
    }

    for (int i = 0; i < num_hp_parts; i++) {
        int ddam = vary? dam * rng (100 - vary, 100) / 100 : dam;
        int cut = 0;
        absorb((body_part) i, ddam, cut);
        hp_cur[i] -= ddam;
        if (hp_cur[i] < 0)
        {
            lifetime_stats()->damage_taken+=hp_cur[i];
            hp_cur[i] = 0;
        }

        mod_pain( dam / 2 / 4 );
        lifetime_stats()->damage_taken+=dam;
    }
}

void player::knock_back_from(int x, int y)
{
 if (x == posx && y == posy)
  return; // No effect
 point to(posx, posy);
 if (x < posx)
  to.x++;
 if (x > posx)
  to.x--;
 if (y < posy)
  to.y++;
 if (y > posy)
  to.y--;

// First, see if we hit a monster
 int mondex = g->mon_at(to.x, to.y);
 if (mondex != -1) {
  monster *critter = &(g->zombie(mondex));
  hit(this, bp_torso, -1, critter->type->size, 0);
  add_effect("stunned", 1);
  if ((str_max - 6) / 4 > critter->type->size) {
   critter->knock_back_from(posx, posy); // Chain reaction!
   critter->hurt((str_max - 6) / 4);
   critter->add_effect("stunned", 1);
  } else if ((str_max - 6) / 4 == critter->type->size) {
   critter->hurt((str_max - 6) / 4);
   critter->add_effect("stunned", 1);
  }

  add_msg_player_or_npc(_("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                            critter->name().c_str() );

  return;
 }

 int npcdex = g->npc_at(to.x, to.y);
 if (npcdex != -1) {
  npc *p = g->active_npc[npcdex];
  hit(this, bp_torso, -1, 3, 0);
  add_effect("stunned", 1);
  p->hit(this, bp_torso, -1, 3, 0);
  add_msg_player_or_npc( _("You bounce off %s!"), _("<npcname> bounces off %s!"), p->name.c_str() );
  return;
 }

// If we're still in the function at this point, we're actually moving a tile!
 if (g->m.has_flag("LIQUID", to.x, to.y) && g->m.has_flag(TFLAG_DEEP_WATER, to.x, to.y)) {
  if (!is_npc()) {
   g->plswim(to.x, to.y);
  }
// TODO: NPCs can't swim!
 } else if (g->m.move_cost(to.x, to.y) == 0) { // Wait, it's a wall (or water)

  // It's some kind of wall.
  hurt(bp_torso, -1, 3);
  add_effect("stunned", 2);
  add_msg_player_or_npc( _("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                             g->m.tername(to.x, to.y).c_str() );

 } else { // It's no wall
  posx = to.x;
  posy = to.y;
 }
}

void player::bp_convert(hp_part &hpart, body_part bp, int side)
{
    hpart =  num_hp_parts;
    switch(bp) {
        case bp_head:
            hpart = hp_head;
            break;
        case bp_torso:
            hpart = hp_torso;
            break;
        case bp_arms:
            if (side == 0) {
                hpart = hp_arm_l;
            } else {
                hpart = hp_arm_r;
            }
            break;
        case bp_legs:
            if (side == 0) {
                hpart = hp_leg_l;
            } else {
                hpart = hp_leg_r;
            }
            break;
    }
}

void player::hp_convert(hp_part hpart, body_part &bp, int &side)
{
    bp =  num_bp;
    side = -1;
    switch(hpart) {
        case hp_head:
            bp = bp_head;
            break;
        case hp_torso:
            bp = bp_torso;
            break;
        case hp_arm_l:
            bp = bp_arms;
            side = 0;
            break;
        case hp_arm_r:
            bp = bp_arms;
            side = 1;
            break;
        case hp_leg_l:
            bp = bp_legs;
            side = 0;
            break;
        case hp_leg_r:
            bp = bp_legs;
            side = 1;
            break;
    }
}

int player::hp_percentage()
{
 int total_cur = 0, total_max = 0;
// Head and torso HP are weighted 3x and 2x, respectively
 total_cur = hp_cur[hp_head] * 3 + hp_cur[hp_torso] * 2;
 total_max = hp_max[hp_head] * 3 + hp_max[hp_torso] * 2;
 for (int i = hp_arm_l; i < num_hp_parts; i++) {
  total_cur += hp_cur[i];
  total_max += hp_max[i];
 }
 return (100 * total_cur) / total_max;
}

void player::recalc_hp()
{
    int new_max_hp[num_hp_parts];
    for (int i = 0; i < num_hp_parts; i++)
    {
        new_max_hp[i] = 60 + str_max * 3;
        if (has_trait("HUGE")) {
            // Bad-Huge doesn't quite have the cardio/skeletal/etc to support the mass,
            // so no HP bonus from the ST above/beyond that from Large
            new_max_hp[i] -= 6;
        }
        // You lose half the HP you'd expect from BENDY mutations.  Your gelatinous
        // structure can help with that, a bit.
        if (has_trait("BENDY2")) {
            new_max_hp[i] += 3;
        }
        if (has_trait("BENDY3")) {
            new_max_hp[i] += 6;
        }
        // Only the most extreme applies.
        if (has_trait("TOUGH")) {
            new_max_hp[i] *= 1.2;
        } else if (has_trait("TOUGH2")) {
            new_max_hp[i] *= 1.3;
        } else if (has_trait("TOUGH3")) {
            new_max_hp[i] *= 1.4;
        } else if (has_trait("FLIMSY")) {
            new_max_hp[i] *= .75;
        } else if (has_trait("FLIMSY2")) {
            new_max_hp[i] *= .5;
        } else if (has_trait("FLIMSY3")) {
            new_max_hp[i] *= .25;
        }
        // Mutated toughness stacks with starting, by design.
        if (has_trait("MUT_TOUGH")) {
            new_max_hp[i] *= 1.2;
        } else if (has_trait("MUT_TOUGH2")) {
            new_max_hp[i] *= 1.3;
        } else if (has_trait("MUT_TOUGH3")) {
            new_max_hp[i] *= 1.4;
        }
    }
    if (has_trait("GLASSJAW"))
    {
        new_max_hp[hp_head] *= 0.8;
    }
    for (int i = 0; i < num_hp_parts; i++)
    {
        hp_cur[i] *= (float)new_max_hp[i]/(float)hp_max[i];
        hp_max[i] = new_max_hp[i];
    }
}

void player::get_sick()
{
 if (health > 0 && rng(0, health + 10) < health)
  health--;
 if (health < 0 && rng(0, 10 - health) < (0 - health))
  health++;
 if (one_in(12))
  health -= 1;

 if (g->debugmon)
  debugmsg("Health: %d", health);

 if (has_trait("DISIMMUNE"))
  return;

 if (!has_disease("flu") && !has_disease("common_cold") &&
     one_in(900 + 10 * health + (has_trait("DISRESISTANT") ? 300 : 0))) {
  if (one_in(6))
   infect("flu", bp_mouth, 3, rng(40000, 80000));
  else
   infect("common_cold", bp_mouth, 3, rng(20000, 60000));
 }
}

bool player::infect(dis_type type, body_part vector, int strength,
                     int duration, bool permanent, int intensity,
                     int max_intensity, int decay, int additive, bool targeted,
                     int side, bool main_parts_only)
{
    if (strength <= 0) {
        return false;
    }

    if (dice(strength, 3) > dice(get_env_resist(vector), 3)) {
        if (targeted) {
            add_disease(type, duration, permanent, intensity, max_intensity, decay,
                          additive, vector, side, main_parts_only);
        } else {
            add_disease(type, duration, permanent, intensity, max_intensity, decay, additive);
        }
        return true;
    }

    return false;
}

void player::add_disease(dis_type type, int duration, bool permanent,
                         int intensity, int max_intensity, int decay,
                         int additive, body_part part, int side,
                         bool main_parts_only)
{
    if (duration <= 0) {
        return;
    }

    if (part !=  num_bp && hp_cur[part] == 0) {
        return;
    }

    if (main_parts_only) {
        if (part == bp_eyes || part == bp_mouth) {
            part = bp_head;
        } else if (part == bp_hands) {
            part = bp_arms;
        } else if (part == bp_feet) {
            part = bp_legs;
        }
    }

    bool found = false;
    int i = 0;
    while ((i < illness.size()) && !found) {
        if (illness[i].type == type) {
            if ((part == num_bp) ^ (illness[i].bp == num_bp)) {
                debugmsg("Bodypart missmatch when applying disease %s",
                         type.c_str());
                return;
            } else if (illness[i].bp == part &&
                       ((illness[i].side == -1) ^ (side == -1))) {
                debugmsg("Side of body missmatch when applying disease %s",
                         type.c_str());
                return;
            }
            if (illness[i].bp == part && illness[i].side == side) {
                if (additive > 0) {
                    illness[i].duration += duration;
                } else if (additive < 0) {
                    illness[i].duration -= duration;
                    if (illness[i].duration <= 0) {
                        illness[i].duration = 1;
                    }
                }
                illness[i].intensity += intensity;
                if (max_intensity != -1 && illness[i].intensity > max_intensity) {
                    illness[i].intensity = max_intensity;
                }
                if (permanent) {
                    illness[i].permanent = true;
                }
                illness[i].decay = decay;
                found = true;
            }
        }
        i++;
    }
    if (!found) {
        disease tmp(type, duration, intensity, part, side, permanent, decay);
        illness.push_back(tmp);

        if (!is_npc()) {
            if (dis_msg(type)) {
                SCT.add(this->xpos(),
                        this->ypos(),
                        SOUTH,
                        dis_name(tmp), m_info);
            }
        }
    }

    recalc_sight_limits();
}

void player::rem_disease(dis_type type, body_part part, int side)
{
    for (int i = 0; i < illness.size();) {
        if (illness[i].type == type &&
            ( part == num_bp || illness[i].bp == part ) &&
            ( side == -1 || illness[i].side == side ) ) {
            illness.erase(illness.begin() + i);
            if(!is_npc()) {
                dis_remove_memorial(type);
            }
        } else {
            i++;
        }
    }

    recalc_sight_limits();
}

bool player::has_disease(dis_type type, body_part part, int side) const
{
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == type &&
            ( part == num_bp || illness[i].bp == part ) &&
            ( side == -1 || illness[i].side == side ) ) {
            return true;
        }
    }
    return false;
}

bool player::pause_disease(dis_type type, body_part part, int side)
{
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == type &&
            ( part == num_bp || illness[i].bp == part ) &&
            ( side == -1 || illness[i].side == side ) ) {
                illness[i].permanent = true;
                return true;
        }
    }
    return false;
}

bool player::unpause_disease(dis_type type, body_part part, int side)
{
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == type &&
            ( part == num_bp || illness[i].bp == part ) &&
            ( side == -1 || illness[i].side == side ) ) {
                illness[i].permanent = false;
                return true;
        }
    }
    return false;
}

int player::disease_duration(dis_type type, bool all, body_part part, int side)
{
    int tmp = 0;
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == type && (part ==  num_bp || illness[i].bp == part) &&
            (side == -1 || illness[i].side == side)) {
            if (all == false) {
                return illness[i].duration;
            } else {
                tmp += illness[i].duration;
            }
        }
    }
    return tmp;
}

int player::disease_intensity(dis_type type, bool all, body_part part, int side)
{
    int tmp = 0;
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == type && (part ==  num_bp || illness[i].bp == part) &&
            (side == -1 || illness[i].side == side)) {
            if (all == false) {
                return illness[i].intensity;
            } else {
                tmp += illness[i].intensity;
            }
        }
    }
    return tmp;
}

void player::add_addiction(add_type type, int strength)
{
 if (type == ADD_NULL)
  return;
 int timer = 1200;
 if (has_trait("ADDICTIVE")) {
  strength = int(strength * 1.5);
  timer = 800;
 }
 if (has_trait("NONADDICTIVE")) {
  strength = int(strength * .50);
  timer = 1800;
 }
 //Update existing addiction
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type) {
   if (addictions[i].sated < 0) {
    addictions[i].sated = timer;
   } else if (addictions[i].sated < 600) {
    addictions[i].sated += timer; // TODO: Make this variable?
   } else {
    addictions[i].sated += int((3000 - addictions[i].sated) / 2);
   }
   if ((rng(0, strength) > rng(0, addictions[i].intensity * 5) ||
       rng(0, 500) < strength) && addictions[i].intensity < 20) {
    addictions[i].intensity++;
   }
   return;
  }
 }
 //Add a new addiction
 if (rng(0, 100) < strength) {
  //~ %s is addiction name
  add_memorial_log(pgettext("memorial_male", "Became addicted to %s."),
                   pgettext("memorial_female", "Became addicted to %s."),
                   addiction_type_name(type).c_str());
  addiction tmp(type, 1);
  addictions.push_back(tmp);
 }
}

bool player::has_addiction(add_type type) const
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type &&
      addictions[i].intensity >= MIN_ADDICTION_LEVEL)
   return true;
 }
 return false;
}

void player::rem_addiction(add_type type)
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type) {
   //~ %s is addiction name
   add_memorial_log(pgettext("memorial_male", "Overcame addiction to %s."),
                    pgettext("memorial_female", "Overcame addiction to %s."),
                    addiction_type_name(type).c_str());
   addictions.erase(addictions.begin() + i);
   return;
  }
 }
}

int player::addiction_level(add_type type)
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type)
   return addictions[i].intensity;
 }
 return 0;
}

bool player::siphon(vehicle *veh, ammotype desired_liquid)
{
    int liquid_amount = veh->drain( desired_liquid, veh->fuel_capacity(desired_liquid) );
    item used_item( default_ammo(desired_liquid), calendar::turn );
    used_item.charges = liquid_amount;
    int extra = g->move_liquid( used_item );
    if( extra == -1 ) {
        // Failed somehow, put the liquid back and bail out.
        veh->refill( desired_liquid, used_item.charges );
        return false;
    }
    int siphoned = liquid_amount - extra;
    veh->refill( desired_liquid, extra );
    if( siphoned > 0 ) {
        add_msg(ngettext("Siphoned %d unit of %s from the %s.",
                            "Siphoned %d units of %s from the %s.",
                            siphoned),
                   siphoned, used_item.tname().c_str(), veh->name.c_str());
        //Don't consume turns if we decided not to siphon
        return true;
    } else {
        return false;
    }
}

static void manage_fire_exposure(player &p, int fireStrength) {
    // TODO: this should be determined by material properties
    p.hurtall(3*fireStrength);
    for (int i = 0; i < p.worn.size(); i++) {
        item tmp = p.worn[i];
        bool burnVeggy = (tmp.made_of("veggy") || tmp.made_of("paper"));
        bool burnFabric = ((tmp.made_of("cotton") || tmp.made_of("wool")) && one_in(10*fireStrength));
        bool burnPlastic = ((tmp.made_of("plastic")) && one_in(50*fireStrength));
        if (burnVeggy || burnFabric || burnPlastic) {
            p.worn.erase(p.worn.begin() + i);
            i--;
        }
    }
}
static void handle_cough(player &p, int intensity, int loudness) {
    if (p.is_player()) {
        add_msg(m_bad, _("You cough heavily."));
        g->sound(p.posx, p.posy, loudness, "");
    } else {
        g->sound(p.posx, p.posy, loudness, _("a hacking cough."));
    }
    p.mod_moves(-80);
    if (rng(1,6) < intensity) {
        p.apply_damage(NULL, bp_torso, -1, 1);
    }
    if (p.has_disease("sleep") && intensity >= 2) {
        p.wake_up(_("You wake up coughing."));
    }
}
void player::process_effects() {
    int psnChance;
    for( auto effect_it = effects.begin(); effect_it != effects.end(); ++effect_it ) {
        std::string id = effect_it->second.get_id();
        if (id == "onfire") {
            manage_fire_exposure(*this, 1);
        } else if (id == "poison") {
            psnChance = 150;
            if (has_trait("POISRESIST")) {
                psnChance *= 6;
            } else {
                mod_str_bonus(-2);
                mod_per_bonus(-1);
            }
            // Increased body mass means poison's less effective
            if (has_trait("FAT")) {
                psnChance *= 1.5;
            }
            if (has_trait("LARGE") || has_trait("LARGE_OK")) {
                psnChance *= 2;
            }
            if (has_trait("HUGE") || has_trait("HUGE_OK")) {
                psnChance *= 3;
            }
            if ((one_in(psnChance)) && (!(has_trait("NOPAIN")))) {
                add_msg_if_player(m_bad, _("You're suddenly wracked with pain!"));
                mod_pain(1);
                hurt(bp_torso, -1, rng(0, 2) * rng(0, 1));
            }
            mod_per_bonus(-1);
            mod_dex_bonus(-1);
        } else if (id == "glare") {
            mod_per_bonus(-1);
            if (one_in(200)) {
                add_msg_if_player(m_bad, _("The sunlight's glare makes it hard to see."));
            }
        } else if (id == "smoke") {
            // A hard limit on the duration of the smoke disease.
            if( effect_it->second.get_duration() >= 600) {
                effect_it->second.set_duration(600);
            }
            mod_str_bonus(-1);
            mod_dex_bonus(-1);
            effect_it->second.set_intensity((effect_it->second.get_duration()+190)/200);
            if( effect_it->second.get_intensity() >= 10 && one_in(6)) {
                handle_cough(*this, effect_it->second.get_intensity());
            }
        } else if (id == "teargas") {
            mod_str_bonus(-2);
            mod_dex_bonus(-2);
            mod_per_bonus(-5);
            if (one_in(3)) {
                handle_cough(*this, 4);
            }
        } else if ( id == "stung" ) {
            mod_pain(1);
        }
    }

    Creature::process_effects();
}

void player::suffer()
{
    for (int i = 0; i < my_bionics.size(); i++) {
        if (my_bionics[i].powered) {
            activate_bionic(i);
        }
    }
    if (underwater) {
        if (!has_trait("GILLS")) {
            oxygen--;
        }
        if (oxygen < 12 && worn_with_flag("REBREATHER")) {
                oxygen += 12;
            }
        if (oxygen < 0) {
            if (has_bionic("bio_gills") && power_level > 0) {
                oxygen += 5;
                power_level--;
            } else {
                add_msg(m_bad, _("You're drowning!"));
                hurt(bp_torso, -1, rng(1, 4));
            }
        }
    }

    if( has_trait("ROOTS3") && g->m.has_flag("DIGGABLE", posx, posy) &&
        (!(wearing_something_on(bp_feet))) ) {
        if (one_in(25)) {
            add_msg(m_good, _("This soil is delicious!"));
            hunger -= 2;
            thirst -= 2;
            if (health <= 10) {
                health++;
            }
            // Mmm, dat soil...
            focus_pool--;
        } else if (one_in(20)){
            hunger--;
            thirst--;
            if (health <= 5) {
                health++;
            }
        }
    }

    for (int i = 0; i < illness.size(); i++) {
        dis_effect(*this, illness[i]);
    }

    // Diseases may remove themselves as part of applying (MA buffs do) so do a
    // separate loop through the remaining ones for duration, decay, etc..
    for (int i = 0; i < illness.size(); i++) {
        if (!illness[i].permanent) {
            illness[i].duration--;
        }
        if (illness[i].decay > 0 && one_in(illness[i].decay)) {
            illness[i].intensity--;
        }
        if (illness[i].duration <= 0 || illness[i].intensity == 0) {
            dis_end_msg(*this, illness[i]);
            illness.erase(illness.begin() + i);
            i--;
        }
    }

    if (!has_disease("sleep")) {
        if (weight_carried() > weight_capacity()) {
            // Starts at 1 in 25, goes down by 5 for every 50% more carried
            if (one_in(35 - 5 * weight_carried() / (weight_capacity() / 2))) {
                add_msg_if_player(m_bad, _("Your body strains under the weight!"));
                // 1 more pain for every 800 grams more (5 per extra STR needed)
                if ( ((weight_carried() - weight_capacity()) / 800 > pain && pain < 100)) {
                    mod_pain(1);
                }
            }
        }
        if (weight_carried() > 4 * weight_capacity()) {
            if (has_trait("LEG_TENT_BRACE")){
                add_msg_if_player(m_bad, _("Your tentacles buckle under the weight!"));
            }
            if (has_effect("downed")) {
                add_effect("downed", 1);
            } else {
                add_effect("downed", 2);
            }
        }
        int timer = -3600;
        if (has_trait("ADDICTIVE")) {
            timer = -4000;
        }
        if (has_trait("NONADDICTIVE")) {
            timer = -3200;
        }
        for (int i = 0; i < addictions.size(); i++) {
            if (addictions[i].sated <= 0 &&
                addictions[i].intensity >= MIN_ADDICTION_LEVEL) {
                addict_effect(addictions[i]);
            }
            addictions[i].sated--;
            if (!one_in(addictions[i].intensity - 2) && addictions[i].sated > 0) {
                addictions[i].sated -= 1;
            }
            if (addictions[i].sated < timer - (100 * addictions[i].intensity)) {
                if (addictions[i].intensity <= 2) {
                    addictions.erase(addictions.begin() + i);
                    i--;
                } else {
                    addictions[i].intensity = int(addictions[i].intensity / 2);
                    addictions[i].intensity--;
                    addictions[i].sated = 0;
                }
            }
        }
        if (has_trait("CHEMIMBALANCE")) {
            if (one_in(3600) && (!(has_trait("NOPAIN")))) {
                add_msg(m_bad, _("You suddenly feel sharp pain for no reason."));
                mod_pain( 3 * rng(1, 3) );
            }
            if (one_in(3600)) {
                int pkilladd = 5 * rng(-1, 2);
                if (pkilladd > 0) {
                    add_msg(m_bad, _("You suddenly feel numb."));
                } else if ((pkilladd < 0) && (!(has_trait("NOPAIN")))) {
                    add_msg(m_bad, _("You suddenly ache."));
                }
                pkill += pkilladd;
            }
            if (one_in(3600)) {
                add_msg(m_bad, _("You feel dizzy for a moment."));
                moves -= rng(10, 30);
            }
            if (one_in(3600)) {
                int hungadd = 5 * rng(-1, 3);
                if (hungadd > 0) {
                    add_msg(m_bad, _("You suddenly feel hungry."));
                } else {
                    add_msg(m_good, _("You suddenly feel a little full."));
                }
                hunger += hungadd;
            }
            if (one_in(3600)) {
                add_msg(m_bad, _("You suddenly feel thirsty."));
                thirst += 5 * rng(1, 3);
            }
            if (one_in(3600)) {
                add_msg(m_good, _("You feel fatigued all of a sudden."));
                fatigue += 10 * rng(2, 4);
            }
            if (one_in(4800)) {
                if (one_in(3)) {
                    add_morale(MORALE_FEELING_GOOD, 20, 100);
                } else {
                    add_morale(MORALE_FEELING_BAD, -20, -100);
                }
            }
            if (one_in(3600)) {
                if (one_in(3)) {
                    add_msg(m_bad, _("You suddenly feel very cold."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_VERY_COLD;
                    }
                } else {
                    add_msg(m_bad, _("You suddenly feel cold."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_COLD;
                    }
                }
            }
            if (one_in(3600)) {
                if (one_in(3)) {
                    add_msg(m_bad, _("You suddenly feel very hot."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_VERY_HOT;
                    }
                } else {
                    add_msg(m_bad, _("You suddenly feel hot."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_HOT;
                    }
                }
            }
        }
        if ((has_trait("SCHIZOPHRENIC") || has_artifact_with(AEP_SCHIZO)) &&
            one_in(2400)) { // Every 4 hours or so
            monster phantasm;
            int i;
            switch(rng(0, 11)) {
                case 0:
                    add_disease("hallu", 3600);
                    break;
                case 1:
                    add_disease("visuals", rng(15, 60));
                    break;
                case 2:
                    add_msg(m_warning, _("From the south you hear glass breaking."));
                    break;
                case 3:
                    add_msg(m_warning, _("YOU SHOULD QUIT THE GAME IMMEDIATELY."));
                    add_morale(MORALE_FEELING_BAD, -50, -150);
                    break;
                case 4:
                    for (i = 0; i < 10; i++) {
                        add_msg("XXXXXXXXXXXXXXXXXXXXXXXXXXX");
                    }
                    break;
                case 5:
                    add_msg(m_bad, _("You suddenly feel so numb..."));
                    pkill += 25;
                    break;
                case 6:
                    add_msg(m_bad, _("You start to shake uncontrollably."));
                    add_disease("shakes", 10 * rng(2, 5));
                    break;
                case 7:
                    for (i = 0; i < 10; i++) {
                        g->spawn_hallucination();
                    }
                    break;
                case 8:
                    add_msg(m_bad, _("It's a good time to lie down and sleep."));
                    add_disease("lying_down", 200);
                    break;
                case 9:
                    add_msg(m_bad, _("You have the sudden urge to SCREAM!"));
                    g->sound(posx, posy, 10 + 2 * str_cur, "AHHHHHHH!");
                    break;
                case 10:
                    add_msg(std::string(name + name + name + name + name + name + name +
                        name + name + name + name + name + name + name +
                        name + name + name + name + name + name).c_str());
                    break;
                case 11:
                    body_part bp = random_body_part(true);
                    int side = random_side(bp);
                    add_disease("formication", 600, false, 1, 3, 0, 1, bp, side, true);
                    break;
            }
        }
        if (has_trait("JITTERY") && !has_disease("shakes")) {
            if (stim > 50 && one_in(300 - stim)) {
                add_disease("shakes", 300 + stim);
            } else if (hunger > 80 && one_in(500 - hunger)) {
                add_disease("shakes", 400);
            }
        }

        if (has_trait("MOODSWINGS") && one_in(3600)) {
            if (rng(1, 20) > 9) { // 55% chance
                add_morale(MORALE_MOODSWING, -100, -500);
            } else {  // 45% chance
                add_morale(MORALE_MOODSWING, 100, 500);
            }
        }

        if (has_trait("VOMITOUS") && one_in(4200)) {
            vomit();
        }
        if (has_trait("SHOUT1") && one_in(3600)) {
            g->sound(posx, posy, 10 + 2 * str_cur, _("You shout loudly!"));
        }
        if (has_trait("SHOUT2") && one_in(2400)) {
            g->sound(posx, posy, 15 + 3 * str_cur, _("You scream loudly!"));
        }
        if (has_trait("SHOUT3") && one_in(1800)) {
            g->sound(posx, posy, 20 + 4 * str_cur, _("You let out a piercing howl!"));
        }
    } // Done with while-awake-only effects

    if (has_trait("ASTHMA") && one_in(3600 - stim * 50)) {
        bool auto_use = has_charges("inhaler", 1);
        if (underwater) {
            oxygen = int(oxygen / 2);
            auto_use = false;
        }

        if (has_disease("sleep")) {
            wake_up(_("Your asthma wakes you up!"));
            auto_use = false;
        }

        if (auto_use) {
            use_charges("inhaler", 1);
        } else {
            add_disease("asthma", 50 * rng(1, 4));
            if (!is_npc()) {
                g->cancel_activity_query(_("You have an asthma attack!"));
            }
        }
    }

    if (has_trait("LEAVES") && g->is_in_sunlight(posx, posy) && one_in(600)) {
        hunger--;
    }

    if (pain > 0) {
        if (has_trait("PAINREC1") && one_in(600)) {
            pain--;
        }
        if (has_trait("PAINREC2") && one_in(300)) {
            pain--;
        }
        if (has_trait("PAINREC3") && one_in(150)) {
            pain--;
        }
    }

    if (has_trait("ALBINO") && g->is_in_sunlight(posx, posy) && one_in(10)) {
        // Umbrellas and rain gear can also keep the sun off!
        // (No, really, I know someone who uses an umbrella when it's sunny out.)
        if (!((worn_with_flag("RAINPROOF")) || (weapon.has_flag("RAIN_PROTECT"))) ) {
            add_msg(m_bad, _("The sunlight is really irritating."));
            if (has_disease("sleep")) {
                wake_up(_("You wake up!"));
            }
            if (one_in(10)) {
                mod_pain(1);
            }
            else focus_pool --;
        }
    }

    if (has_trait("SUNBURN") && g->is_in_sunlight(posx, posy) && one_in(10)) {
        if (!((worn_with_flag("RAINPROOF")) || (weapon.has_flag("RAIN_PROTECT"))) ) {
        add_msg(m_bad, _("The sunlight burns your skin!"));
        if (has_disease("sleep")) {
            wake_up(_("You wake up!"));
        }
        mod_pain(1);
        hurtall(1);
        }
    }

    if ((has_trait("TROGLO") || has_trait("TROGLO2")) &&
        g->is_in_sunlight(posx, posy) && g->weather == WEATHER_SUNNY) {
        mod_str_bonus(-1);
        mod_dex_bonus(-1);
        mod_int_bonus(-1);
        mod_per_bonus(-1);
    }
    if (has_trait("TROGLO2") && g->is_in_sunlight(posx, posy)) {
        mod_str_bonus(-1);
        mod_dex_bonus(-1);
        mod_int_bonus(-1);
        mod_per_bonus(-1);
    }
    if (has_trait("TROGLO3") && g->is_in_sunlight(posx, posy)) {
        mod_str_bonus(-4);
        mod_dex_bonus(-4);
        mod_int_bonus(-4);
        mod_per_bonus(-4);
    }

    if (has_trait("SORES")) {
        for (int i = bp_head; i < num_bp; i++) {
            if ((pain < 5 + 4 * abs(encumb(body_part(i)))) && (!(has_trait("NOPAIN")))) {
                pain = 0;
                mod_pain( 5 + 4 * abs(encumb(body_part(i))) );
            }
        }
    }

    if (has_trait("SLIMY") && !in_vehicle) {
        g->m.add_field(posx, posy, fd_slime, 1);
    }

    if (has_trait("VISCOUS") && !in_vehicle) {
        if (one_in(3)){
            g->m.add_field(posx, posy, fd_slime, 1);
        }
        else {
            g->m.add_field(posx, posy, fd_slime, 2);
        }
    }

    // Blind/Deaf for brief periods about once an hour,
    // and visuals about once every 30 min.
    if (has_trait("PER_SLIME")) {
        if (one_in(600) && !has_effect("deaf")) {
            add_msg(m_bad, _("Suddenly, you can't hear anything!"));
            add_effect("deaf", 20 * rng (2, 6)) ;
        }
        if (one_in(600) && !(has_effect("blind"))) {
            add_msg(m_bad, _("Suddenly, your eyes stop working!"));
            add_effect("blind", 10 * rng (2, 6)) ;
        }
        // Yes, you can be blind and hallucinate at the same time.
        // Your post-human biology is truly remarkable.
        if (one_in(300) && !(has_effect("visuals"))) {
            add_msg(m_bad, _("Your visual centers must be acting up..."));
            add_effect("visuals", 120 * rng (3, 6)) ;
        }
    }

    if (has_trait("WEB_SPINNER") && !in_vehicle && one_in(3)) {
        g->m.add_field(posx, posy, fd_web, 1); //this adds density to if its not already there.
    }

    if (has_trait("RADIOGENIC") && int(calendar::turn) % 50 == 0 && radiation >= 10) {
        radiation -= 10;
        healall(1);
    }

    if (has_trait("RADIOACTIVE1")) {
        if (g->m.get_radiation(posx, posy) < 10 && one_in(50)) {
            g->m.adjust_radiation(posx, posy, 1);
        }
    }
    if (has_trait("RADIOACTIVE2")) {
        if (g->m.get_radiation(posx, posy) < 20 && one_in(25)) {
            g->m.adjust_radiation(posx, posy, 1);
        }
    }
    if (has_trait("RADIOACTIVE3")) {
        if (g->m.get_radiation(posx, posy) < 30 && one_in(10)) {
            g->m.adjust_radiation(posx, posy, 1);
        }
    }

    if (has_trait("UNSTABLE") && one_in(28800)) { // Average once per 2 days
        mutate();
    }
    if (has_trait("CHAOTIC") && one_in(7200)) { // Should be once every 12 hours
        mutate();
    }
    if (has_artifact_with(AEP_MUTAGENIC) && one_in(28800)) {
        mutate();
    }
    if (has_artifact_with(AEP_FORCE_TELEPORT) && one_in(600)) {
        g->teleport(this);
    }

    // checking for radioactive items in inventory
    int selfRadiation = 0;
    selfRadiation = leak_level("RADIOACTIVE");

    int localRadiation = g->m.get_radiation(posx, posy);

    if (localRadiation || selfRadiation) {
        bool has_helmet = false;

        bool power_armored = is_wearing_power_armor(&has_helmet);

        if ((power_armored && has_helmet) || worn_with_flag("RAD_PROOF")) {
            radiation += 0; // Power armor protects completely from radiation
        } else if (power_armored || worn_with_flag("RAD_RESIST")) {
            radiation += rng(0, localRadiation / 40) + rng(0, selfRadiation / 5);
        } else {
            radiation += rng(0, localRadiation / 16) + rng(0, selfRadiation);;
        }

        // Apply rads to any radiation badges.
        std::vector<item *> possessions = inv_dump();
        for( std::vector<item *>::iterator it = possessions.begin(); it != possessions.end(); ++it ) {
            if( (*it)->type->id == "rad_badge" ) {
                // Actual irradiation levels of badges and the player aren't precisely matched.
                // This is intentional.
                int before = (*it)->irridation;
                (*it)->irridation += rng(0, localRadiation / 16);
                if( inv.has_item(*it) ) { continue; }
                for( int i = 0; i < sizeof(rad_dosage_thresholds) / sizeof(rad_dosage_thresholds[0]);
                     i++ ){
                    if( before < rad_dosage_thresholds[i] &&
                        (*it)->irridation >= rad_dosage_thresholds[i] ) {
                        add_msg_if_player(m_warning, _("Your radiation badge changes from %s to %s!"),
                                                     rad_threshold_colors[i - 1].c_str(),
                                                     rad_threshold_colors[i].c_str() );
                    }
                }
            }
        }
    }

    if( int(calendar::turn) % 150 == 0 ) {
        if (radiation < 0) {
            radiation = 0;
        } else if (radiation > 2000) {
            radiation = 2000;
        }
        if (OPTIONS["RAD_MUTATION"] && rng(60, 2500) < radiation) {
            mutate();
            radiation /= 2;
            radiation -= 5;
        } else if (radiation > 100 && rng(1, 1500) < radiation) {
            vomit();
            radiation -= 50;
        }
    }

    if( radiation > 150 && !(int(calendar::turn) % 90) ) {
        hurtall(radiation / 100);
    }

    // Negative bionics effects
    if (has_bionic("bio_dis_shock") && one_in(1200)) {
        add_msg(m_bad, _("You suffer a painful electrical discharge!"));
        mod_pain(1);
        moves -= 150;
    }
    if (has_bionic("bio_dis_acid") && one_in(1500)) {
        add_msg(m_bad, _("You suffer a burning acidic discharge!"));
        hurtall(1);
    }
    if (has_bionic("bio_drain") && power_level > 0 && one_in(600)) {
        add_msg(m_bad, _("Your batteries discharge slightly."));
        power_level--;
    }
    if (has_bionic("bio_noise") && one_in(500)) {
        if(!is_deaf())
            add_msg(m_bad, _("A bionic emits a crackle of noise!"));
        else
            add_msg(m_bad, _("A bionic shudders, but you hear nothing."));
        g->sound(posx, posy, 60, "");
    }
    if (has_bionic("bio_power_weakness") && max_power_level > 0 &&
        power_level >= max_power_level * .75) {
        mod_str_bonus(-3);
    }
    if (has_bionic("bio_trip") && one_in(500) && !has_disease("visuals")) {
        add_msg(m_bad, _("Your vision pixelates!"));
        add_disease("visuals", 100);
    }
    if (has_bionic("bio_spasm") && one_in(3000) && !has_disease("downed")) {
        add_msg(m_bad, _("Your malfunctioning bionic causes you to spasm and fall to the floor!"));
        mod_pain(1);
        add_effect("stunned", 1);
        add_effect("downed", 1);
    }
    if (has_bionic("bio_shakes") && power_level > 0 && one_in(1200)) {
        add_msg(m_bad, _("Your bionics short-circuit, causing you to tremble and shiver."));
        power_level--;
        add_disease("shakes", 50);
    }
    if (has_bionic("bio_leaky") && one_in(500)) {
        health--;
    }
    if (has_bionic("bio_sleepy") && one_in(500)) {
        fatigue++;
    }
    if (has_bionic("bio_itchy") && one_in(500) && !has_disease("formication")) {
        add_msg(m_bad, _("Your malfunctioning bionic itches!"));
      body_part bp = random_body_part(true);
      int side = random_side(bp);
        add_disease("formication", 100, false, 1, 3, 0, 1, bp, side, true);
    }

    // Artifact effects
    if (has_artifact_with(AEP_ATTENTION)) {
        add_disease("attention", 3);
    }

    // check for limb mending every 1000 turns (~1.6 hours)
    if(calendar::turn.get_turn() % 1000 == 0) {
        mend();
    }
}

void player::mend()
{
 // Wearing splints can slowly mend a broken limb back to 1 hp.
 // 2 weeks is faster than a fracture would heal IRL,
 // but 3 weeks average (a generous estimate) was tedious and no fun.
 for(int i = 0; i < num_hp_parts; i++) {
  int broken = (hp_cur[i] <= 0);
  if(broken) {
   double mending_odds = 200.0; // 2 weeks, on average. (~20160 minutes / 100 minutes)
   double healing_factor = 1.0;
   // Studies have shown that alcohol and tobacco use delay fracture healing time
   if(has_disease("cig") | addiction_level(ADD_CIG)) {
    healing_factor *= 0.5;
   }
   if(has_disease("drunk") | addiction_level(ADD_ALCOHOL)) {
    healing_factor *= 0.5;
   }

   // Bed rest speeds up mending
   if(has_disease("sleep")) {
    healing_factor *= 4.0;
   } else if(fatigue > 383) {
    // but being dead tired does not...
    healing_factor *= 0.75;
   }

   // Being healthy helps.
   if(health > 0) {
    healing_factor *= 2.0;
   }

   // And being well fed...
   if(hunger < 0) {
    healing_factor *= 2.0;
   }

   if(thirst < 0) {
    healing_factor *= 2.0;
   }

   // Mutagenic healing factor!
   if(has_trait("REGEN")) {
    healing_factor *= 16.0;
   } else if (has_trait("FASTHEALER2")) {
    healing_factor *= 4.0;
   } else if (has_trait("FASTHEALER")) {
    healing_factor *= 2.0;
   } else if (has_trait("SLOWHEALER")) {
    healing_factor *= 0.5;
   }

   bool mended = false;
   int side = 0;
   body_part part;
   switch(i) {
    case hp_arm_r:
     side = 1;
     // fall-through
    case hp_arm_l:
     part = bp_arms;
     mended = is_wearing("arm_splint") && x_in_y(healing_factor, mending_odds);
     break;
    case hp_leg_r:
     side = 1;
     // fall-through
    case hp_leg_l:
     part = bp_legs;
     mended = is_wearing("leg_splint") && x_in_y(healing_factor, mending_odds);
     break;
    default:
     // No mending for you!
     break;
   }
   if(mended) {
    hp_cur[i] = 1;
    //~ %s is bodypart
    add_memorial_log(pgettext("memorial_male", "Broken %s began to mend."),
                     pgettext("memorial_female", "Broken %s began to mend."),
                     body_part_name(part, side).c_str());
    add_msg(m_good, _("Your %s has started to mend!"),
      body_part_name(part, side).c_str());
   }
  }
 }
}

void player::vomit()
{
    add_memorial_log(pgettext("memorial_male", "Threw up."),
                     pgettext("memorial_female", "Threw up."));
    add_msg(m_bad, _("You throw up heavily!"));
    int nut_loss = 100 / (1 + exp(.15 * (hunger / 100)));
    int quench_loss = 100 / (1 + exp(.025 * (thirst / 10)));
    hunger += rng(nut_loss / 2, nut_loss);
    thirst += rng(quench_loss / 2, quench_loss);
    moves -= 100;
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == "foodpoison") {
            illness[i].duration -= 300;
            if (illness[i].duration < 0) {
                rem_disease(illness[i].type);
            }
        } else if (illness[i].type == "drunk") {
            illness[i].duration -= rng(1, 5) * 100;
            if (illness[i].duration < 0) {
                rem_disease(illness[i].type);
            }
        }
    }
    rem_disease("pkill1");
    rem_disease("pkill2");
    rem_disease("pkill3");
    rem_disease("sleep");
}

void player::drench(int saturation, int flags)
{
    // OK, water gets in your AEP suit or whatever.  It wasn't built to keep you dry.
    if ( (is_waterproof(flags)) && (!(g->m.has_flag(TFLAG_DEEP_WATER, posx, posy))) ) {
        return;
    }

    int effected = 0;
    int tot_ignored = 0; //Always ignored
    int tot_neut = 0; //Ignored for good wet bonus
    int tot_good = 0; //Increase good wet bonus

    for (std::map<int, int>::iterator iter = mDrenchEffect.begin(); iter != mDrenchEffect.end(); ++iter) {
        if (mfb(iter->first) & flags) {
            effected += iter->second;
            tot_ignored += mMutDrench[iter->first]["ignored"];
            tot_neut += mMutDrench[iter->first]["neutral"];
            tot_good += mMutDrench[iter->first]["good"];
        }
    }

    if (effected == 0) {
        return;
    }

    bool wants_drench = false;
    // If not protected by mutations then check clothing
    if (tot_good + tot_neut + tot_ignored < effected) {
        wants_drench = is_water_friendly(flags);
    } else {
        wants_drench = true;
    }

    int morale_cap;
    if (wants_drench) {
        morale_cap = g->get_temperature() - std::min(65, 65 + (tot_ignored - tot_good) / 2) * saturation / 100;
    } else {
        morale_cap = -(saturation / 2);
    }

    // Good increases pos and cancels neg, neut cancels neg, ignored cancels both
    if (morale_cap > 0) {
        morale_cap = morale_cap * (effected - tot_ignored + tot_good) / effected;
    } else if (morale_cap < 0) {
        morale_cap = morale_cap * (effected - tot_ignored - tot_neut - tot_good) / effected;
    }

    if (morale_cap == 0) {
        return;
    }

    int morale_effect = morale_cap / 8;
    if (morale_effect == 0) {
        if (morale_cap > 0) {
            morale_effect = 1;
        } else {
            morale_effect = -1;
        }
    }

    int dur = 60;
    int d_start = 30;
    if (morale_cap < 0) {
        if (has_trait("LIGHTFUR") || has_trait("FUR") || has_trait("FELINE_FUR") || has_trait("LUPINE_FUR")) {
            dur /= 5;
            d_start /= 5;
        }
        // Shaggy fur holds water longer.  :-/
        if (has_trait("URSINE_FUR")) {
            dur /= 3;
            d_start /= 3;
        }
    } else {
        if (has_trait("SLIMY")) {
            dur *= 1.2;
            d_start *= 1.2;
        }
    }

    add_morale(MORALE_WET, morale_effect, morale_cap, dur, d_start);
}

void player::drench_mut_calc()
{
    mMutDrench.clear();
    int ignored, neutral, good;

    for (std::map<int, int>::iterator it = mDrenchEffect.begin(); it != mDrenchEffect.end(); ++it) {
        ignored = 0;
        neutral = 0;
        good = 0;

        for( auto iter = my_mutations.begin(); iter != my_mutations.end(); ++iter ) {
            for( auto wp_iter = mutation_data[*iter].protection.begin();
                 wp_iter != mutation_data[*iter].protection.end(); ++wp_iter) {
                if (body_parts[wp_iter->first] == it->first) {
                    ignored += wp_iter->second.second.x;
                    neutral += wp_iter->second.second.y;
                    good += wp_iter->second.second.z;
                }
            }
        }

        mMutDrench[it->first]["good"] = good;
        mMutDrench[it->first]["neutral"] = neutral;
        mMutDrench[it->first]["ignored"] = ignored;
    }
}

int player::weight_carried()
{
    int ret = 0;
    ret += weapon.weight();
    for (int i = 0; i < worn.size(); i++) {
        ret += worn[i].weight();
    }
    ret += inv.weight();
    return ret;
}

int player::volume_carried()
{
    return inv.volume();
}

int player::weight_capacity(bool /* return_stat_effect */)
{
    // return_stat_effect is effectively pointless
    // player info window shows current stat effects
    // current str is used anyway (probably) always.
    // int str = return_stat_effect ? get_str() : get_str();
    int str = get_str();
    int ret = 13000 + str * 4000;
    if (has_trait("BADBACK")) {
        ret = int(ret * .65);
    }
    if (has_trait("STRONGBACK")) {
        ret = int(ret * 1.35);
    }
    if (has_trait("LIGHT_BONES")) {
        ret = int(ret * .80);
    }
    if (has_trait("HOLLOW_BONES")) {
        ret = int(ret * .60);
    }
    if (has_artifact_with(AEP_CARRY_MORE)) {
        ret += 22500;
    }
    if (ret < 0) {
        ret = 0;
    }
    return ret;
}

int player::volume_capacity()
{
    int ret = 2; // A small bonus (the overflow)
    it_armor *armor = NULL;
    for (int i = 0; i < worn.size(); i++) {
        armor = dynamic_cast<it_armor*>(worn[i].type);
        ret += armor->storage;
    }
    if (has_bionic("bio_storage")) {
        ret += 8;
    }
    if (has_trait("SHELL")) {
        ret += 16;
    }
    if (has_trait("PACKMULE")) {
        ret = int(ret * 1.4);
    }
    if (has_trait("DISORGANIZED")) {
        ret = int(ret * 0.6);
    }
    if (ret < 2) {
        ret = 2;
    }
    return ret;
}

double player::convert_weight(int weight)
{
    double ret;
    ret = double(weight);
    if (OPTIONS["USE_METRIC_WEIGHTS"] == "kg") {
        ret /= 1000;
    } else {
        ret /= 453.6;
    }
    return ret;
}

bool player::can_pickVolume(int volume)
{
    return (volume_carried() + volume <= volume_capacity());
}
bool player::can_pickWeight(int weight, bool safe)
{
    if (!safe)
    {
        //Player can carry up to four times their maximum weight
        return (weight_carried() + weight <= weight_capacity() * 4);
    }
    else
    {
        return (weight_carried() + weight <= weight_capacity());
    }
}

int player::net_morale(morale_point effect)
{
    double bonus = effect.bonus;

    // If the effect is old enough to have started decaying,
    // reduce it appropriately.
    if (effect.age > effect.decay_start)
    {
        bonus *= logistic_range(effect.decay_start,
                                effect.duration, effect.age);
    }

    // Optimistic characters focus on the good things in life,
    // and downplay the bad things.
    if (has_trait("OPTIMISTIC"))
    {
        if (bonus >= 0)
        {
            bonus *= 1.25;
        }
        else
        {
            bonus *= 0.75;
        }
    }

     // Again, those grouchy Bad-Tempered folks always focus on the negative.
     // They can't handle positive things as well.  They're No Fun.  D:
    if (has_trait("BADTEMPER"))
    {
        if (bonus < 0)
        {
            bonus *= 1.25;
        }
        else
        {
            bonus *= 0.75;
        }
    }
    return bonus;
}

int player::morale_level()
{
    // Add up all of the morale bonuses (and penalties).
    int ret = 0;
    for (int i = 0; i < morale.size(); i++)
    {
        ret += net_morale(morale[i]);
    }

    // Prozac reduces negative morale by 75%.
    if (has_disease("took_prozac") && ret < 0)
    {
        ret = int(ret / 4);
    }

    return ret;
}

void player::add_morale(morale_type type, int bonus, int max_bonus,
                        int duration, int decay_start,
                        bool cap_existing, itype* item_type)
{
    bool placed = false;

    // Search for a matching morale entry.
    for (int i = 0; i < morale.size() && !placed; i++)
    {
        if (morale[i].type == type && morale[i].item_type == item_type)
        {
            // Found a match!
            placed = true;

            // Scale the morale bonus to its current level.
            if (morale[i].age > morale[i].decay_start)
            {
                morale[i].bonus *= logistic_range(morale[i].decay_start,
                                                  morale[i].duration, morale[i].age);
            }

            // If we're capping the existing effect, we can use the new duration
            // and decay start.
            if (cap_existing)
            {
                morale[i].duration = duration;
                morale[i].decay_start = decay_start;
            }
            else
            {
                // Otherwise, we need to figure out whether the existing effect had
                // more remaining duration and decay-resistance than the new one does.
                // Only takes the new duration if new bonus and old are the same sign.
                if (morale[i].duration - morale[i].age <= duration &&
                   ((morale[i].bonus > 0) == (max_bonus > 0)) )
                {
                    morale[i].duration = duration;
                }
                else
                {
                    // This will give a longer duration than above.
                    morale[i].duration -= morale[i].age;
                }

                if (morale[i].decay_start - morale[i].age <= decay_start &&
                   ((morale[i].bonus > 0) == (max_bonus > 0)) )
                {
                    morale[i].decay_start = decay_start;
                }
                else
                {
                    // This will give a later decay start than above.
                    morale[i].decay_start -= morale[i].age;
                }
            }

            // Now that we've finished using it, reset the age to 0.
            morale[i].age = 0;

            // Is the current morale level for this entry below its cap, if any?
            if (abs(morale[i].bonus) < abs(max_bonus) || max_bonus == 0)
            {
                // Add the requested morale boost.
                morale[i].bonus += bonus;

                // If we passed the cap, pull back to it.
                if (abs(morale[i].bonus) > abs(max_bonus) && max_bonus != 0)
                {
                    morale[i].bonus = max_bonus;
                }
            }
            else if (cap_existing)
            {
                // The existing bonus is above the new cap.  Reduce it.
                morale[i].bonus = max_bonus;
            }
        }
    }

    // No matching entry, so add a new one
    if (!placed)
    {
        morale_point tmp(type, item_type, bonus, duration, decay_start, 0);
        morale.push_back(tmp);
    }
}

int player::has_morale( morale_type type ) const
{
    for( size_t i = 0; i < morale.size(); ++i ) {
        if( morale[i].type == type ) {
            return morale[i].bonus;
        }
    }
    return 0;
}

void player::rem_morale(morale_type type, itype* item_type)
{
 for (int i = 0; i < morale.size(); i++) {
  if (morale[i].type == type && morale[i].item_type == item_type) {
    morale.erase(morale.begin() + i);
    break;
  }
 }
}

item& player::i_add(item it)
{
 itype_id item_type_id = "null";
 if( it.type ) item_type_id = it.type->id;

 last_item = item_type_id;

 if (it.is_food() || it.is_ammo() || it.is_gun()  || it.is_armor() ||
     it.is_book() || it.is_tool() || it.is_weap() || it.is_food_container())
  inv.unsort();

 if (g != NULL && it.is_artifact() && it.is_tool()) {
  it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(it.type);
  g->add_artifact_messages(art->effects_carried);
 }
 return inv.add_item(it);
}

bool player::has_active_item(const itype_id & id) const
{
    if (weapon.type->id == id && weapon.active)
    {
        return true;
    }
    return inv.has_active_item(id);
}

long player::active_item_charges(itype_id id)
{
    long max = 0;
    if (weapon.type->id == id && weapon.active)
    {
        max = weapon.charges;
    }

    long inv_max = inv.max_active_item_charges(id);
    if (inv_max > max)
    {
        max = inv_max;
    }
    return max;
}

void player::process_active_items()
{
    if (weapon.is_artifact() && weapon.is_tool()) {
        g->process_artifact(&weapon, this, true);
    } else if (weapon.active) {
        if (weapon.has_flag("CHARGE")) {
            if (weapon.charges == 8) { // Maintaining charge takes less power.
                if( use_charges_if_avail("adv_UPS_on", 2) || use_charges_if_avail("UPS_on", 4) ) {
                    weapon.poison++;
                } else {
                    weapon.poison--;
                }
                if ( (weapon.poison >= 3) && (one_in(20)) ) { // 3 turns leeway, then it may discharge.
                    //~ %s is weapon name
                    add_memorial_log(pgettext("memorial_male", "Accidental discharge of %s."),
                                     pgettext("memorial_female", "Accidental discharge of %s."),
                                     weapon.tname().c_str());
                    add_msg(m_bad, _("Your %s discharges!"), weapon.tname().c_str());
                    point target(posx + rng(-12, 12), posy + rng(-12, 12));
                    std::vector<point> traj = line_to(posx, posy, target.x, target.y, 0);
                    g->fire(*this, target.x, target.y, traj, false);
                } else {
                    add_msg(m_warning, _("Your %s beeps alarmingly."), weapon.tname().c_str());
                }
            } else { // We're chargin it up!
                if ( use_charges_if_avail("adv_UPS_on", ceil(static_cast<float>(1 + weapon.charges) / 2)) ||
                     use_charges_if_avail("UPS_on", 1 + weapon.charges) ) {
                    weapon.poison++;
                } else {
                    weapon.poison--;
                }

                if (weapon.poison >= weapon.charges) {
                    weapon.charges++;
                    weapon.poison = 0;
                }
            }
            if (weapon.poison < 0) {
                add_msg(_("Your %s spins down."), weapon.tname().c_str());
                weapon.charges--;
                weapon.poison = weapon.charges - 1;
            }
            if (weapon.charges <= 0) {
                weapon.active = false;
            }
        }
        else if (!process_single_active_item(&weapon)) {
            weapon = ret_null;
        }
    }

    std::vector<item *> inv_active = inv.active_items();
    for (std::vector<item *>::iterator iter = inv_active.begin(); iter != inv_active.end(); ++iter) {
        item *tmp_it = *iter;
        if (tmp_it->is_artifact() && tmp_it->is_tool()) {
            g->process_artifact(tmp_it, this);
        }
        if (!process_single_active_item(tmp_it)) {
            inv.remove_item(tmp_it);
        }
    }

    // worn items
    for (int i = 0; i < worn.size(); i++) {
        if (worn[i].is_artifact()) {
            g->process_artifact(&(worn[i]), this);
        }
        if (!process_single_active_item(&worn[i])) {
            worn.erase(worn.begin() + i);
            i--;
        }
    }

  // Drain UPS if using optical cloak.
  // TODO: Move somewhere else.
  if ((has_active_item("UPS_on") || has_active_item("adv_UPS_on"))
      && is_wearing("optical_cloak")) {
    // Drain UPS.
    if (has_charges("adv_UPS_on", 24)) {
      use_charges("adv_UPS_on", 24);
      if (charges_of("adv_UPS_on") < 120 && one_in(3))
        add_msg_if_player( m_warning, _("Your optical cloak flickers for a moment!"));
    } else if (has_charges("UPS_on", 40)) {
      use_charges("UPS_on", 40);
      if (charges_of("UPS_on") < 200 && one_in(3))
        add_msg_if_player( m_warning, _("Your optical cloak flickers for a moment!"));
    } else {
      if (has_charges("adv_UPS_on", charges_of("adv_UPS_on"))) {
          // Drain last power.
          use_charges("adv_UPS_on", charges_of("adv_UPS_on"));
      }
      else {
        use_charges("UPS_on", charges_of("UPS_on"));
      }
    }
  }
}

// returns false if the item needs to be removed
bool player::process_single_active_item(item *it)
{
    if (it->active ||
        (it->is_container() && !it->contents.empty() && it->contents[0].active))
    {
        if (it->is_food())
        {
            it->calc_rot(this->pos());
            if (it->has_flag("HOT"))
            {
                it->item_counter--;
                if (it->item_counter == 0)
                {
                    it->item_tags.erase("HOT");
                }
            }
        }
        else if (it->is_food_container())
        {
            it->contents[0].calc_rot(this->pos());
            if (it->contents[0].has_flag("HOT"))
            {
                it->contents[0].item_counter--;
                if (it->contents[0].item_counter == 0)
                {
                    it->contents[0].item_tags.erase("HOT");
                }
            }
        }
        else if( it->has_flag("WET") )
        {
            it->item_counter--;
            if(it->item_counter == 0)
            {
                const it_tool *tool = dynamic_cast<const it_tool*>(it->type);
                if (tool != NULL && tool->revert_to != "null") {
                    it->make(tool->revert_to);
                }
                it->item_tags.erase("WET");
                if (!it->has_flag("ABSORBENT")) {
                    it->item_tags.insert("ABSORBENT");
                }
                it->active = false;
            }
        }
        else if( it->has_flag("LITCIG") )
        {
            it->item_counter--;
            if(it->item_counter % 2 == 0) { // only puff every other turn
                int duration = 10;
                if (this->has_trait("TOLERANCE")) {
                    duration = 5;
                }
                else if (this->has_trait("LIGHTWEIGHT")) {
                    duration = 20;
                }
                add_msg_if_player( _("You take a puff of your %s."), it->tname().c_str());
                if(it->has_flag("TOBACCO")) {
                    this->add_disease("cig", duration);
                    g->m.add_field(this->posx + int(rng(-1, 1)), this->posy + int(rng(-1, 1)),
                                   fd_cigsmoke, 2);
                } else { // weedsmoke
                    this->add_disease("weed_high", duration / 2);
                    g->m.add_field(this->posx + int(rng(-1, 1)), this->posy + int(rng(-1, 1)),
                                   fd_weedsmoke, 2);
                }
                this->moves -= 15;
            }

            if((this->has_disease("shakes") && one_in(10)) ||
                (this->has_trait("JITTERY") && one_in(200))) {
                add_msg_if_player( m_bad,_("Your shaking hand causes you to drop your %s."),
                                   it->tname().c_str());
                g->m.add_item_or_charges(this->posx + int(rng(-1, 1)),
                                         this->posy + int(rng(-1, 1)), this->i_rem(it), 2);
            }

            // done with cig
            if(it->item_counter <= 0) {
                add_msg_if_player( _("You finish your %s."), it->tname().c_str());
                if(it->type->id == "cig_lit") {
                    it->make("cig_butt");
                } else if(it->type->id == "cigar_lit"){
                    it->make("cigar_butt");
                } else { // joint
                    this->add_disease("weed_high", 10); // one last puff
                    g->m.add_field(this->posx + int(rng(-1, 1)), this->posy + int(rng(-1, 1)),
                                   fd_weedsmoke, 2);
                    weed_msg(this);
                    it->make("joint_roach");
                }
                it->active = false;
            }
        } else if (it->is_tool()) {
            it_tool* tmp = dynamic_cast<it_tool*>(it->type);
            tmp->invoke(this, it, true);
            if (tmp->turns_per_charge > 0 && int(calendar::turn) % tmp->turns_per_charge == 0) {
                it->charges--;
            }
            if (it->charges <= 0 && !(it->has_flag("USE_UPS") &&
                                      (has_charges("adv_UPS_on", 1) || has_charges("UPS_on", 1) ||
                                       has_active_bionic("bio_ups")))) {
                if (it->has_flag("USE_UPS")){
                    add_msg_if_player(_("You need an active UPS to run that!"));
                    tmp->invoke(this, it, false);
                } else {
                    tmp->invoke(this, it, false);
                    if (tmp->revert_to == "null") {
                        return false;
                    } else {
                        it->type = itypes[tmp->revert_to];
                        it->active = false;
                    }
                }
            }
        } else if (it->type->id == "corpse") {
            if (it->ready_to_revive() && g->revive_corpse(posx, posy, it)) {
                //~ %s is corpse name
                add_memorial_log(pgettext("memorial_male", "Had a %s revive while carrying it."),
                                 pgettext("memorial_female", "Had a %s revive while carrying it."),
                                 it->tname().c_str());
                add_msg_if_player( m_warning, _("Oh dear god, a corpse you're carrying has started moving!"));
                return false;
            }
        } else {
            debugmsg("%s is active, but has no known active function.", it->tname().c_str());
        }
    }
    return true;
}

item player::remove_weapon()
{
 if (weapon.has_flag("CHARGE") && weapon.active) { //unwield a charged charge rifle.
  weapon.charges = 0;
  weapon.active = false;
 }
 item tmp = weapon;
 weapon = ret_null;
// We need to remove any boosts related to our style
 rem_disease("attack_boost");
 rem_disease("dodge_boost");
 rem_disease("damage_boost");
 rem_disease("speed_boost");
 rem_disease("armor_boost");
 rem_disease("viper_combo");
 return tmp;
}

void player::remove_mission_items(int mission_id)
{
 if (mission_id == -1)
  return;
 if (weapon.mission_id == mission_id) {
  remove_weapon();
 } else {
  for (int i = 0; i < weapon.contents.size(); i++) {
   if (weapon.contents[i].mission_id == mission_id)
    remove_weapon();
  }
 }
 inv.remove_mission_items(mission_id);
}

item player::reduce_charges(int position, long quantity) {
    if (position == -1) {
        if (!weapon.count_by_charges())
        {
            debugmsg("Tried to remove %s by charges, but item is not counted by charges",
                    weapon.type->nname(1).c_str());
        }

        if (quantity > weapon.charges)
        {
            debugmsg("Charges: Tried to remove charges that does not exist, \
                      removing maximum available charges instead");
            quantity = weapon.charges;
        }
        weapon.charges -= quantity;
        if (weapon.charges <= 0)
        {
            return remove_weapon();
        }
        return weapon;
    } else if (position < -1) {
        debugmsg("Wearing charged items is not implemented.");
        return ret_null;
    } else {
        return inv.reduce_charges(position, quantity);
    }
}


item player::i_rem(char let)
{
 item tmp;
 if (weapon.invlet == let) {
  tmp = weapon;
  weapon = ret_null;
  return tmp;
 }
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let) {
   tmp = worn[i];
   worn.erase(worn.begin() + i);
   return tmp;
  }
 }
 if (!inv.item_by_letter(let).is_null())
  return inv.remove_item(let);
 return ret_null;
}

item player::i_rem(int pos)
{
 item tmp;
 if (pos == -1) {
     tmp = weapon;
     weapon = ret_null;
     return tmp;
 } else if (pos < -1 && pos > worn_position_to_index(worn.size())) {
     tmp = worn[worn_position_to_index(pos)];
     worn.erase(worn.begin() + worn_position_to_index(pos));
     return tmp;
 }
 return inv.remove_item(pos);
}

item player::i_rem(itype_id type)
{
    if (weapon.type->id == type)
    {
        return remove_weapon();
    }
    return inv.remove_item(type);
}

item player::i_rem(item *it)
{
    if (&weapon == it)
    {
        return remove_weapon();
    }
    return inv.remove_item(it);
}

// Negative positions indicate weapon/clothing, 0 & positive indicate inventory
item& player::i_at(int position)
{
 if (position == -1)
     return weapon;
 if (position < -1) {
     int worn_index = worn_position_to_index(position);
     if (worn_index < worn.size()) {
         return worn[worn_index];
     }
 }
 return inv.find_item(position);
}

item& player::i_at(char let)
{
 if (let == KEY_ESCAPE)
  return ret_null;
 if (weapon.invlet == let)
  return weapon;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let)
   return worn[i];
 }
 return inv.item_by_letter(let);
}

item& player::i_of_type(itype_id type)
{
 if (weapon.type->id == type)
  return weapon;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == type)
   return worn[i];
 }
 return inv.item_by_type(type);
}

char player::position_to_invlet(int position) {
    return i_at(position).invlet;
}

int player::invlet_to_position(char invlet) {
    if (weapon.invlet == invlet)
     return -1;
    for (int i = 0; i < worn.size(); i++) {
     if (worn[i].invlet == invlet)
      return worn_position_to_index(i);
    }
    return inv.position_by_letter(invlet);
}

int player::get_item_position(item* it) {
    if (&weapon == it) {
        return -1;
    }
    for (int i = 0; i < worn.size(); i++) {
     if (&worn[i] == it)
      return worn_position_to_index(i);
    }
    return inv.position_by_item(it);
}


const martialart &player::get_combat_style() const
{
    auto it = martialarts.find( style_selected );
    if( it != martialarts.end() ) {
        return it->second;
    }
    debugmsg( "unknown martial art style %s selected", style_selected.c_str() );
    return martialarts["style_none"];
}

std::vector<item *> player::inv_dump()
{
 std::vector<item *> ret;
    if (!weapon.is_null() && !weapon.has_flag("NO_UNWIELD")) {
        ret.push_back(&weapon);
    }
 for (int i = 0; i < worn.size(); i++)
  ret.push_back(&worn[i]);
 inv.dump(ret);
 return ret;
}

item player::i_remn(char invlet)
{
 return inv.remove_item(invlet);
}

std::list<item> player::use_amount(itype_id it, int quantity, bool use_container)
{
    std::list<item> ret;
    if (weapon.use_amount(it, quantity, use_container, ret)) {
        remove_weapon();
    }
    for (std::vector<item>::iterator a = worn.begin(); a != worn.end() && quantity > 0; ++a) {
        for (std::vector<item>::iterator b = a->contents.begin(); b != a->contents.end() && quantity > 0; ) {
            if (b->use_amount(it, quantity, use_container, ret)) {
                b = a->contents.erase(b);
            } else {
                ++b;
            }
        }
    }
    if (quantity <= 0) {
        return ret;
    }
    std::list<item> tmp = inv.use_amount(it, quantity, use_container);
    ret.splice(ret.end(), tmp);
    return ret;
}

bool player::use_charges_if_avail(itype_id it, long quantity)
{
    if (has_charges(it, quantity))
    {
        use_charges(it, quantity);
        return true;
    }
    return false;
}

bool player::has_fire(const int quantity)
{
// TODO: Replace this with a "tool produces fire" flag.

    if (g->m.has_nearby_fire(posx, posy)) {
        return true;
    } else if (has_charges("torch_lit", 1)) {
        return true;
    } else if (has_charges("battletorch_lit", quantity)) {
        return true;
    } else if (has_charges("handflare_lit", 1)) {
        return true;
    } else if (has_charges("candle_lit", 1)) {
        return true;
    } else if (has_bionic("bio_tools")) {
        return true;
    } else if (has_bionic("bio_lighter")) {
        return true;
    } else if (has_bionic("bio_laser")) {
        return true;
    } else if (has_charges("ref_lighter", quantity)) {
        return true;
    } else if (has_charges("matches", quantity)) {
        return true;
    } else if (has_charges("lighter", quantity)) {
        return true;
    } else if (has_charges("flamethrower", quantity)) {
        return true;
    } else if (has_charges("flamethrower_simple", quantity)) {
        return true;
    } else if (has_charges("hotplate", quantity)) {
        return true;
    } else if (has_charges("welder", quantity)) {
        return true;
    } else if (has_charges("welder_crude", quantity)) {
        return true;
    } else if (has_charges("shishkebab_on", quantity)) {
        return true;
    } else if (has_charges("firemachete_on", quantity)) {
        return true;
    } else if (has_charges("broadfire_on", quantity)) {
        return true;
    } else if (has_charges("firekatana_on", quantity)) {
        return true;
    } else if (has_charges("zweifire_on", quantity)) {
        return true;
    }
    return false;
}

void player::use_fire(const int quantity)
{
//Ok, so checks for nearby fires first,
//then held lit torch or candle, bio tool/lighter/laser
//tries to use 1 charge of lighters, matches, flame throwers
// (home made, military), hotplate, welder in that order.
// bio_lighter, bio_laser, bio_tools, has_bionic("bio_tools"

    if (g->m.has_nearby_fire(posx, posy)) {
        return;
    } else if (has_charges("torch_lit", 1)) {
        return;
    } else if (has_charges("battletorch_lit", 1)) {
        return;
    } else if (has_charges("handflare_lit", 1)) {
        return;
    } else if (has_charges("candle_lit", 1)) {
        return;
    } else if (has_charges("shishkebab_on", quantity)) {
        return;
    } else if (has_charges("firemachete_on", quantity)) {
        return;
    } else if (has_charges("broadfire_on", quantity)) {
        return;
    } else if (has_charges("firekatana_on", quantity)) {
        return;
    } else if (has_charges("zweifire_on", quantity)) {
        return;
    } else if (has_bionic("bio_tools")) {
        return;
    } else if (has_bionic("bio_lighter")) {
        return;
    } else if (has_bionic("bio_laser")) {
        return;
    } else if (has_charges("ref_lighter", quantity)) {
        use_charges("ref_lighter", quantity);
        return;
    } else if (has_charges("matches", quantity)) {
        use_charges("matches", quantity);
        return;
    } else if (has_charges("lighter", quantity)) {
        use_charges("lighter", quantity);
        return;
    } else if (has_charges("flamethrower", quantity)) {
        use_charges("flamethrower", quantity);
        return;
    } else if (has_charges("flamethrower_simple", quantity)) {
        use_charges("flamethrower_simple", quantity);
        return;
    } else if (has_charges("hotplate", quantity)) {
        use_charges("hotplate", quantity);
        return;
    } else if (has_charges("welder", quantity)) {
        use_charges("welder", quantity);
        return;
    } else if (has_charges("welder_crude", quantity)) {
        use_charges("welder_crude", quantity);
        return;
    } else if (has_charges("shishkebab_off", quantity)) {
        use_charges("shishkebab_off", quantity);
        return;
    } else if (has_charges("firemachete_off", quantity)) {
        use_charges("firemachete_off", quantity);
        return;
    } else if (has_charges("broadfire_off", quantity)) {
        use_charges("broadfire_off", quantity);
        return;
    } else if (has_charges("firekatana_off", quantity)) {
        use_charges("firekatana_off", quantity);
        return;
    } else if (has_charges("zweifire_off", quantity)) {
        use_charges("zweifire_off", quantity);
        return;
    }
}

// does NOT return anything if the item is integrated toolset or fire!
std::list<item> player::use_charges(itype_id it, long quantity)
{
    std::list<item> ret;
    // the first two cases *probably* don't need to be tracked for now...
    if (it == "toolset") {
        power_level -= quantity;
        if (power_level < 0) {
            power_level = 0;
        }
        return ret;
    }
    if (it == "fire") {
        use_fire(quantity);
        return ret;
    }
    if (weapon.use_charges(it, quantity, ret)) {
        remove_weapon();
    }
    for (std::vector<item>::iterator a = worn.begin(); a != worn.end() && quantity > 0; ++a) {
        for (std::vector<item>::iterator b = a->contents.begin(); b != a->contents.end() && quantity > 0; ) {
            if (b->use_charges(it, quantity, ret)) {
                b = a->contents.erase(b);
            } else {
                ++b;
            }
        }
    }
    if (quantity <= 0) {
        return ret;
    }
    std::list<item> tmp = inv.use_charges(it, quantity);
    ret.splice(ret.end(), tmp);
    return ret;
}

int player::butcher_factor() const
{
    int lowest_factor = INT_MAX;
    if (has_bionic("bio_tools")) {
        lowest_factor = 100;
    }
    int inv_factor = inv.butcher_factor();
    if (inv_factor < lowest_factor) {
        lowest_factor = inv_factor;
    }
    lowest_factor = std::min(lowest_factor, weapon.butcher_factor());
    for (std::vector<item>::const_iterator a = worn.begin(); a != worn.end(); ++a) {
        lowest_factor = std::min(lowest_factor, a->butcher_factor());
    }
    return lowest_factor;
}

item* player::pick_usb()
{
 std::vector<std::pair<item*, int> > drives = inv.all_items_by_type("usb_drive");

 if (drives.empty())
  return NULL; // None available!

 std::vector<std::string> selections;
 for (int i = 0; i < drives.size() && i < 9; i++)
  selections.push_back( drives[i].first->tname() );

 int select = menu_vec(false, _("Choose drive:"), selections);

 return drives[ select - 1 ].first;
}

bool player::is_wearing(const itype_id & it) const
{
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == it)
   return true;
 }
 return false;
}

bool player::worn_with_flag( std::string flag ) const
{
    for (int i = 0; i < worn.size(); i++) {
        if (worn[i].has_flag( flag )) {
            return true;
        }
    }
    return false;
}

bool player::covered_with_flag(const std::string flag, int parts) const {
  int covered = 0;

  for (std::vector<item>::const_reverse_iterator armorPiece = worn.rbegin(); armorPiece != worn.rend(); ++armorPiece) {
    int cover = dynamic_cast<it_armor *>(armorPiece->type)->covers & parts;

    if (!cover) continue; // For our purposes, this piece covers nothing.
    if (cover & covered) continue; // the body part(s) is already covered.

    bool hasFlag = armorPiece->has_flag(flag);

    if (!hasFlag)
      return false; // The item is the top layer on a relevant body part, and isn't tagged, so we fail.
    else
      covered |= cover; // The item is the top layer on a relevant body part, and is tagged.
  }

  return (covered == parts);
}

bool player::covered_with_flag_exclusively(const std::string flag, int flags) const {
  for (std::vector<item>::const_iterator armorPiece = worn.begin(); armorPiece != worn.end(); ++armorPiece) {
    if ((dynamic_cast<it_armor *>(armorPiece->type)->covers & flags) && !armorPiece->has_flag(flag))
      return false;
  }

  return true;
}

bool player::is_water_friendly(int flags) const {
  return covered_with_flag_exclusively("WATER_FRIENDLY", flags);
}

bool player::is_waterproof(int flags) const {
  return covered_with_flag("WATERPROOF", flags);
}

bool player::has_artifact_with(const art_effect_passive effect) const
{
 if (weapon.is_artifact() && weapon.is_tool()) {
  it_artifact_tool *tool = dynamic_cast<it_artifact_tool*>(weapon.type);
  for (int i = 0; i < tool->effects_wielded.size(); i++) {
   if (tool->effects_wielded[i] == effect)
    return true;
  }
  for (int i = 0; i < tool->effects_carried.size(); i++) {
   if (tool->effects_carried[i] == effect)
    return true;
  }
 }
 if (inv.has_artifact_with(effect)) {
  return true;
 }
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].is_artifact()) {
   it_artifact_armor *armor = dynamic_cast<it_artifact_armor*>(worn[i].type);
   for (int i = 0; i < armor->effects_worn.size(); i++) {
    if (armor->effects_worn[i] == effect)
     return true;
   }
  }
 }
 return false;
}

bool player::has_amount(itype_id it, int quantity)
{
    if (it == "toolset")
    {
        return has_bionic("bio_tools");
    }
    return (amount_of(it) >= quantity);
}

int player::amount_of(itype_id it) {
    if (it == "toolset" && has_bionic("bio_tools")) {
        return 1;
    }
    if (it == "apparatus") {
        if (has_amount("crackpipe", 1) ||
            has_amount("can_drink", 1) ||
            has_amount("pipe_glass", 1) ||
            has_amount("pipe_tobacco", 1)) {
            return 1;
        }
    }
    int quantity = weapon.amount_of(it, true);
    for (std::vector<item>::iterator a = worn.begin(); a != worn.end(); ++a) {
        quantity += a->amount_of(it, true);
    }
    quantity += inv.amount_of(it);
    return quantity;
}

bool player::has_charges(itype_id it, long quantity)
{
    if (it == "fire" || it == "apparatus") {
        return has_fire(quantity);
    }
    return (charges_of(it) >= quantity);
}

long player::charges_of(itype_id it)
{
    if (it == "toolset") {
        if (has_bionic("bio_tools")) {
            return power_level;
        } else {
            return 0;
        }
    }
    long quantity = weapon.charges_of(it);
    for (std::vector<item>::iterator a = worn.begin(); a != worn.end(); ++a) {
        quantity += a->charges_of(it);
    }
    quantity += inv.charges_of(it);
    return quantity;
}

int  player::leak_level( std::string flag ) const
{
    int leak_level = 0;
    leak_level = inv.leak_level(flag);
    return leak_level;
}

bool player::has_drink()
{
    if (inv.has_drink()) {
        return true;
    }
    if (weapon.is_container() && !weapon.contents.empty()) {
        return weapon.contents[0].is_drink();
    }
    return false;
}

bool player::has_weapon_or_armor(char let) const
{
 if (weapon.invlet == let)
  return true;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let)
   return true;
 }
 return false;
}

bool player::has_item_with_flag( std::string flag ) const
{
    //check worn items for flag
    if (worn_with_flag( flag ))
    {
        return true;
    }

    //check weapon for flag
    if (weapon.has_flag( flag ) || weapon.contains_with_flag( flag ))
    {
        return true;
    }

    //check inventory items for flag
    if (inv.has_flag( flag ))
    {
        return true;
    }

    return false;
}

bool player::has_item(char let)
{
 return (has_weapon_or_armor(let) || !inv.item_by_letter(let).is_null());
}

std::set<char> player::allocated_invlets() {
    std::set<char> invlets = inv.allocated_invlets();

    if (weapon.invlet != 0) {
        invlets.insert(weapon.invlet);
    }
    for (int i = 0; i < worn.size(); i++) {
        invlets.insert(worn[i].invlet);
    }

    return invlets;
}

bool player::has_item(int position) {
    return !i_at(position).is_null();
}

bool player::has_item(item *it)
{
 if (it == &weapon)
  return true;
 for (int i = 0; i < worn.size(); i++) {
  if (it == &(worn[i]))
   return true;
 }
 return inv.has_item(it);
}

bool player::has_mission_item(int mission_id)
{
    if (mission_id == -1)
    {
        return false;
    }
    if (weapon.mission_id == mission_id)
    {
        return true;
    }
    for (int i = 0; i < weapon.contents.size(); i++)
    {
        if (weapon.contents[i].mission_id == mission_id)
        return true;
    }
    if (inv.has_mission_item(mission_id))
    {
        return true;
    }
    return false;
}

bool player::i_add_or_drop(item& it, int qty) {
    bool retval = true;
    bool drop = false;
    inv.assign_empty_invlet(it);
    for (int i = 0; i < qty; ++i) {
        if (!drop && (!can_pickWeight(it.weight(), !OPTIONS["DANGEROUS_PICKUPS"])
                      || !can_pickVolume(it.volume()))) {
            drop = true;
        }
        if (drop) {
            retval &= g->m.add_item_or_charges(posx, posy, it);
        } else {
            i_add(it);
        }
    }
    return retval;
}

char player::lookup_item(char let)
{
 if (weapon.invlet == let)
  return -1;

 if (inv.item_by_letter(let).is_null())
  return -2; // -2 is for "item not found"

 return let;
}

hint_rating player::rate_action_eat(item *it)
{
 //TODO more cases, for HINT_IFFY
 if (it->is_food_container() || it->is_food()) {
  return HINT_GOOD;
 }
 return HINT_CANT;
}

//Returns the amount of charges that were consumed byt he player
int player::drink_from_hands(item& water) {
    int charges_consumed = 0;
    if (query_yn(_("Drink from your hands?")))
        {
            // Create a dose of water no greater than the amount of water remaining.
            item water_temp(water);
            inv.push_back(water_temp);
            // If player is slaked water might not get consumed.
            if (consume(inv.position_by_type(water_temp.typeId())))
            {
                moves -= 350;

                charges_consumed = 1;// for some reason water_temp doesn't seem to have charges consumed, jsut set this to 1
            }
            inv.remove_item(inv.position_by_type(water_temp.typeId()));
        }
    return charges_consumed;
}


bool player::consume(int pos)
{
    item *to_eat = NULL;
    it_comest *comest = NULL;
    int which = -3; // Helps us know how to delete the item which got eaten

    if(pos == INT_MIN) {
        add_msg_if_player( m_info, _("You do not have that item."));
        return false;
    } if (is_underwater()) {
        add_msg_if_player( m_info, _("You can't do that while underwater."));
        return false;
    } else if( pos < -1 ) {
        add_msg_if_player( m_info, _( "You can't eat worn items, you have to take them off." ) );
        return false;
    } else if (pos == -1) {
        // Consume your current weapon
        if (weapon.is_food_container(this)) {
            to_eat = &weapon.contents[0];
            which = -2;
            if (weapon.contents[0].is_food()) {
                comest = dynamic_cast<it_comest*>(weapon.contents[0].type);
            }
        } else if (weapon.is_food(this)) {
            to_eat = &weapon;
            which = -1;
            comest = dynamic_cast<it_comest*>(weapon.type);
        } else {
            add_msg_if_player(m_info, _("You can't eat your %s."), weapon.tname().c_str());
            if(is_npc()) {
                debugmsg("%s tried to eat a %s", name.c_str(), weapon.tname().c_str());
            }
            return false;
        }
    } else {
        // Consume item from inventory
        item& it = inv.find_item(pos);
        if (it.is_food_container(this)) {
            to_eat = &(it.contents[0]);
            which = 1;
            if (it.contents[0].is_food()) {
                comest = dynamic_cast<it_comest*>(it.contents[0].type);
            }
        } else if (it.is_food(this)) {
            to_eat = &it;
            which = 0;
            comest = dynamic_cast<it_comest*>(it.type);
        } else {
            add_msg_if_player(m_info, _("You can't eat your %s."), it.tname().c_str());
            if(is_npc()) {
                debugmsg("%s tried to eat a %s", name.c_str(), it.tname().c_str());
            }
            return false;
        }
    }

    if(to_eat == NULL) {
        debugmsg("Consumed item is lost!");
        return false;
    }

    int amount_used = 1;
    bool was_consumed = false;
    if (comest != NULL) {
        if (comest->comesttype == "FOOD" || comest->comesttype == "DRINK") {
            was_consumed = eat(to_eat, comest);
            if (!was_consumed) {
                return was_consumed;
            }
        } else if (comest->comesttype == "MED") {
            if (comest->tool != "null") {
                // Check tools
                bool has = has_amount(comest->tool, 1);
                // Tools with charges need to have charges, not just be present.
                if (itypes[comest->tool]->count_by_charges()) {
                    has = has_charges(comest->tool, 1);
                }
                if (!has) {
                    add_msg_if_player(m_info, _("You need a %s to consume that!"),
                                         itypes[comest->tool]->nname(1).c_str());
                    return false;
                }
                use_charges(comest->tool, 1); // Tools like lighters get used
            }
            if (comest->has_use()) {
                //Check special use
                amount_used = comest->invoke(this, to_eat, false);
                if( amount_used <= 0 ) {
                    return false;
                }
            }
            consume_effects(to_eat, comest);
            moves -= 250;
            was_consumed = true;
        } else {
            debugmsg("Unknown comestible type of item: %s\n", to_eat->tname().c_str());
        }
    } else {
 // Consume other type of items.
        // For when bionics let you eat fuel
        if (to_eat->is_ammo() && has_active_bionic("bio_batteries") &&
            dynamic_cast<it_ammo*>(to_eat->type)->type == "battery") {
            const int factor = 20;
            int max_change = max_power_level - power_level;
            if (max_change == 0) {
                add_msg_if_player(m_info, _("Your internal power storage is fully powered."));
            }
            charge_power(to_eat->charges / factor);
            to_eat->charges -= max_change * factor; //negative charges seem to be okay
            to_eat->charges++; //there's a flat subtraction later
        } else if (!to_eat->type->is_food() && !to_eat->is_food_container(this)) {
            if (to_eat->type->is_book()) {
                it_book* book = dynamic_cast<it_book*>(to_eat->type);
                if (book->type != NULL && !query_yn(_("Really eat %s?"), to_eat->tname().c_str())) {
                    return false;
                }
            }
            int charge = (to_eat->volume() + to_eat->weight()) / 225;
            if (to_eat->type->m1 == "leather" || to_eat->type->m2 == "leather") {
                charge /= 4;
            }
            if (to_eat->type->m1 == "wood" || to_eat->type->m2 == "wood") {
                charge /= 2;
            }
            charge_power(charge);
            to_eat->charges = 0;
            add_msg_player_or_npc( _("You eat your %s."), _("<npcname> eats a %s."),
                                     to_eat->tname().c_str());
        }
        moves -= 250;
        was_consumed = true;
    }

    if (!was_consumed) {
        return false;
    }

    // Actions after consume
    to_eat->charges -= amount_used;
    if (to_eat->charges <= 0) {
        if (which == -1) {
            weapon = ret_null;
        } else if (which == -2) {
            weapon.contents.erase(weapon.contents.begin());
            add_msg_if_player(_("You are now wielding an empty %s."), weapon.tname().c_str());
        } else if (which == 0) {
            inv.remove_item(pos);
        } else if (which >= 0) {
            item& it = inv.find_item(pos);
            it.contents.erase(it.contents.begin());
            const bool do_restack = inv.const_stack(pos).size() > 1;
            if (!is_npc()) {
                bool drop_it = false;
                if (OPTIONS["DROP_EMPTY"] == "no") {
                    drop_it = false;
                } else if (OPTIONS["DROP_EMPTY"] == "watertight") {
                    drop_it = it.is_container() && !(it.has_flag("WATERTIGHT") && it.has_flag("SEALS"));
                } else if (OPTIONS["DROP_EMPTY"] == "all") {
                    drop_it = true;
                }
                if (drop_it) {
                    add_msg(_("You drop the empty %s."), it.tname().c_str());
                    g->m.add_item_or_charges(posx, posy, inv.remove_item(pos));
                } else {
                    add_msg(m_info, _("%c - an empty %s"), it.invlet, it.tname().c_str());
                }
            }
            if (do_restack) {
                inv.restack(this);
            }
            inv.unsort();
        }
    }
    return true;
}

bool player::eat(item *eaten, it_comest *comest)
{
    int to_eat = 1;
    if (comest == NULL) {
        debugmsg("player::eat(%s); comest is NULL!", eaten->tname().c_str());
        return false;
    }
    if (comest->tool != "null") {
        bool has = has_amount(comest->tool, 1);
        if (itypes[comest->tool]->count_by_charges()) {
            has = has_charges(comest->tool, 1);
        }
        if (!has) {
            add_msg_if_player(m_info, _("You need a %s to consume that!"),
                       itypes[comest->tool]->nname(1).c_str());
            return false;
        }
    }
    if (is_underwater()) {
        add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    // Here's why PROBOSCIS is such a negative trait.
    if ( (has_trait("PROBOSCIS")) && (comest->comesttype == "FOOD" ||
        eaten->has_flag("USE_EAT_VERB")) ) {
        add_msg_if_player(m_info,  _("Ugh, you can't drink that!"));
        return false;
    }
    bool overeating = (!has_trait("GOURMAND") && hunger < 0 &&
                       comest->nutr >= 5);
    bool hiberfood = (has_trait("HIBERNATE") && (hunger > -60 && thirst > -60 ));
    eaten->calc_rot(pos()); // check if it's rotten before eating!
    bool spoiled = eaten->rotten();

    last_item = itype_id(eaten->type->id);

    if (overeating && !has_trait("HIBERNATE") && !has_trait("EATHEALTH") && !is_npc() &&
        !has_trait("SLIMESPAWNER") && !query_yn(_("You're full.  Force yourself to eat?"))) {
        return false;
    }
    int temp_nutr = comest->nutr;
    int temp_quench = comest->quench;
    if (hiberfood && !is_npc() && (((hunger - temp_nutr) < -60) || ((thirst - temp_quench) < -60))){
       if (!query_yn(_("You're adequately fueled. Prepare for hibernation?"))) {
        return false;
       }
       else
       if(!is_npc()) {add_memorial_log(pgettext("memorial_male", "Began preparing for hibernation."),
                                       pgettext("memorial_female", "Began preparing for hibernation."));
                      add_msg(_("You've begun stockpiling calories and liquid for hibernation. You get the feeling that you should prepare for bed, just in case, but...you're hungry again, and you could eat a whole week's worth of food RIGHT NOW."));
      }
    }

    if (has_trait("CARNIVORE") && (eaten->made_of("veggy") || eaten->made_of("fruit")) && comest->nutr > 0) {
        add_msg_if_player(m_info, _("You can't stand the thought of eating veggies."));
        return false;
    }
    if ((!has_trait("SAPIOVORE") && !has_trait("CANNIBAL") && !has_trait("PSYCHOPATH")) && eaten->made_of("hflesh")&&
        !is_npc() && !query_yn(_("The thought of eating that makes you feel sick. Really do it?"))) {
        return false;
    }
    if ((!has_trait("SAPIOVORE") && has_trait("CANNIBAL") && !has_trait("PSYCHOPATH")) && eaten->made_of("hflesh")&& !is_npc() &&
        !query_yn(_("The thought of eating that makes you feel both guilty and excited. Really do it?"))) {
        return false;
    }

    if (has_trait("VEGETARIAN") && eaten->made_of("flesh") && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("MEATARIAN") && eaten->made_of("veggy") && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("LACTOSE") && eaten->made_of("milk") && (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("ANTIFRUIT") && eaten->made_of("fruit") && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("ANTIJUNK") && eaten->made_of("junk") && (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if ((has_trait("ANTIWHEAT") || has_trait("CARNIVORE")) && eaten->made_of("wheat") && (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if ((has_trait("SAPROPHAGE") && (!spoiled) && (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str()))) {
        //~ No, we don't eat "rotten" food. We eat properly aged food, like a normal person.
        //~ Semantic difference, but greatly facilitates people being proud of their character.
        add_msg_if_player(m_info,  _("It's too fresh, let it age a little first.  "));
        return false;
    }

    if (spoiled) {
        if (is_npc()) {
            return false;
        }
        if ((!(has_trait("SAPROVORE") || has_trait("SAPROPHAGE"))) &&
            !query_yn(_("This %s smells awful!  Eat it?"), eaten->tname().c_str())) {
            return false;
        }
    }

    //not working directly in the equation... can't imagine why
    int temp_hunger = hunger - comest->nutr;
    int temp_thirst = thirst - comest->quench;
    int capacity = has_trait("GOURMAND") ? -60 : -20;
    if( has_trait("HIBERNATE") && !is_npc() &&
        // If BOTH hunger and thirst are above the capacity...
        ( hunger > capacity && thirst > capacity ) &&
        // ...and EITHER of them crosses under the capacity...
        ( temp_hunger < capacity || temp_thirst < capacity ) ) {
        // Prompt to make sure player wants to gorge for hibernation...
        if( query_yn(_("Start gorging in preperation for hibernation?")) ) {
            // ...and explain what that means.
            add_msg(m_info, _("As you force yourself to eat, you have the feeling that you'll just be able to keep eating and then sleep for a long time."));
        } else {
            return false;
        }
    }

    if ( has_trait("HIBERNATE") ) {
        capacity = -620;
    }
    if ( has_trait("GIZZARD") ) {
        capacity = 0;
    }

    if( has_trait("SLIMESPAWNER") && !is_npc() ) {
        capacity -= 40;
        if ( (temp_hunger < capacity && temp_thirst <= (capacity + 10) ) ||
        (temp_thirst < capacity && temp_hunger <= (capacity + 10) ) ) {
            add_msg(m_mixed, _("You feel as though you're going to split open! In a good way??"));
            mod_pain(5);
            std::vector<point> valid;
            for (int x = posx - 1; x <= posx + 1; x++) {
                for (int y = posy - 1; y <= posy + 1; y++) {
                    if (g->is_empty(x, y)) {
                        valid.push_back( point(x, y) );
                    }
                }
            }
            monster slime(GetMType("mon_player_blob"));
            int numslime = 1;
            for (int i = 0; i < numslime; i++) {
                int index = rng(0, valid.size() - 1);
                point sp = valid[index];
                valid.erase(valid.begin() + index);
                slime.spawn(sp.x, sp.y);
                slime.friendly = -1;
                g->add_zombie(slime);
            }
            hunger += 40;
            thirst += 40;
            //~slimespawns have *small voices* which may be the Nice equivalent
            //~of the Rat King's ALL CAPS invective.  Probably shared-brain telepathy.
            add_msg(_("hey, you look like me! let's work together!"));
        }
    }

    if( ( comest->nutr > 0 && temp_hunger < capacity ) ||
        ( comest->quench > 0 && temp_thirst < capacity ) ) {
        if ((spoiled) && !(has_trait("SAPROPHAGE")) ){//rotten get random nutrification
            if (!query_yn(_("You can hardly finish it all. Consume it?"))) {
                return false;
            }
        } else {
            if ( (( comest->nutr > 0 && temp_hunger < capacity ) ||
              ( comest->quench > 0 && temp_thirst < capacity )) &&
              ( (!(has_trait("EATHEALTH"))) || (!(has_trait("SLIMESPAWNER"))) ) ) {
                if (!query_yn(_("You will not be able to finish it all. Consume it?"))) {
                return false;
                }
            }
        }
    }

    if (comest->has_use()) {
        to_eat = comest->invoke(this, eaten, false);
        if( to_eat <= 0 ) {
            return false;
        }
    }

    if ( (spoiled) && !(has_trait("SAPROPHAGE")) ) {
        add_msg(m_bad, _("Ick, this %s doesn't taste so good..."), eaten->tname().c_str());
        if (!has_trait("SAPROVORE") && !has_trait("EATDEAD") &&
       (!has_bionic("bio_digestion") || one_in(3))) {
            add_disease("foodpoison", rng(60, (comest->nutr + 1) * 60));
        }
        consume_effects(eaten, comest, spoiled);
    } else if ((spoiled) && has_trait("SAPROPHAGE")) {
        add_msg(m_good, _("Mmm, this %s tastes delicious..."), eaten->tname().c_str());
        consume_effects(eaten, comest, spoiled);
    } else {
        consume_effects(eaten, comest);
        if (!(has_trait("GOURMAND") || has_trait("HIBERNATE") || has_trait("EATHEALTH"))) {
            if ((overeating && rng(-200, 0) > hunger)) {
                vomit();
            }
        }
    }
    // At this point, we've definitely eaten the item, so use up some turns.
    int mealtime = 250;
      if (has_trait("MOUTH_TENTACLES")  || has_trait ("MANDIBLES")) {
        mealtime /= 2;
    } if (has_trait("GOURMAND")) {
        mealtime -= 100;
    } if ((has_trait("BEAK_HUM")) &&
      (comest->comesttype == "FOOD" || eaten->has_flag("USE_EAT_VERB")) ) {
        mealtime += 200; // Much better than PROBOSCIS but still optimized for fluids
    } if (has_trait("SABER_TEETH")) {
        mealtime += 250; // They get In The Way
    } if (has_trait("AMORPHOUS")) {
        mealtime *= 1.1; // Minor speed penalty for having to flow around it
                          // rather than just grab & munch
    }
        moves -= (mealtime);

    // If it's poisonous... poison us.  TODO: More several poison effects
    if (eaten->poison > 0) {
        debugmsg("Ate some posioned stuff");
        if (!has_trait("EATPOISON") && !has_trait("EATDEAD")) {
            if (eaten->poison >= rng(2, 4)) {
                add_effect("poison", eaten->poison * 100);
            }
            add_disease("foodpoison", eaten->poison * 300);
        }
    }


    if (has_trait("AMORPHOUS")) {
        add_msg_player_or_npc(_("You assimilate your %s."), _("<npcname> assimilates a %s."),
                                  eaten->tname().c_str());
    } else if (comest->comesttype == "DRINK" && !eaten->has_flag("USE_EAT_VERB")) {
        add_msg_player_or_npc( _("You drink your %s."), _("<npcname> drinks a %s."),
                                  eaten->tname().c_str());
    } else if (comest->comesttype == "FOOD" || eaten->has_flag("USE_EAT_VERB")) {
        add_msg_player_or_npc( _("You eat your %s."), _("<npcname> eats a %s."),
                                  eaten->tname().c_str());
    }

    // Moved this later in the process, so you actually eat it before converting to HP
    if ( (has_trait("EATHEALTH")) && ( comest->nutr > 0 && temp_hunger < capacity ) ) {
        int room = (capacity - temp_hunger);
        int excess_food = ((comest->nutr) - room);
        add_msg_player_or_npc( _("You feel the %s filling you out."),
                                 _("<npcname> looks better after eating the %s."),
                                  eaten->tname().c_str());
        // Guaranteed 1 HP healing, no matter what.  You're welcome.  ;-)
        if (excess_food <= 5) {
            healall(1);
        }
        // Straight conversion, except it's divided amongst all your body parts.
        else healall(excess_food /= 5);
    }

    if (itypes[comest->tool]->is_tool()) {
        use_charges(comest->tool, 1); // Tools like lighters get used
    }

    if( has_bionic("bio_ethanol") && comest->can_use( "ALCOHOL" ) ) {
        charge_power(rng(2, 8));
    }
    if( has_bionic("bio_ethanol") && comest->can_use( "ALCOHOL_WEAK" ) ) {
        charge_power(rng(1, 4));
    }
    if( has_bionic("bio_ethanol") && comest->can_use( "ALCOHOL_STRONG" ) ) {
        charge_power(rng(3, 12));
    }

    if (eaten->made_of("hflesh") && !has_trait("SAPIOVORE")) {
    // Sapiovores don't recognize humans as the same species.
    // It's not cannibalism if you're not eating your own kind.
      if (has_trait("CANNIBAL") && has_trait("PSYCHOPATH")) {
          add_msg_if_player(m_good, _("You feast upon the human flesh."));
          add_morale(MORALE_CANNIBAL, 15, 200);
      } else if (has_trait("PSYCHOPATH") && !has_trait("CANNIBAL")) {
          add_msg_if_player( _("Meh. You've eaten worse."));
      } else if (!has_trait("PSYCHOPATH") && has_trait("CANNIBAL")) {
          add_msg_if_player(m_good, _("You indulge your shameful hunger."));
          add_morale(MORALE_CANNIBAL, 10, 50);
      } else {
          add_msg_if_player(m_bad, _("You feel horrible for eating a person."));
          add_morale(MORALE_CANNIBAL, -60, -400, 600, 300);
      }
    }
    if (has_trait("VEGETARIAN") && (eaten->made_of("flesh") || eaten->made_of("hflesh") || eaten->made_of("iflesh"))) {
        add_msg_if_player(m_bad, _("Almost instantly you feel a familiar pain in your stomach."));
        add_morale(MORALE_VEGETARIAN, -75, -400, 300, 240);
    }
    if (has_trait("MEATARIAN") && eaten->made_of("veggy")) {
        add_msg_if_player(m_bad, _("Yuck! How can anybody eat this stuff?"));
        add_morale(MORALE_MEATARIAN, -75, -400, 300, 240);
    }
    if (has_trait("LACTOSE") && eaten->made_of("milk")) {
        add_msg_if_player(m_bad, _("Your stomach begins gurgling and you feel bloated and ill."));
        add_morale(MORALE_LACTOSE, -75, -400, 300, 240);
    }
    if (has_trait("ANTIFRUIT") && eaten->made_of("fruit")) {
        add_msg_if_player(m_bad, _("Yuck! How can anybody eat this stuff?"));
        add_morale(MORALE_ANTIFRUIT, -75, -400, 300, 240);
    }
    if (has_trait("ANTIJUNK") && eaten->made_of("junk")) {
        add_msg_if_player(m_bad, _("Yuck! How can anybody eat this stuff?"));
        add_morale(MORALE_ANTIJUNK, -75, -400, 300, 240);
    }
    if ((has_trait("ANTIWHEAT") || has_trait("CARNIVORE")) && eaten->made_of("wheat")) {
        add_msg_if_player(m_bad, _("Your stomach begins gurgling and you feel bloated and ill."));
        add_morale(MORALE_ANTIWHEAT, -75, -400, 300, 240);
    }
    if (has_trait("SAPROPHAGE") && !(spoiled)) {
        add_msg_if_player(m_bad, _("Your stomach begins gurgling and you feel bloated and ill."));
        add_morale(MORALE_NO_DIGEST, -75, -400, 300, 240);
    }
    if ((!crossed_threshold() || has_trait("THRESH_URSINE")) && mutation_category_level["MUTCAT_URSINE"] > 40
        && eaten->made_of("honey")) {
        //Need at least 5 bear muts for effect to show, to filter out mutations in common with other mutcats
        int honey_fun = has_trait("THRESH_URSINE") ?
            std::min(mutation_category_level["MUTCAT_URSINE"]/8, 20) :
            mutation_category_level["MUTCAT_URSINE"]/12;
        if (honey_fun < 10)
            add_msg_if_player(m_good, _("You find the sweet taste of honey surprisingly palatable."));
        else
            add_msg_if_player(m_good, _("You feast upon the sweet honey."));
        add_morale(MORALE_HONEY, honey_fun, 100);
    }
    if ((has_trait("HERBIVORE") || has_trait("RUMINANT")) &&
            (eaten->made_of("flesh") || eaten->made_of("egg"))) {
        if (!one_in(3)) {
            vomit();
        }
        if (comest->quench >= 2) {
            thirst += int(comest->quench / 2);
        }
        if (comest->nutr >= 2) {
            hunger += int(comest->nutr * .75);
        }
    }
    return true;
}

void player::consume_effects(item *eaten, it_comest *comest, bool rotten)
{
    if ((rotten) && !(has_trait("SAPROPHAGE")) ) {
        hunger -= rng(0, comest->nutr);
        thirst -= comest->quench;
        if (!has_trait("SAPROVORE") && !has_bionic("bio_digestion")) {
            health -= 3;
        }
    } if (has_trait("GIZZARD")) {
        // Shorter GI tract, so less nutrients captured.
        hunger -= ((comest->nutr) *= 0.66);
        thirst -= ((comest->quench) *= 0.66);
        health += ((comest->healthy) *= 0.66);
    } else {
    // Saprophages get the same boost from rotten food that others get from fresh.
        hunger -= comest->nutr;
        thirst -= comest->quench;
        health += comest->healthy;
    }

    if (has_bionic("bio_digestion")) {
        hunger -= rng(0, comest->nutr);
    }

    if (comest->stim != 0) {
        if (abs(stim) < (abs(comest->stim) * 3)) {
            if (comest->stim < 0) {
                stim = std::max(comest->stim * 3, stim + comest->stim);
            } else {
                stim = std::min(comest->stim * 3, stim + comest->stim);
            }
        }
    }
    add_addiction(comest->add, comest->addict);
    if (addiction_craving(comest->add) != MORALE_NULL) {
        rem_morale(addiction_craving(comest->add));
    }
    if (eaten->has_flag("HOT") && eaten->has_flag("EATEN_HOT")) {
        add_morale(MORALE_FOOD_HOT, 5, 10);
    }
    if (has_trait("GOURMAND")) {
        if (comest->fun < -2) {
            add_morale(MORALE_FOOD_BAD, comest->fun * 0.5, comest->fun, 60, 30, false, comest);
        } else if (comest->fun > 0) {
            add_morale(MORALE_FOOD_GOOD, comest->fun * 3, comest->fun * 6, 60, 30, false, comest);
        }
        if (has_trait("GOURMAND") && !(has_trait("HIBERNATE"))) {
        if ((comest->nutr > 0 && hunger < -60) || (comest->quench > 0 && thirst < -60)) {
            add_msg_if_player(_("You can't finish it all!"));
        }
        if (hunger < -60) {
            hunger = -60;
        }
        if (thirst < -60) {
            thirst = -60;
        }
    }
    } if (has_trait("HIBERNATE")) {
         if ((comest->nutr > 0 && hunger < -60) || (comest->quench > 0 && thirst < -60)) { //Tell the player what's going on
            add_msg_if_player(_("You gorge yourself, preparing to hibernate."));
            if (one_in(2)) {
                (fatigue += (comest->nutr)); //50% chance of the food tiring you
            }
        }
        if ((comest->nutr > 0 && hunger < -200) || (comest->quench > 0 && thirst < -200)) { //Hibernation should cut burn to 60/day
            add_msg_if_player(_("You feel stocked for a day or two. Got your bed all ready and secured?"));
            if (one_in(2)) {
                (fatigue += (comest->nutr)); //And another 50%, intended cumulative
            }
        }
        if ((comest->nutr > 0 && hunger < -400) || (comest->quench > 0 && thirst < -400)) {
            add_msg_if_player(_("Mmm.  You can stil fit some more in...but maybe you should get comfortable and sleep."));
             if (!(one_in(3))) {
                (fatigue += (comest->nutr)); //Third check, this one at 66%
            }
        }
        if ((comest->nutr > 0 && hunger < -600) || (comest->quench > 0 && thirst < -600)) {
            add_msg_if_player(_("That filled a hole!  Time for bed..."));
            fatigue += (comest->nutr); //At this point, you're done.  Schlaf gut.
        }
        if ((comest->nutr > 0 && hunger < -620) || (comest->quench > 0 && thirst < -620)) {
            add_msg_if_player(_("You can't finish it all!"));
        }
        if (hunger < -620) {
            hunger = -620;
        }
        if (thirst < -620) {
            thirst = -620;
        }
    } else {
        if (comest->fun < 0) {
            add_morale(MORALE_FOOD_BAD, comest->fun, comest->fun * 6, 60, 30, false, comest);
        } else if (comest->fun > 0) {
            add_morale(MORALE_FOOD_GOOD, comest->fun, comest->fun * 4, 60, 30, false, comest);
        }
        if ((comest->nutr > 0 && hunger < -20) || (comest->quench > 0 && thirst < -20)) {
            add_msg_if_player(_("You can't finish it all!"));
        }
        if (hunger < -20) {
            hunger = -20;
        }
        if (thirst < -20) {
            thirst = -20;
        }
    }
}

void player::rooted_message() const
{
    if( (has_trait("ROOTS2") || has_trait("ROOTS3") ) &&
        g->m.has_flag("DIGGABLE", posx, posy) &&
        !wearing_something_on(bp_feet) ) {
        add_msg(m_info, _("You sink your roots into the soil."));
    }
}

void player::rooted()
// Should average a point every two minutes or so; ground isn't uniformly fertile
// If being able to "overfill" is a serious balance issue, will revisit
// Otherwise, nutrient intake via roots can fill past the "Full" point, WAI
{
    if( (has_trait("ROOTS2") || has_trait("ROOTS3")) &&
        g->m.has_flag("DIGGABLE", posx, posy) &&
        !wearing_something_on(bp_feet) ) {
        if( one_in(20) ) {
            hunger--;
            thirst--;
            if (health <= 5) {
                health++;
            }
        }
    }
}

bool player::wield(item* it, bool autodrop)
{
 if (weapon.has_flag("NO_UNWIELD")) {
  add_msg(m_info, _("You cannot unwield your %s!  Withdraw them with 'p'."),
             weapon.tname().c_str());
  return false;
 }
 if (it == NULL || it->is_null()) {
  if(weapon.is_null()) {
   return false;
  }
  if (autodrop || volume_carried() + weapon.volume() < volume_capacity()) {
   inv.add_item_keep_invlet(remove_weapon());
   inv.unsort();
   moves -= 20;
   recoil = 0;
   return true;
  } else if (query_yn(_("No space in inventory for your %s.  Drop it?"),
                      weapon.tname().c_str())) {
   g->m.add_item_or_charges(posx, posy, remove_weapon());
   recoil = 0;
   return true;
  } else
   return false;
 }
 if (&weapon == it) {
  add_msg(m_info, _("You're already wielding that!"));
  return false;
 } else if (it == NULL || it->is_null()) {
  add_msg(m_info, _("You don't have that item."));
  return false;
 }

 if (it->is_two_handed(this) && !has_two_arms()) {
  add_msg(m_info, _("You cannot wield a %s with only one arm."),
             it->tname().c_str());
  return false;
 }
 if (!is_armed()) {
  weapon = inv.remove_item(it);
  if (weapon.is_artifact() && weapon.is_tool()) {
   it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(weapon.type);
   g->add_artifact_messages(art->effects_wielded);
  }
  moves -= 30;
  last_item = itype_id(weapon.type->id);
  return true;
 } else if (volume_carried() + weapon.volume() - it->volume() <
            volume_capacity()) {
  item tmpweap = remove_weapon();
  weapon = inv.remove_item(it);
  inv.add_item_keep_invlet(tmpweap);
  inv.unsort();
  moves -= 45;
  if (weapon.is_artifact() && weapon.is_tool()) {
   it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(weapon.type);
   g->add_artifact_messages(art->effects_wielded);
  }
  last_item = itype_id(weapon.type->id);
  return true;
 } else if (query_yn(_("No space in inventory for your %s.  Drop it?"),
                     weapon.tname().c_str())) {
  g->m.add_item_or_charges(posx, posy, remove_weapon());
  weapon = *it;
  inv.remove_item(weapon.invlet);
  inv.unsort();
  moves -= 30;
  if (weapon.is_artifact() && weapon.is_tool()) {
   it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(weapon.type);
   g->add_artifact_messages(art->effects_wielded);
  }
  last_item = itype_id(weapon.type->id);
  return true;
 }

 return false;

}

// ids of martial art styles that are available with the bio_cqb bionic.
static const std::array<std::string, 4> bio_cqb_styles {{
"style_karate", "style_judo", "style_muay_thai", "style_biojutsu"
}};

class ma_style_callback : public uimenu_callback
{
public:
    virtual bool key(int key, int entnum, uimenu *menu) {
        if( key != '?' ) {
            return false;
        }
        std::string style_selected;
        const size_t index = entnum;
        if( g->u.has_active_bionic( "bio_cqb" ) && index < menu->entries.size() ) {
            const size_t id = menu->entries[index].retval - 1;
            if( id < bio_cqb_styles.size() ) {
                style_selected = bio_cqb_styles[id];
            }
        } else if( index >= 2 && index - 2 < g->u.ma_styles.size() ) {
            style_selected = g->u.ma_styles[index - 2];
        }
        if( !style_selected.empty() ) {
            const martialart &ma = martialarts[style_selected];
            std::ostringstream buffer;
            buffer << ma.name << "\n\n";
            buffer << ma.description << "\n\n";
            if( !ma.techniques.empty() ) {
                buffer << ngettext( "Technique:", "Techniques:", ma.techniques.size() ) << " ";
                for( auto technique = ma.techniques.cbegin();
                     technique != ma.techniques.cend(); ++technique ) {
                    buffer << ma_techniques[*technique].name;
                    if( ma.techniques.size() > 1 && technique == ----ma.techniques.cend() ) {
                        //~ Seperators that comes before last element of a list.
                        buffer << _(" and ");
                    } else if( technique != --ma.techniques.cend() ) {
                        //~ Seperator for a list of items.
                        buffer << _(", ");
                    }
                }
                buffer << "\n";
            }
            if( !ma.weapons.empty() ) {
                buffer << ngettext( "Weapon:", "Weapons:", ma.weapons.size() ) << " ";
                for( auto weapon = ma.weapons.cbegin(); weapon != ma.weapons.cend(); ++weapon ) {
                    buffer << item(*weapon, 0).type->nname(1);
                    if( ma.weapons.size() > 1 && weapon == ----ma.weapons.cend() ) {
                        //~ Seperators that comes before last element of a list.
                        buffer << _(" and ");
                    } else if( weapon != --ma.weapons.cend() ) {
                        //~ Seperator for a list of items.
                        buffer << _(", ");
                    }
                }
            }
            popup(buffer.str(), PF_NONE);
            menu->redraw();
        }
        return true;
    }
    virtual ~ma_style_callback() { }
};

void player::pick_style() // Style selection menu
{
    //Create menu
    // Entries:
    // 0: Cancel
    // 1: No style
    // x: dynamic list of selectable styles

    //If there are style already, cursor starts there
    // if no selected styles, cursor starts from no-style

    // Any other keys quit the menu

    uimenu kmenu;
    kmenu.text = _("Select a style (press ? for style info)");
    std::auto_ptr<ma_style_callback> ma_style_info(new ma_style_callback());
    kmenu.callback = ma_style_info.get();

    if (has_active_bionic("bio_cqb")) {
        kmenu.addentry( 0, true, 'c', _("Cancel") );
        for(size_t i = 0; i < bio_cqb_styles.size(); i++) {
            if (martialarts.find(bio_cqb_styles[i]) != martialarts.end()) {
                kmenu.addentry( i + 1, true, -1, martialarts[bio_cqb_styles[i]].name );
            }
        }

        kmenu.query();
        const size_t selection = kmenu.ret - 1;
        if( selection < bio_cqb_styles.size() ) {
            style_selected = bio_cqb_styles[selection];
        }
    }
    else {
        kmenu.addentry( 0, true, 'c', _("Cancel") );
        kmenu.addentry( 1, true, 'n', _("No style") );
        kmenu.selected = 1;
        kmenu.return_invalid = true; //cancel with any other keys

        for (size_t i = 0; i < ma_styles.size(); i++) {
            if(martialarts.find(ma_styles[i]) == martialarts.end()) {
                debugmsg ("Bad hand to hand style: %s",ma_styles[i].c_str());
            } else {
                //Check if this style is currently selected
                if( ma_styles[i] == style_selected ) {
                    kmenu.selected =i+2; //+2 because there are "cancel" and "no style" first in the list
                }
                kmenu.addentry( i+2, true, -1, martialarts[ma_styles[i]].name );
            }
        }

        kmenu.query();
        int selection = kmenu.ret;

        //debugmsg("selected %d",choice);
        if (selection >= 2)
            style_selected = ma_styles[selection - 2];
        else if (selection == 1)
            style_selected = "style_none";

        //else
        //all other means -> don't change, keep current.
    }
}

hint_rating player::rate_action_wear(item *it)
{
 //TODO flag already-worn items as HINT_IFFY

 if (!it->is_armor()) {
  return HINT_CANT;
 }

 it_armor* armor = dynamic_cast<it_armor*>(it->type);

 // are we trying to put on power armor? If so, make sure we don't have any other gear on.
 if (armor->is_power_armor() && worn.size()) {
  if (armor->covers & mfb(bp_torso)) {
   return HINT_IFFY;
  } else if (armor->covers & mfb(bp_head) && !worn[0].type->is_power_armor()) {
   return HINT_IFFY;
  }
 }
 // are we trying to wear something over power armor? We can't have that, unless it's a backpack, or similar.
 if (worn.size() && worn[0].type->is_power_armor() && !(armor->covers & mfb(bp_head))) {
  if (!(armor->covers & mfb(bp_torso) && armor->color == c_green)) {
   return HINT_IFFY;
  }
 }

 // Make sure we're not wearing 2 of the item already
 int count = 0;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == it->type->id)
   count++;
 }
 if (count == 2) {
  return HINT_IFFY;
 }
 if (has_trait("WOOLALLERGY") && it->made_of("wool")) {
  return HINT_IFFY; //should this be HINT_CANT? I kinda think not, because HINT_CANT is more for things that can NEVER happen
 }
 if (armor->covers & mfb(bp_head) && encumb(bp_head) != 0) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_hands) && has_trait("WEBBED")) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_hands) && has_trait("TALONS")) {
  return HINT_IFFY;
 }
 if ( armor->covers & mfb(bp_hands) && (has_trait("ARM_TENTACLES") ||
        has_trait("ARM_TENTACLES_4") || has_trait("ARM_TENTACLES_8")) ) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_mouth) && (has_trait("BEAK") ||
    has_trait("BEAK_PECK") || has_trait("BEAK_HUM")) ) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_feet) && has_trait("HOOVES")) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_feet) && has_trait("LEG_TENTACLES")) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_head) && has_trait("HORNS_CURLED")) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_torso) && has_trait("SHELL")) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_head) && !it->made_of("wool") &&
     !it->made_of("cotton") && !it->made_of("leather") && !it->made_of("nomex") &&
     (has_trait("HORNS_POINTED") || has_trait("ANTENNAE") || has_trait("ANTLERS"))) {
  return HINT_IFFY;
 }
 // Checks to see if the player is wearing shoes
 if (armor->covers & mfb(bp_feet) && wearing_something_on(bp_feet) &&
     (!it->has_flag("BELTED")) &&
     (!it->has_flag("SKINTIGHT") && is_wearing_shoes())){
  return HINT_IFFY;
 }

 return HINT_GOOD;
}

bool player::wear(int pos, bool interactive)
{
    item* to_wear = NULL;
    if (pos == -1)
    {
        to_wear = &weapon;
    }
    else if( pos < -1 ) {
        if( interactive ) {
            add_msg( m_info, _( "You are already wearing that." ) );
        }
        return false;
    }
    else
    {
        to_wear = &inv.find_item(pos);
    }

    if (to_wear->is_null())
    {
        if(interactive)
        {
            add_msg(m_info, _("You don't have that item."));
        }

        return false;
    }

    if (!wear_item(to_wear, interactive))
    {
        return false;
    }

    if (pos == -1)
    {
        weapon = ret_null;
    }
    else
    {
        // it has been copied into worn vector, but assigned an invlet,
        // in case it's a stack, reset the invlet to avoid duplicates
        to_wear->invlet = 0;
        inv.remove_item(to_wear);
        inv.restack(this);
    }

    return true;
}

bool player::wear_item(item *to_wear, bool interactive)
{
    it_armor* armor = NULL;

    if (to_wear->is_armor())
    {
        armor = dynamic_cast<it_armor*>(to_wear->type);
    }
    else
    {
        add_msg(m_info, _("Putting on a %s would be tricky."), to_wear->tname().c_str());
        return false;
    }

    // are we trying to put on power armor? If so, make sure we don't have any other gear on.
    if (armor->is_power_armor())
    {
        for (std::vector<item>::iterator it = worn.begin(); it != worn.end(); ++it)
        {
            if ((dynamic_cast<it_armor*>(it->type))->covers & armor->covers)
            {
                if(interactive)
                {
                    add_msg(m_info, _("You can't wear power armor over other gear!"));
                }
                return false;
            }
        }

        if (!(armor->covers & mfb(bp_torso)))
        {
            bool power_armor = false;

            if (worn.size())
            {
                for (std::vector<item>::iterator it = worn.begin(); it != worn.end(); ++it)
                {
                    if (it->type->is_power_armor())
                    {
                        power_armor = true;
                        break;
                    }
                }
            }

            if (!power_armor)
            {
                if(interactive)
                {
                    add_msg(m_info, _("You can only wear power armor components with power armor!"));
                }
                return false;
            }
        }

        for (int i = 0; i < worn.size(); i++)
        {
            if (worn[i].type->is_power_armor() && worn[i].type == armor)
            {
                if(interactive)
                {
                    add_msg(m_info, _("You cannot wear more than one %s!"), to_wear->tname().c_str());
                }
                return false;
            }
        }
    }
    else
    {
        // Only headgear can be worn with power armor, except other power armor components
        if( armor->covers & ~(mfb(bp_head) | mfb(bp_eyes) | mfb(bp_mouth) ) ) {
            for (int i = 0; i < worn.size(); i++)
            {
                if( worn[i].type->is_power_armor() )
                {
                    if(interactive)
                    {
                        add_msg(m_info, _("You can't wear %s with power armor!"), to_wear->tname().c_str());
                    }
                    return false;
                }
            }
        }
    }

    // Make sure we're not wearing 2 of the item already
    int count = 0;

    for (int i = 0; i < worn.size(); i++) {
        if (worn[i].type->id == to_wear->type->id) {
            count++;
        }
    }

    if (count == 2) {
        if(interactive) {
            add_msg(m_info, _("You can't wear more than two %s at once."),
                    to_wear->tname(count).c_str());
        }
        return false;
    }

    if (!to_wear->has_flag("OVERSIZE")) {
        if (has_trait("WOOLALLERGY") && to_wear->made_of("wool")) {
            if(interactive) {
                add_msg(m_info, _("You can't wear that, it's made of wool!"));
            }
            return false;
        }

        if (armor->covers & mfb(bp_head) && encumb(bp_head) != 0 && armor->encumber > 0) {
            if(interactive) {
                add_msg(m_info, wearing_something_on(bp_head) ?
                                _("You can't wear another helmet!") : _("You can't wear a helmet!"));
            }
            return false;
        }

        if ((armor->covers & (mfb(bp_hands) | mfb(bp_arms) | mfb(bp_torso) |
                              mfb(bp_legs) | mfb(bp_feet) | mfb(bp_head))) &&
            (has_trait("HUGE") || has_trait("HUGE_OK"))) {
            if(interactive) {
                add_msg(m_info, _("The %s is much too small to fit your huge body!"),
                        armor->nname(1).c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_hands) && has_trait("WEBBED"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot put %s over your webbed hands."), armor->nname(1).c_str());
            }
            return false;
        }

        if ( armor->covers & mfb(bp_hands) &&
             (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
              has_trait("ARM_TENTACLES_8")) )
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot put %s over your tentacles."), armor->nname(1).c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_hands) && has_trait("TALONS"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot put %s over your talons."), armor->nname(1).c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_hands) && (has_trait("PAWS") || has_trait("PAWS_LARGE")) )
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot get %s to stay on your paws."), armor->nname(1).c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_mouth) && (has_trait("BEAK") || has_trait("BEAK_PECK") ||
        has_trait("BEAK_HUM")) )
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot put a %s over your beak."), armor->nname(1).c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_mouth) &&
            (has_trait("MUZZLE") || has_trait("MUZZLE_BEAR") || has_trait("MUZZLE_LONG")))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot fit the %s over your muzzle."), armor->nname(1).c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_mouth) && has_trait("MINOTAUR"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot fit the %s over your snout."), armor->nname(1).c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_mouth) && has_trait("SABER_TEETH"))
        {
            if(interactive)
            {
                add_msg(m_info, _("Your saber teeth are simply too large for %s to fit."), armor->nname(1).c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_mouth) && has_trait("MANDIBLES"))
        {
            if(interactive)
            {
                add_msg(_("Your mandibles are simply too large for %s to fit."), armor->nname(1).c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_mouth) && has_trait("PROBOSCIS"))
        {
            if(interactive)
            {
                add_msg(m_info, _("Your proboscis is simply too large for %s to fit."), armor->nname(1).c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_feet) && has_trait("HOOVES"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot wear footwear on your hooves."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_feet) && has_trait("LEG_TENTACLES"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot wear footwear on your tentacles."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_feet) && has_trait("RAP_TALONS"))
        {
            if(interactive)
            {
                add_msg(m_info, _("Your talons are much too large for footgear."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_head) && has_trait("HORNS_CURLED"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot wear headgear over your horns."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_torso) && has_trait("SHELL"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot wear anything over your shell."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_torso) && ((has_trait("INSECT_ARMS")) || (has_trait("ARACHNID_ARMS"))) )
        {
            if(interactive)
            {
                add_msg(m_info, _("Your new limbs are too wriggly to fit under that."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_head) &&
            !to_wear->made_of("wool") && !to_wear->made_of("cotton") &&
            !to_wear->made_of("nomex") && !to_wear->made_of("leather") &&
            (has_trait("HORNS_POINTED") || has_trait("ANTENNAE") || has_trait("ANTLERS")))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot wear a helmet over your %s."),
                           (has_trait("HORNS_POINTED") ? _("horns") :
                            (has_trait("ANTENNAE") ? _("antennae") : _("antlers"))));
            }
            return false;
        }

        if (armor->covers & mfb(bp_feet) && wearing_something_on(bp_feet) &&
            (!to_wear->has_flag("BELTED")) &&
            (!to_wear->has_flag("SKINTIGHT")) && is_wearing_shoes()) {
            // Checks to see if the player is wearing shoes
            if(interactive){
                add_msg(m_info, _("You're already wearing footwear!"));
            }
            return false;
        }
    }

    if (to_wear->invlet == 0) {
        inv.assign_empty_invlet( *to_wear, false );
    }

    const bool was_deaf = is_deaf();
    last_item = itype_id(to_wear->type->id);
    worn.push_back(*to_wear);

    if(interactive)
    {
        add_msg(_("You put on your %s."), to_wear->tname().c_str());
        moves -= 350; // TODO: Make this variable?

        if (to_wear->is_artifact())
        {
            it_artifact_armor *art = dynamic_cast<it_artifact_armor*>(to_wear->type);
            g->add_artifact_messages(art->effects_worn);
        }

        for (body_part i = bp_head; i < num_bp; i = body_part(i + 1))
        {
            if (armor->covers & mfb(i) && encumb(i) >= 4)
            {
                add_msg(m_warning,
                    (i == bp_head || i == bp_torso || i == bp_mouth) ?
                    _("Your %s is very encumbered! %s"):_("Your %s are very encumbered! %s"),
                    body_part_name(body_part(i), -1).c_str(), encumb_text(body_part(i)).c_str());
            }
        }
        if( !was_deaf && is_deaf() ) {
            add_msg( m_info, _( "You're deafened!" ) );
        }
    }

    recalc_sight_limits();

    return true;
}

hint_rating player::rate_action_takeoff(item *it) {
 if (!it->is_armor()) {
  return HINT_CANT;
 }

 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == it->invlet) { //surely there must be an easier way to do this?
   return HINT_GOOD;
  }
 }

 return HINT_IFFY;
}

bool player::takeoff(int pos, bool autodrop, std::vector<item> *items)
{
    bool taken_off = false;
    if (pos == -1) {
        taken_off = wield(NULL, autodrop);
    } else {
        int worn_index = worn_position_to_index(pos);
        if (worn_index >=0 && worn_index < worn.size()) {
            item &w = worn[worn_index];

            // Handle power armor.
            if (w.type->is_power_armor() &&
                    (dynamic_cast<it_armor*>(w.type)->covers & mfb(bp_torso))) {
                // We're trying to take off power armor, but cannot do that if we have a power armor component on!
                for (int j = worn.size() - 1; j >= 0; j--) {
                    if (worn[j].type->is_power_armor() &&
                            j != worn_index) {
                        if( autodrop || items != nullptr ) {
                            if( items != nullptr ) {
                                items->push_back( worn[j] );
                            } else {
                                g->m.add_item_or_charges( posx, posy, worn[j] );
                            }
                            add_msg(_("You take off your your %s."), worn[j].tname().c_str());
                            worn.erase(worn.begin() + j);
                            // If we are before worn_index, erasing this element shifted its position by 1.
                            if (worn_index > j) {
                                worn_index -= 1;
                                w = worn[worn_index];
                            }
                            taken_off = true;
                        } else {
                            add_msg(m_info, _("You can't take off power armor while wearing other power armor components."));
                            return false;
                        }
                    }
                }
            }

            if( items != nullptr ) {
                items->push_back( w );
                taken_off = true;
            } else if (autodrop || volume_capacity() - dynamic_cast<it_armor*>(w.type)->storage > volume_carried() + w.type->volume) {
                inv.add_item_keep_invlet(w);
                taken_off = true;
            } else if (query_yn(_("No room in inventory for your %s.  Drop it?"),
                    w.tname().c_str())) {
                g->m.add_item_or_charges(posx, posy, w);
                taken_off = true;
            } else {
                taken_off = false;
            }
            if( taken_off ) {
                add_msg(_("You take off your your %s."), w.tname().c_str());
                worn.erase(worn.begin() + worn_index);
            }
        } else {
            add_msg(m_info, _("You are not wearing that item."));
        }
    }

    recalc_sight_limits();

    return taken_off;
}

void player::use_wielded() {
  use(-1);
}

hint_rating player::rate_action_reload(item *it) {
    if (it->has_flag("NO_RELOAD")) {
        return HINT_CANT;
    }
    if (it->is_gun()) {
        if (it->has_flag("RELOAD_AND_SHOOT") || it->ammo_type() == "NULL") {
            return HINT_CANT;
        }
        if (it->charges == it->clip_size()) {
            int alternate_magazine = -1;
            for (int i = 0; i < it->contents.size(); i++) {
                if ((it->contents[i].is_gunmod() &&
                      (it->contents[i].typeId() == "spare_mag" &&
                      it->contents[i].charges < (dynamic_cast<it_gun*>(it->type))->clip)) ||
                      (it->contents[i].has_flag("MODE_AUX") &&
                      it->contents[i].charges < it->contents[i].clip_size())) {
                    alternate_magazine = i;
                }
            }
            if(alternate_magazine == -1) {
                return HINT_IFFY;
            }
        }
        return HINT_GOOD;
    } else if (it->is_tool()) {
        it_tool* tool = dynamic_cast<it_tool*>(it->type);
        if (tool->ammo == "NULL") {
            return HINT_CANT;
        }
        return HINT_GOOD;
    }
    return HINT_CANT;
}

hint_rating player::rate_action_unload(item *it) {
 if (!it->is_gun() && !it->is_container() &&
     (!it->is_tool() || it->ammo_type() == "NULL")) {
  return HINT_CANT;
 }
 int spare_mag = -1;
 int has_m203 = -1;
 int has_40mml = -1;
 int has_shotgun = -1;
 int has_shotgun2 = -1;
 int has_shotgun3 = -1;
 int has_auxflamer = -1;
 if (it->is_gun()) {
  spare_mag = it->has_gunmod ("spare_mag");
  has_m203 = it->has_gunmod ("m203");
  has_40mml = it->has_gunmod ("pipe_launcher40mm");
  has_shotgun = it->has_gunmod ("u_shotgun");
  has_shotgun2 = it->has_gunmod ("masterkey");
  has_shotgun3 = it->has_gunmod ("rm121aux");
  has_auxflamer = it->has_gunmod ("aux_flamer");
 }
 if (it->is_container() ||
     (it->charges == 0 &&
      (spare_mag == -1 || it->contents[spare_mag].charges <= 0) &&
      (has_m203 == -1 || it->contents[has_m203].charges <= 0) &&
      (has_40mml == -1 || it->contents[has_40mml].charges <= 0) &&
      (has_shotgun == -1 || it->contents[has_shotgun].charges <= 0) &&
      (has_shotgun2 == -1 || it->contents[has_shotgun2].charges <= 0) &&
      (has_shotgun3 == -1 || it->contents[has_shotgun3].charges <= 0) &&
      (has_auxflamer == -1 || it->contents[has_auxflamer].charges <= 0) )) {
  if (it->contents.empty()) {
   return HINT_IFFY;
  }
 }

 return HINT_GOOD;
}

//TODO refactor stuff so we don't need to have this code mirroring game::disassemble
hint_rating player::rate_action_disassemble(item *it) {
 for (recipe_map::iterator cat_iter = recipes.begin(); cat_iter != recipes.end(); ++cat_iter)
    {
        for (recipe_list::iterator list_iter = cat_iter->second.begin();
             list_iter != cat_iter->second.end();
             ++list_iter)
        {
            recipe* cur_recipe = *list_iter;
            if (it->type == itypes[cur_recipe->result] && cur_recipe->reversible)
            // ok, a valid recipe exists for the item, and it is reversible
            // assign the activity
            {
                // check tools are available
                // loop over the tools and see what's required...again
                inventory crafting_inv = g->crafting_inventory(this);
                for (int j = 0; j < cur_recipe->tools.size(); j++)
                {
                    bool have_tool = false;
                    if (cur_recipe->tools[j].empty()) // no tools required, may change this
                    {
                        have_tool = true;
                    }
                    else
                    {
                        for (int k = 0; k < cur_recipe->tools[j].size(); k++)
                        {
                            itype_id type = cur_recipe->tools[j][k].type;
                            int req = cur_recipe->tools[j][k].count; // -1 => 1

                            if ((req <= 0 && crafting_inv.has_amount (type, 1)) ||
                                (req >  0 && crafting_inv.has_charges(type, req)))
                            {
                                have_tool = true;
                                k = cur_recipe->tools[j].size();
                            }
                            // if crafting recipe required a welder, disassembly requires a hacksaw or super toolkit
                            if (type == "welder")
                            {
                                if (crafting_inv.has_amount("hacksaw", 1) ||
                                    crafting_inv.has_amount("toolset", 1))
                                {
                                    have_tool = true;
                                }
                                else
                                {
                                    have_tool = false;
                                }
                            }
                        }
                    }
                    if (!have_tool)
                    {
                       return HINT_IFFY;
                    }
                }
                // all tools present
                return HINT_GOOD;
            }
        }
    }
    if(it->is_book())
        return HINT_GOOD;
    // no recipe exists, or the item cannot be disassembled
    return HINT_CANT;
}

hint_rating player::rate_action_use(const item *it) const
{
    if (it->is_tool()) {
        it_tool *tool = dynamic_cast<it_tool*>(it->type);
        if (tool->charges_per_use != 0 && it->charges < tool->charges_per_use) {
            return HINT_IFFY;
        } else {
            return HINT_GOOD;
        }
    } else if (it->is_gunmod()) {
        if (get_skill_level("gun") == 0) {
            return HINT_IFFY;
        } else {
            return HINT_GOOD;
        }
    } else if (it->is_bionic()) {
        return HINT_GOOD;
    } else if (it->is_food() || it->is_food_container() || it->is_book() || it->is_armor()) {
        return HINT_IFFY; //the rating is subjective, could be argued as HINT_CANT or HINT_GOOD as well
    } else if (it->is_gun()) {
        if (!it->contents.empty()) {
            return HINT_GOOD;
        } else {
            return HINT_IFFY;
        }
    } else if( it->type->has_use() ) {
        return HINT_GOOD;
    }

    return HINT_CANT;
}

void player::use(int pos)
{
    item* used = &i_at(pos);
    item copy;

    if (used->is_null()) {
        add_msg(m_info, _("You do not have that item."));
        return;
    }

    last_item = itype_id(used->type->id);

    if (used->is_tool()) {
        it_tool *tool = dynamic_cast<it_tool*>(used->type);
        int charges_used = tool->invoke(this, used, false);
        if (tool->charges_per_use == 0 || used->charges >= tool->charges_per_use ||
            (used->has_flag("USE_UPS") &&
             (has_charges("adv_UPS_on", charges_used * (.6)) || has_charges("UPS_on", charges_used) ||
              has_charges("adv_UPS_off", charges_used * (.6)) ||
              has_charges("UPS_off", charges_used) ||
              (has_bionic("bio_ups") && power_level >= (charges_used / 10))))) {
            if ( charges_used >= 1 ) {
                if( tool->charges_per_use > 0 ) {
                    if (used->has_flag("USE_UPS")){
                        //If the device has been modded to run off ups,
                        // we want to reduce ups charges instead of item charges.
                        if (has_charges("adv_UPS_off", charges_used * (.6))) {
                            use_charges("adv_UPS_off", charges_used * (.6));
                        } else if (has_charges("adv_UPS_on", charges_used * (.6))) {
                            use_charges("adv_UPS_on", charges_used * (.6));
                        } else if (has_charges("UPS_off", charges_used)) {
                            use_charges("UPS_off", charges_used);
                        } else if (has_charges("UPS_on", charges_used)) {
                            use_charges("UPS_on", charges_used);
                        } else if (has_bionic("bio_ups")) {
                            charge_power(-1 * (charges_used/10));
                        }
                    } else {
                        used->charges -= std::min(used->charges, long(charges_used));
                    }
                } else {
                    // An item that doesn't normally expend charges is destroyed instead.
                    /* We can't be certain the item is still in the same position,
                     * as other items may have been consumed as well, so remove
                     * the item directly instead of by its position. */
                    i_rem(used);
                }
            }
            // We may have fiddled with the state of the item in the iuse method,
            // so restack to sort things out.
            inv.restack();
        } else {
            add_msg(m_info, ngettext("Your %s has %d charge but needs %d.",
                                "Your %s has %d charges but needs %d.",
                                used->charges),
                       used->tname().c_str(),
                       used->charges, tool->charges_per_use);
        }
    } else if (used->is_gunmod()) {
        if (skillLevel("gun") == 0) {
            add_msg(m_info, _("You need to be at least level 1 in the marksmanship skill before you\
 can modify weapons."));
            return;
        }
        int gunpos = g->inv(_("Select gun to modify:"));
        it_gunmod *mod = dynamic_cast<it_gunmod*>(used->type);
        item* gun = &(i_at(gunpos));
        if (gun->is_null()) {
            add_msg(m_info, _("You do not have that item."));
            return;
        } else if (!gun->is_gun()) {
            add_msg(m_info, _("That %s is not a weapon."), gun->tname().c_str());
            return;
        }
        it_gun* guntype = dynamic_cast<it_gun*>(gun->type);
        if (guntype->skill_used == Skill::skill("pistol") && !mod->used_on_pistol) {
            add_msg(m_info, _("That %s cannot be attached to a handgun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("shotgun") && !mod->used_on_shotgun) {
            add_msg(m_info, _("That %s cannot be attached to a shotgun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("smg") && !mod->used_on_smg) {
            add_msg(m_info, _("That %s cannot be attached to a submachine gun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("rifle") && !mod->used_on_rifle) {
            add_msg(m_info, _("That %s cannot be attached to a rifle."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("archery") && !mod->used_on_bow && guntype->ammo == "arrow") {
            add_msg(m_info, _("That %s cannot be attached to a bow."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("archery") && !mod->used_on_crossbow && guntype->ammo == "bolt") {
            add_msg(m_info, _("That %s cannot be attached to a crossbow."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("launcher") && !mod->used_on_launcher) {
            add_msg(m_info, _("That %s cannot be attached to a launcher."),
                       used->tname().c_str());
            return;
        } else if ( !mod->acceptible_ammo_types.empty() &&
                    mod->acceptible_ammo_types.count(guntype->ammo) == 0 ) {
                add_msg(m_info, _("That %s cannot be used on a %s."), used->tname().c_str(),
                       ammo_name(guntype->ammo).c_str());
                return;
        } else if (guntype->valid_mod_locations.count(mod->location) == 0) {
            add_msg(m_info, _("Your %s doesn't have a slot for this mod."), gun->tname().c_str());
            return;
        } else if (gun->get_free_mod_locations(mod->location) <= 0) {
            add_msg(m_info, _("Your %s doesn't have enough room for another %s mod. To remove the mods, \
activate your weapon."), gun->tname().c_str(), _(mod->location.c_str()));
            return;
        }
        if (mod->id == "spare_mag" && gun->has_flag("RELOAD_ONE")) {
            add_msg(m_info, _("You can not use a spare magazine in your %s."),
                       gun->tname().c_str());
            return;
        }
        if (mod->location == "magazine" &&
            gun->clip_size() <= 2) {
            add_msg(m_info, _("You can not extend the ammo capacity of your %s."),
                       gun->tname().c_str());
            return;
        }
        if (mod->id == "waterproof_gunmod" && gun->has_flag("WATERPROOF_GUN")) {
            add_msg(m_info, _("Your %s is already waterproof."),
                       gun->tname().c_str());
            return;
        }
        if (mod->id == "tuned_mechanism" && gun->has_flag("NEVER_JAMS")) {
            add_msg(m_info, _("This %s is eminently reliable. You can't improve upon it this way."),
                       gun->tname().c_str());
            return;
        }
        if (guntype->id == "hand_crossbow" && !mod->used_on_pistol) {
          add_msg(m_info, _("Your %s isn't big enough to use that mod.'"), gun->tname().c_str(),
          used->tname().c_str());
          return;
        }
        if (mod->id == "brass_catcher" && gun->has_flag("RELOAD_EJECT")) {
            add_msg(m_info, _("You cannot attach a brass catcher to your %s."),
                       gun->tname().c_str());
            return;
        }
        for (int i = 0; i < gun->contents.size(); i++) {
            if (gun->contents[i].type->id == used->type->id) {
                add_msg(m_info, _("Your %s already has a %s."), gun->tname().c_str(),
                           used->tname().c_str());
                return;
            } else if ((mod->id == "clip" || mod->id == "clip2") &&
                       (gun->contents[i].type->id == "clip" ||
                        gun->contents[i].type->id == "clip2")) {
                add_msg(m_info, _("Your %s already has an extended magazine."),
                           gun->tname().c_str());
                return;
            }
        }
        add_msg(_("You attach the %s to your %s."), used->tname().c_str(),
                   gun->tname().c_str());
        gun->contents.push_back(i_rem(used));
        return;

    } else if (used->is_bionic()) {
        it_bionic* tmp = dynamic_cast<it_bionic*>(used->type);
        if (install_bionics(tmp)) {
            i_rem(pos);
        }
        return;
    } else if (used->is_food() || used->is_food_container()) {
        consume(pos);
        return;
    } else if (used->is_book()) {
        read(pos);
        return;
    } else if (used->is_gun()) {
        std::vector<item> &mods = used->contents;
        // Get weapon mod names.
        if (mods.empty()) {
            add_msg(m_info, _("Your %s doesn't appear to be modded."), used->tname().c_str());
            return;
        }
        // Create menu.
        int choice = -1;

        uimenu kmenu;
        kmenu.selected = 0;
        kmenu.text = _("Remove which modification?");
        for (int i = 0; i < mods.size(); i++) {
            kmenu.addentry( i, true, -1, mods[i].tname() );
        }
        kmenu.addentry( mods.size(), true, 'r', _("Remove all") );
        kmenu.addentry( mods.size() + 1, true, 'q', _("Cancel") );
        kmenu.query();
        choice = kmenu.ret;

        if (choice < mods.size()) {
            const std::string mod = used->contents[choice].tname();
            remove_gunmod(used, choice);
            add_msg(_("You remove your %s from your %s."), mod.c_str(), used->tname().c_str());
        } else if (choice == mods.size()) {
            for (int i = used->contents.size() - 1; i >= 0; i--) {
                remove_gunmod(used, i);
            }
            add_msg(_("You remove all the modifications from your %s."), used->tname().c_str());
        } else {
            add_msg(_("Never mind."));
            return;
        }
        // Removing stuff from a gun takes time.
        moves -= int(used->reload_time(*this) / 2);
        return;
    } else if ( used->type->has_use() ) {
        used->type->invoke(this, used, false);
        return;
    } else {
        add_msg(m_info, _("You can't do anything interesting with your %s."),
                used->tname().c_str());
        return;
    }
}

void player::remove_gunmod(item *weapon, int id)
{
    if (id >= weapon->contents.size()) {
        return;
    }
    item *gunmod = &weapon->contents[id];
    item ammo;
    if (gunmod->charges > 0) {
        if (gunmod->curammo != NULL) {
            ammo = item(gunmod->curammo->id, calendar::turn);
        } else {
            ammo = item(default_ammo(weapon->ammo_type()), calendar::turn);
        }
        ammo.charges = gunmod->charges;
        if (ammo.made_of(LIQUID)) {
            while(!g->handle_liquid(ammo, false, false)) {
                // handled only part of it, retry
            }
        } else {
            i_add_or_drop(ammo);
        }
        gunmod->curammo = NULL;
        gunmod->charges = 0;
    }
    if (gunmod->mode == "MODE_AUX") {
        weapon->next_mode();
    }
    i_add_or_drop(*gunmod);
    weapon->contents.erase(weapon->contents.begin()+id);
}

hint_rating player::rate_action_read(item *it)
{
 //note: there's a cryptic note about macguffins in player::read(). Do we have to account for those?
 if (!it->is_book()) {
  return HINT_CANT;
 }

 it_book *book = dynamic_cast<it_book*>(it->type);

 if (g && g->light_level() < 8 && LL_LIT > g->m.light_at(posx, posy)) {
  return HINT_IFFY;
 } else if (morale_level() < MIN_MORALE_READ &&  book->fun <= 0) {
  return HINT_IFFY; //won't read non-fun books when sad
 } else if (book->intel > 0 && has_trait("ILLITERATE")) {
  return HINT_IFFY;
 } else if (has_trait("HYPEROPIC") && !is_wearing("glasses_reading")
            && !is_wearing("glasses_bifocal") && !has_disease("contacts")) {
  return HINT_IFFY;
 }

 return HINT_GOOD;
}

void player::read(int pos)
{
    vehicle *veh = g->m.veh_at (posx, posy);
    if (veh && veh->player_in_control (this)) {
        add_msg(m_info, _("It's a bad idea to read while driving!"));
        return;
    }

    // Check if reading is okay
    // check for light level
    if (fine_detail_vision_mod() > 4) { //minimum LL_LOW or LL_DARK + (ELFA_NV or atomic_light)
        add_msg(m_info, _("You can't see to read!"));
        return;
    }

    // check for traits
    if (has_trait("HYPEROPIC") && !is_wearing("glasses_reading") &&
        !is_wearing("glasses_bifocal") && !has_disease("contacts")) {
        add_msg(m_info, _("Your eyes won't focus without reading glasses."));
        return;
    }

    // Find the object
    item* it = &i_at(pos);

    if (it == NULL || it->is_null()) {
        add_msg(m_info, _("You do not have that item."));
        return;
    }

// Some macguffins can be read, but they aren't treated like books.
    it_macguffin* mac = NULL;
    if (it->is_macguffin()) {
        mac = dynamic_cast<it_macguffin*>(it->type);
    }
    if (mac != NULL) {
        mac->invoke(this, it, false);
        return;
    }

    if (!it->is_book()) {
        add_msg(m_info, _("Your %s is not good reading material."),
        it->tname().c_str());
        return;
    }

    it_book* tmp = dynamic_cast<it_book*>(it->type);
    int time; //Declare this here so that we can change the time depending on whats needed
    // activity.get_value(0) == 1: see below at player_activity(ACT_READ)
    const bool continuous = (activity.get_value(0) == 1);
    bool study = continuous;
    if (tmp->intel > 0 && has_trait("ILLITERATE")) {
        add_msg(m_info, _("You're illiterate!"));
        return;
    }

    // Now we've established that the player CAN read.

    // If the player hasn't read this book before, skim it to get an idea of what's in it.
    if( !has_identified( it->type->id ) ) {
        // Base read_speed() is 1000 move points (1 minute per tmp->time)
        time = tmp->time * read_speed() * (fine_detail_vision_mod());
        if (tmp->intel > int_cur) {
            // Lower int characters can read, at a speed penalty
            time += (tmp->time * (tmp->intel - int_cur) * 100);
        }
        // We're just skimming, so it's 10x faster.
        time /= 10;

        activity = player_activity(ACT_READ, time - moves, -1, pos, "");
        // Never trigger studying when skimming the book.
        activity.values.push_back(0);
        moves = 0;
        return;
    }



    if (tmp->type == NULL) {
        // special guidebook effect: print a misc. hint when read
        if (tmp->id == "guidebook") {
            add_msg(m_info, get_hint().c_str());
            moves -= 100;
            return;
        }
        // otherwise do nothing as there's no associated skill
    } else if (morale_level() < MIN_MORALE_READ && tmp->fun <= 0) { // See morale.h
        add_msg(m_info, _("What's the point of studying?  (Your morale is too low!)"));
        return;
    } else if (skillLevel(tmp->type) < (int)tmp->req) {
        add_msg(_("The %s-related jargon flies over your head!"),
                   tmp->type->name().c_str());
        if (tmp->recipes.empty()) {
            return;
        } else {
            add_msg(m_info, _("But you might be able to learn a recipe or two."));
        }
    } else if (skillLevel(tmp->type) >= (int)tmp->level && !can_study_recipe(tmp) &&
               !query_yn(tmp->fun > 0 ?
                         _("It would be fun, but your %s skill won't be improved.  Read anyway?") :
                         _("Your %s skill won't be improved.  Read anyway?"),
                         tmp->type->name().c_str())) {
        return;
    } else if (!continuous && !query_yn(_("Study %s until you learn something? (gain a level)"),
                                        tmp->type->name().c_str())) {
        study = false;
    } else {
        //If we just started studying, tell the player how to stop
        if(!continuous) {
            add_msg(m_info, _("Now studying %s, %s to stop early."),
                    it->tname().c_str(), press_x(ACTION_PAUSE).c_str());
            if ( (has_trait("ROOTS2") || (has_trait("ROOTS3"))) &&
                 g->m.has_flag("DIGGABLE", posx, posy) &&
                 (!(wearing_something_on(bp_feet))) ) {
                add_msg(m_info, _("You sink your roots into the soil."));
            }
        }
        study = true;
    }

    if (!tmp->recipes.empty() && !continuous) {
        if (can_study_recipe(tmp)) {
            add_msg(m_info, _("This book has more recipes for you to learn."));
        } else if (studied_all_recipes(tmp)) {
            add_msg(m_info, _("You know all the recipes this book has to offer."));
        } else {
            add_msg(m_info, _("This book has more recipes, but you don't have the skill to learn them yet."));
        }
    }

    // Base read_speed() is 1000 move points (1 minute per tmp->time)
    time = tmp->time * read_speed() * (fine_detail_vision_mod());
    if (fine_detail_vision_mod() > 1.0) {
        add_msg(m_warning, _("It's difficult to see fine details right now. Reading will take longer than usual."));
    }

    if (tmp->intel > int_cur) {
        add_msg(m_warning, _("This book is too complex for you to easily understand. It will take longer to read."));
        // Lower int characters can read, at a speed penalty
        time += (tmp->time * (tmp->intel - int_cur) * 100);
    }

    activity = player_activity(ACT_READ, time, -1, pos, "");
    // activity.get_value(0) == 1 means continuous studing until
    // the player gained the next skill level, this ensured by this:
    activity.values.push_back(study ? 1 : 0);
    moves = 0;

    // Reinforce any existing morale bonus/penalty, so it doesn't decay
    // away while you read more.
    int minutes = time / 1000;
    // If you don't have a problem with eating humans, To Serve Man becomes rewarding
    if ((has_trait("CANNIBAL") || has_trait("PSYCHOPATH") || has_trait("SAPIOVORE")) &&
        tmp->id == "cookbook_human") {
        add_morale(MORALE_BOOK, 0, 75, minutes + 30, minutes, false, tmp);
    } else {
        add_morale(MORALE_BOOK, 0, tmp->fun * 15, minutes + 30, minutes, false, tmp);
    }
}

void player::do_read( item *book )
{
    it_book *reading = dynamic_cast<it_book *>(book->type);

    if( !has_identified( book->type->id ) ) {
        // Note that we've read the book.
        items_identified.insert( book->type->id );

        add_msg(_("You skim %s to find out what's in it."), reading->nname(1).c_str());
        if( reading->type ) {
            add_msg(_("Can bring your %s skill to %d."),
                    reading->type->name().c_str(), reading->level);
            if( reading->req != 0 ){
                add_msg(_("Requires %s level %d to understand."),
                        reading->type->name().c_str(), reading->req);
            }
        }

        add_msg(_("Requires intelligence of %d to easily read."), reading->intel);
        if (reading->fun != 0) {
            add_msg(_("Reading this book affects your morale by %d"), reading->fun);
        }
        add_msg(ngettext("This book takes %d minute to read.",
                         "This book takes %d minutes to read.", reading->time),
                reading->time );

        if (!(reading->recipes.empty())) {
            std::string recipes = "";
            size_t index = 1;
            for (std::map<recipe*, int>::iterator iter = reading->recipes.begin();
                 iter != reading->recipes.end(); ++iter, ++index) {
                recipes += item( iter->first->result, 0 ).type->nname(1);
                if(index == reading->recipes.size() - 1) {
                    recipes += _(" and "); // Who gives a fuck about an oxford comma?
                } else if(index != reading->recipes.size()) {
                    recipes += _(", ");
                }
            }
            std::string recipe_line = string_format(
                ngettext("This book contains %1$d crafting recipe: %2$s",
                         "This book contains %1$d crafting recipes: %2$s", reading->recipes.size()),
                reading->recipes.size(), recipes.c_str());
            add_msg( "%s", recipe_line.c_str());
        }
        activity.type = ACT_NULL;
        return;
    }

    if( reading->fun != 0 ) {
        int fun_bonus = 0;
        if( book->charges == 0 ) {
            //Book is out of chapters -> re-reading old book, less fun
            add_msg(_("The %s isn't as much fun now that you've finished it."), book->tname().c_str());
            if( one_in(6) ) { //Don't nag incessantly, just once in a while
                add_msg(m_info, _("Maybe you should find something new to read..."));
            }
            //50% penalty
            fun_bonus = (reading->fun * 5) / 2;
        } else {
            fun_bonus = reading->fun * 5;
        }
        // If you don't have a problem with eating humans, To Serve Man becomes rewarding
        if( (has_trait("CANNIBAL") || has_trait("PSYCHOPATH") || has_trait("SAPIOVORE")) &&
            reading->id == "cookbook_human" ) {
            fun_bonus = 25;
            add_morale(MORALE_BOOK, fun_bonus, fun_bonus * 3, 60, 30, true, reading);
        } else {
            add_morale(MORALE_BOOK, fun_bonus, reading->fun * 15, 60, 30, true, reading);
        }
    }

    if( book->charges > 0 ) {
        book->charges--;
    }

    bool no_recipes = true;
    if( !reading->recipes.empty() ) {
        bool recipe_learned = try_study_recipe(reading);
        if( !studied_all_recipes(reading) ) {
            no_recipes = false;
        }

        // for books that the player cannot yet read due to skill level or have no skill component,
        // but contain lower level recipes, break out once recipe has been studied
        if( reading->type == NULL || (skillLevel(reading->type) < (int)reading->req) ) {
            if( recipe_learned ) {
                add_msg(m_info, _("The rest of the book is currently still beyond your understanding."));
            }
            activity.type = ACT_NULL;
            return;
        }
    }

    if( skillLevel(reading->type) < (int)reading->level ) {
        int originalSkillLevel = skillLevel( reading->type );
        int min_ex = reading->time / 10 + int_cur / 4;
        int max_ex = reading->time /  5 + int_cur / 2 - originalSkillLevel;
        if (min_ex < 1) {
            min_ex = 1;
        }
        if (max_ex < 2) {
            max_ex = 2;
        }
        if (max_ex > 10) {
            max_ex = 10;
        }
        if (max_ex < min_ex) {
            max_ex = min_ex;
        }

        min_ex *= originalSkillLevel + 1;
        max_ex *= originalSkillLevel + 1;

        skillLevel(reading->type).readBook( min_ex, max_ex, reading->level );

        add_msg(_("You learn a little about %s! (%d%%)"), reading->type->name().c_str(),
                skillLevel(reading->type).exercise());

        if (skillLevel(reading->type) == originalSkillLevel && activity.get_value(0) == 1) {
            // continuously read until player gains a new skill level
            activity.type = ACT_NULL;
            read(activity.position);
            // Rooters root (based on time spent reading)
            int root_factor = (reading->time / 20);
            if( (has_trait("ROOTS2") || has_trait("ROOTS3")) &&
                g->m.has_flag("DIGGABLE", posx, posy) &&
                !wearing_something_on(bp_feet) ) {
                hunger -= root_factor;
                thirst -= root_factor;
                health += root_factor;
                health = std::min( 5, health );
            }
            if (activity.type != ACT_NULL) {
                return;
            }
        }

        int new_skill_level = (int)skillLevel(reading->type);
        if (new_skill_level > originalSkillLevel) {
            add_msg(m_good, _("You increase %s to level %d."),
                    reading->type->name().c_str(),
                    new_skill_level);

            if(new_skill_level % 4 == 0) {
                //~ %s is skill name. %d is skill level
                add_memorial_log(pgettext("memorial_male", "Reached skill level %1$d in %2$s."),
                                   pgettext("memorial_female", "Reached skill level %1$d in %2$s."),
                                   new_skill_level, reading->type->name().c_str());
            }
        }

        if (skillLevel(reading->type) == (int)reading->level) {
            if (no_recipes) {
                add_msg(m_info, _("You can no longer learn from %s."), reading->nname(1).c_str());
            } else {
                add_msg(m_info, _("Your skill level won't improve, but %s has more recipes for yo"),
                        reading->nname(1).c_str());
            }
        }
    }

    if( reading->has_use() ) {
        reading->invoke( &g->u, book, false );
    }

    activity.type = ACT_NULL;
}

bool player::has_identified( std::string item_id ) const
{
    return items_identified.count( item_id ) > 0;
}

bool player::can_study_recipe(it_book* book)
{
    for (std::map<recipe*, int>::iterator iter = book->recipes.begin();
         iter != book->recipes.end(); ++iter) {
        if (!knows_recipe(iter->first) &&
            (iter->first->skill_used == NULL || skillLevel(iter->first->skill_used) >= iter->second)) {
            return true;
        }
    }
    return false;
}

bool player::studied_all_recipes(it_book* book)
{
    for (std::map<recipe*, int>::iterator iter = book->recipes.begin();
         iter != book->recipes.end(); ++iter) {
        if (!knows_recipe(iter->first)) {
            return false;
        }
    }
    return true;
}

bool player::try_study_recipe(it_book *book)
{
    for (std::map<recipe*, int>::iterator iter = book->recipes.begin();
         iter != book->recipes.end(); ++iter) {
        if (!knows_recipe(iter->first) &&
            (iter->first->skill_used == NULL ||
             skillLevel(iter->first->skill_used) >= iter->second)) {
            if (iter->first->skill_used == NULL ||
                rng(0, 4) <= (skillLevel(iter->first->skill_used) - iter->second) / 2) {
                learn_recipe(iter->first);
                add_msg(m_good, _("Learned a recipe for %s from the %s."),
                                itypes[iter->first->result]->nname(1).c_str(), book->nname(1).c_str());
                return true;
            } else {
                add_msg(_("Failed to learn a recipe from the %s."), book->nname(1).c_str());
                return false;
            }
        }
    }
    return true; // _("false") seems to mean _("attempted and failed")
}

void player::try_to_sleep()
{
    int vpart = -1;
    vehicle *veh = g->m.veh_at (posx, posy, vpart);
    const trap_id trap_at_pos = g->m.tr_at(posx, posy);
    const ter_id ter_at_pos = g->m.ter(posx, posy);
    const furn_id furn_at_pos = g->m.furn(posx, posy);
    bool plantsleep = false;
    if (has_trait("CHLOROMORPH")) {
        plantsleep = true;
        if( (ter_at_pos == t_dirt || ter_at_pos == t_pit ||
             ter_at_pos == t_dirtmound || ter_at_pos == t_pit_shallow ||
             ter_at_pos == t_ash || ter_at_pos == t_grass) && (!(veh)) &&
            (furn_at_pos == f_null) ) {
            add_msg(m_good, _("You relax as your roots embrace the soil."));
        } else if (veh) {
            add_msg(m_bad, _("It's impossible to sleep in this wheeled pot!"));
        } else if (furn_at_pos != f_null) {
            add_msg(m_bad, _("The humans' furniture blocks your roots. You can't get comfortable."));
        } else { // Floor problems
            add_msg(m_bad, _("Your roots scrabble ineffectively at the unyielding surface."));
        }
    }
    if( (furn_at_pos == f_bed || furn_at_pos == f_makeshift_bed ||
         trap_at_pos == tr_cot || trap_at_pos == tr_rollmat ||
         trap_at_pos == tr_fur_rollmat || furn_at_pos == f_armchair ||
         furn_at_pos == f_sofa || furn_at_pos == f_hay ||
         (veh && veh->part_with_feature (vpart, "SEAT") >= 0) ||
         (veh && veh->part_with_feature (vpart, "BED") >= 0)) &&
        (!(plantsleep)) ) {
        add_msg(m_good, _("This is a comfortable place to sleep."));
    } else if ((ter_at_pos != t_floor) && (!(plantsleep))) {
        add_msg( terlist[ter_at_pos].movecost <= 2 ?
                 _("It's a little hard to get to sleep on this %s.") :
                 _("It's hard to get to sleep on this %s."),
                 terlist[ter_at_pos].name.c_str() );
    }
    add_disease("lying_down", 300);
}

bool player::can_sleep()
{
 int sleepy = 0;
 bool plantsleep = false;
 if (has_addiction(ADD_SLEEP))
  sleepy -= 3;
 if (has_trait("INSOMNIA"))
  sleepy -= 8;
 if (has_trait("EASYSLEEPER"))
  sleepy += 8;
 if (has_trait("CHLOROMORPH"))
  plantsleep = true;

 int vpart = -1;
 vehicle *veh = g->m.veh_at (posx, posy, vpart);
 const trap_id trap_at_pos = g->m.tr_at(posx, posy);
 const ter_id ter_at_pos = g->m.ter(posx, posy);
 const furn_id furn_at_pos = g->m.furn(posx, posy);
 if ( ((veh && veh->part_with_feature (vpart, "BED") >= 0) ||
     furn_at_pos == f_makeshift_bed || trap_at_pos == tr_cot ||
     furn_at_pos == f_sofa || furn_at_pos == f_hay) &&
     (!(plantsleep)) ) {
  sleepy += 4;
 }
 else if ( ((veh && veh->part_with_feature (vpart, "SEAT") >= 0) ||
      trap_at_pos == tr_rollmat || trap_at_pos == tr_fur_rollmat ||
      furn_at_pos == f_armchair) && (!(plantsleep)) ) {
    sleepy += 3;
 }
 else if ( (furn_at_pos == f_bed) && (!(plantsleep)) ) {
    sleepy += 5;
 }
 else if ( (ter_at_pos == t_floor) && (!(plantsleep)) ) {
    sleepy += 1;
 }
 else if (plantsleep) {
    if ((ter_at_pos == t_dirt || ter_at_pos == t_pit ||
        ter_at_pos == t_dirtmound || ter_at_pos == t_pit_shallow) &&
        furn_at_pos == f_null) {
        sleepy += 10; // It's very easy for Chloromorphs to get to sleep on soil!
    }
    else if ((ter_at_pos == t_grass || ter_at_pos == t_ash) &&
        furn_at_pos == f_null) {
        sleepy += 5; // Not as much if you have to dig through stuff first
    }
    else {
        sleepy -= 999; // Sleep ain't happening
    }
 }
 else
  sleepy -= g->m.move_cost(posx, posy);
 if (fatigue < 192)
  sleepy -= int( (192 - fatigue) / 4);
 else
  sleepy += int((fatigue - 192) / 16);
 sleepy += rng(-8, 8);
 sleepy -= 2 * stim;
 if (sleepy > 0)
  return true;
 return false;
}

void player::fall_asleep(int duration)
{
    add_disease("sleep", duration);
}

void player::wake_up(const char * message)
{
    rem_disease("sleep");
    if (message) {
        add_msg_if_player(message);
    } else {
        add_msg_if_player(_("You wake up."));
    }
}

std::string player::is_snuggling()
{
    std::vector<item> *items_to_snuggle = &g->m.i_at( posx, posy );
    if( in_vehicle ) {
        int vpart;
        vehicle *veh = g->m.veh_at( posx, posy, vpart );
        if( veh != nullptr ) {
            int cargo = veh->part_with_feature( vpart, VPFLAG_CARGO, false );
            if( cargo >= 0 ) {
                if( !veh->parts[cargo].items.empty() ) {
                    items_to_snuggle = &veh->parts[cargo].items;
                }
            }
        }
    }
    std::vector<item>& floor_item = *items_to_snuggle;
    it_armor* floor_armor = NULL;
    int ticker = 0;

    // If there are no items on the floor, return nothing
    if ( floor_item.empty() ) {
        return "nothing";
    }

    for ( std::vector<item>::iterator afloor_item = floor_item.begin() ; afloor_item != floor_item.end() ; ++afloor_item) {
        if ( !afloor_item->is_armor() ) {
            continue;
        }
        else if ( afloor_item->volume() > 1 &&
        (dynamic_cast<it_armor*>(afloor_item->type)->covers & mfb(bp_torso) ||
         dynamic_cast<it_armor*>(afloor_item->type)->covers & mfb(bp_legs)) ){
            floor_armor = dynamic_cast<it_armor*>(afloor_item->type);
            ticker++;
        }
    }

    if ( ticker == 0 ) {
        return "nothing";
    }
    else if ( ticker == 1 ) {
        return floor_armor->nname(1).c_str();
    }
    else if ( ticker > 1 ) {
        return "many";
    }

    return "nothing";
}

// Returned values range from 1.0 (unimpeded vision) to 5.0 (totally blind).
// 2.5 is enough light for detail work.
float player::fine_detail_vision_mod()
{
    // PER_SLIME_OK implies you can get enough eyes around the bile
    // that you can generaly see.  There'll still be the haze, but
    // it's annoying rather than limiting.
    if (has_effect("blind") || ((has_disease("boomered")) &&
    !(has_trait("PER_SLIME_OK"))))
    {
        return 5;
    }
    if ( has_nv() )
    {
        return 1.5;
    }
    // flashlight is handled by the light level check below
    if (has_active_item("lightstrip"))
    {
        return 1;
    }
    if (LL_LIT <= g->m.light_at(posx, posy))
    {
        return 1;
    }

    float vision_ii = 0;
    if (g->m.light_at(posx, posy) == LL_LOW) { vision_ii = 4; }
    else if (g->m.light_at(posx, posy) == LL_DARK) { vision_ii = 5; }

    if (has_item_with_flag("LIGHT_2")){
        vision_ii -= 2;
    } else if (has_item_with_flag("LIGHT_1")){
        vision_ii -= 1;
    }

    if (has_trait("NIGHTVISION")) { vision_ii -= .5; }
    else if (has_trait("ELFA_NV")) { vision_ii -= 1; }
    else if (has_trait("NIGHTVISION2") || has_trait("FEL_NV")) { vision_ii -= 2; }
    else if (has_trait("NIGHTVISION3") || has_trait("ELFA_FNV") || is_wearing("rm13_armor_on")) { vision_ii -= 3; }

    if (vision_ii < 1) { vision_ii = 1; }
    return vision_ii;
}

int player::warmth(body_part bp)
{
    int bodywetness = 0;
    int ret = 0, warmth = 0;
    it_armor* armor = NULL;

    // Fetch the morale value of wetness for bodywetness
    for (int i = 0; bodywetness == 0 && i < morale.size(); i++)
    {
        if( morale[i].type == MORALE_WET )
        {
            bodywetness = abs(morale[i].bonus); // Make it positive, less confusing
            break;
        }
    }

    // If the player is not wielding anything, check if hands can be put in pockets
    if(bp == bp_hands && !is_armed() && (temp_conv[bp] <=  BODYTEMP_COLD) && worn_with_flag("POCKETS"))
    {
        ret += 10;
    }

    // If the players head is not encumbered, check if hood can be put up
    if(bp == bp_head && encumb(bp_head) < 1 && (temp_conv[bp] <=  BODYTEMP_COLD) && worn_with_flag("HOOD"))
    {
        ret += 10;
    }

    for (int i = 0; i < worn.size(); i++)
    {
        armor = dynamic_cast<it_armor*>(worn[i].type);

        if (armor->covers & mfb(bp))
        {
            warmth = armor->warmth;
            // Wool items do not lose their warmth in the rain
            if (!worn[i].made_of("wool"))
            {
                warmth *= 1.0 - (float)bodywetness / 100.0;
            }
            ret += warmth;
        }
    }
    return ret;
}

int player::encumb(body_part bp)
{
    int iArmorEnc = 0;
    double iLayers = 0;
    return encumb(bp, iLayers, iArmorEnc);
}


/*
 * Encumbrance logic:
 * Some clothing is intrinsically encumbering, such as heavy jackets, backpacks, body armor, etc.
 * These simply add their encumbrance value to each body part they cover.
 * In addition, each article of clothing after the first in a layer imposes an additional penalty.
 * e.g. one shirt will not encumber you, but two is tight and starts to restrict movement.
 * Clothes on seperate layers don't interact, so if you wear e.g. a light jacket over a shirt,
 * they're intended to be worn that way, and don't impose a penalty.
 * The default is to assume that clothes do not fit, clothes that are "fitted" either
 * reduce the encumbrance penalty by one, or if that is already 0, they reduce the layering effect.
 *
 * Use cases:
 * What would typically be considered normal "street clothes" should not be considered encumbering.
 * Tshirt, shirt, jacket on torso/arms, underwear and pants on legs, socks and shoes on feet.
 * This is currently handled by each of these articles of clothing
 * being on a different layer and/or body part, therefore accumulating no encumbrance.
 */
int player::encumb(body_part bp, double &layers, int &armorenc)
{
    int ret = 0;
    double layer[MAX_CLOTHING_LAYER] = { };
    int level = 0;

    it_armor* armor = NULL;
    for (size_t i = 0; i < worn.size(); ++i) {
        if( !worn[i].is_armor() ) {
            debugmsg("%s::encumb hit a non-armor item at worn[%d] (%s)", name.c_str(),
                     i, worn[i].tname().c_str());
        }
        armor = dynamic_cast<it_armor*>(worn[i].type);

        if( armor->covers & mfb(bp) ) {
            if( worn[i].has_flag( "SKINTIGHT" ) ) {
                level = UNDERWEAR;
            } else if ( worn[i].has_flag( "OUTER" ) ) {
                level = OUTER_LAYER;
            } else if ( worn[i].has_flag( "BELTED") ) {
                level = BELTED_LAYER;
            } else {
                level = REGULAR_LAYER;
            }

            layer[level]++;
            if( armor->is_power_armor() &&
                (has_active_item("UPS_on") || has_active_item("adv_UPS_on") ||
                 has_active_bionic("bio_power_armor_interface") ||
                 has_active_bionic("bio_power_armor_interface_mkII")) ) {
                armorenc += std::max( 0, armor->encumber - 4);
            } else {
                armorenc += armor->encumber;
                // Fitted clothes will reduce either encumbrance or layering.
                if( worn[i].has_flag( "FIT" ) ) {
                    if( armor->encumber > 0 && armorenc > 0 ) {
                        armorenc--;
                    } else if (layer[level] > 0) {
                        layer[level] -= .5;
                    }
                }
            }
        }
    }
    if (armorenc < 0) {
        armorenc = 0;
    }
    ret += armorenc;

    for (int i = 0; i < sizeof(layer) / sizeof(layer[0]); ++i) {
        layers += std::max( 0.0, layer[i] - 1.0 );
    }

    if (layers > 0.0) {
        ret += layers;
    }

    if (volume_carried() > volume_capacity() - 2 && bp != bp_head) {
        ret += 3;
    }

    // Bionics and mutation
    if ( has_bionic("bio_stiff") && bp != bp_head && bp != bp_mouth && bp != bp_eyes ) {
        ret += 1;
    }
    if ( has_trait("CHITIN3") && bp != bp_eyes && bp != bp_mouth ) {
        ret += 1;
    }
    if ( has_trait("SLIT_NOSTRILS") && bp == bp_mouth ) {
        ret += 1;
    }
    if ( has_trait("ARM_FEATHERS") && bp == bp_arms ) {
        ret += 2;
    }
    if ( has_trait("INSECT_ARMS") && bp == bp_arms ) {
        ret += 3;
    }
    if ( has_trait("ARACHNID_ARMS") && bp == bp_arms ) {
        ret += 4;
    }
    if ( has_trait("PAWS") && bp == bp_hands ) {
        ret += 1;
    }
    if ( has_trait("PAWS_LARGE") && bp == bp_hands ) {
        ret += 2;
    }
    if ( has_trait("LARGE") && (bp == bp_arms || bp == bp_torso )) {
        ret += 1;
    }
    if ( has_trait("WINGS_BUTTERFLY") && (bp == bp_torso )) {
        ret += 1;
    }
    if (bp == bp_hands &&
        (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
         has_trait("ARM_TENTACLES_8")) ) {
        ret += 3;
    }
    if (bp == bp_hands &&
        (has_trait("CLAWS_TENTACLE") )) {
        ret += 2;
    }
    if (bp == bp_mouth &&
        ( has_bionic("bio_nostril") ) ) {
        ret += 1;
    }
    if (bp == bp_hands &&
        ( has_bionic("bio_thumbs") ) ) {
        ret += 2;
    }
    if (bp == bp_eyes &&
        ( has_bionic("bio_pokedeye") ) ) {
        ret += 1;
    }
    if ( ret < 0 ) {
        ret = 0;
    }
    return ret;
}

int player::get_armor_bash(body_part bp)
{
    return get_armor_bash_base(bp) + armor_bash_bonus;
}

int player::get_armor_cut(body_part bp)
{
    return get_armor_cut_base(bp) + armor_cut_bonus;
}

int player::get_armor_bash_base(body_part bp)
{
 int ret = 0;
 it_armor* armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  if (armor->covers & mfb(bp))
   ret += worn[i].bash_resist();
 }
 if (has_bionic("bio_carbon")) {
    ret += 2;
 }
 if (bp == bp_head && has_bionic("bio_armor_head")) {
    ret += 3;
 }
 if (bp == bp_arms && has_bionic("bio_armor_arms")) {
    ret += 3;
 }
 if (bp == bp_torso && has_bionic("bio_armor_torso")) {
    ret += 3;
 }
 if (bp == bp_legs && has_bionic("bio_armor_legs")) {
    ret += 3;
 }
 if (bp == bp_eyes && has_bionic("bio_armor_eyes")) {
    ret += 3;
 }
 if (has_trait("FUR") || has_trait("LUPINE_FUR") || has_trait("URSINE_FUR")) {
    ret++;
 }
 if (bp == bp_head && has_trait("LYNX_FUR")) {
    ret++;
 }
 if (has_trait("FAT")) {
    ret ++;
 }
 if (has_trait("CHITIN")) {
    ret += 2;
 }
 if (has_trait("SHELL") && bp == bp_torso) {
    ret += 6;
 }
 ret += rng(0, disease_intensity("armor_boost"));
 return ret;
}

int player::get_armor_cut_base(body_part bp)
{
 int ret = 0;
 it_armor* armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  if (armor->covers & mfb(bp))
   ret += worn[i].cut_resist();
 }
 if (has_bionic("bio_carbon"))
  ret += 4;
 if (bp == bp_head && has_bionic("bio_armor_head"))
  ret += 3;
 else if (bp == bp_arms && has_bionic("bio_armor_arms"))
  ret += 3;
 else if (bp == bp_torso && has_bionic("bio_armor_torso"))
  ret += 3;
 else if (bp == bp_legs && has_bionic("bio_armor_legs"))
  ret += 3;
 else if (bp == bp_eyes && has_bionic("bio_armor_eyes"))
  ret += 3;
 if (has_trait("THICKSKIN"))
  ret++;
 if (has_trait("THINSKIN"))
  ret--;
 if (has_trait("SCALES"))
  ret += 2;
 if (has_trait("THICK_SCALES"))
  ret += 4;
 if (has_trait("SLEEK_SCALES"))
  ret += 1;
 if (has_trait("CHITIN"))
  ret += 2;
 if (has_trait("CHITIN2"))
  ret += 4;
 if (has_trait("CHITIN3"))
  ret += 8;
 if (has_trait("SHELL") && bp == bp_torso)
  ret += 14;
 ret += rng(0, disease_intensity("armor_boost"));
 return ret;
}

void get_armor_on(player* p, body_part bp, std::vector<int>& armor_indices) {
    it_armor* tmp;
    for (int i=0; i<p->worn.size(); i++) {
        tmp = dynamic_cast<it_armor*>(p->worn[i].type);
        if (tmp->covers & mfb(bp)) {
            armor_indices.push_back(i);
        }
    }
}

// mutates du, returns true if armor was damaged
bool player::armor_absorb(damage_unit& du, item& armor) {
    it_armor* armor_type = dynamic_cast<it_armor*>(armor.type);

    float mitigation = 0; // total amount of damage mitigated
    float effective_resist = resistances(armor).get_effective_resist(du);
    bool armor_damaged = false;

    std::string pre_damage_name = armor.tname();
    std::string pre_damage_adj = armor_type->dmg_adj(armor.damage);

    if (rng(0,100) < armor_type->coverage) {
        if (armor_type->is_power_armor()) { // TODO: add some check for power armor
        }

        mitigation = std::min(effective_resist, du.amount);
        du.amount -= mitigation; // mitigate the damage first

        // if the post-mitigation amount is greater than the amount
        if ((du.amount > effective_resist && !one_in(du.amount) && one_in(2)) ||
                // or if it isn't, but 1/50 chance
                (du.amount <= effective_resist && !armor.has_flag("STURDY")
                && !armor_type->is_power_armor() && one_in(200))) {
            armor_damaged = true;
            armor.damage++;
            std::string damage_verb = du.type == DT_BASH
                ? armor_type->bash_dmg_verb()
                : armor_type->cut_dmg_verb();

            // add "further" if the damage adjective and verb are the same
            std::string format_string = pre_damage_adj == damage_verb
                ? _("Your %s is %s further!")
                : _("Your %s is %s!");
            add_msg_if_player( m_bad, format_string.c_str(), pre_damage_name.c_str(),
                                      damage_verb.c_str());
            //item is damaged
            if( is_player() ) {
                SCT.add(xpos(), ypos(),
                    NORTH,
                    pre_damage_name, m_neutral,
                    damage_verb, m_info);
            }
        }
    }
    return armor_damaged;
}
void player::absorb_hit(body_part bp, int, damage_instance &dam) {
    for (std::vector<damage_unit>::iterator it = dam.damage_units.begin();
            it != dam.damage_units.end(); ++it) {

        // Recompute the armor indices for every damage unit because we may have
        // destroyed armor earlier in the loop.
        std::vector<int> armor_indices;

        get_armor_on(this,bp,armor_indices);

        // CBMs absorb damage first before hitting armour
        if (has_active_bionic("bio_ads")) {
            if (it->amount > 0 && power_level > 1) {
                if (it->type == DT_BASH)
                    it->amount -= rng(1, 8);
                else if (it->type == DT_CUT)
                    it->amount -= rng(1, 4);
                else if (it->type == DT_STAB)
                    it->amount -= rng(1, 2);
                power_level--;
            }
            if (it->amount < 0) it->amount = 0;
        }

        // The worn vector has the innermost item first, so
        // iterate reverse to damage the outermost (last in worn vector) first.
        for (std::vector<int>::reverse_iterator armor_it = armor_indices.rbegin();
                armor_it != armor_indices.rend(); ++armor_it) {

            const int index = *armor_it;

            armor_absorb(*it, worn[index]);

            // now check if armour was completely destroyed and display relevant messages
            // TODO: use something less janky than the old code for this check
            if (worn[index].damage >= 5) {
                //~ %s is armor name
                add_memorial_log(pgettext("memorial_male", "Worn %s was completely destroyed."),
                                 pgettext("memorial_female", "Worn %s was completely destroyed."),
                                 worn[index].tname().c_str());
                add_msg_player_or_npc( m_bad, _("Your %s is completely destroyed!"),
                                              _("<npcname>'s %s is completely destroyed!"),
                                              worn[index].tname().c_str() );
                worn.erase(worn.begin() + index);
            }
        }
    }
}


void player::absorb(body_part bp, int &dam, int &cut)
{
    it_armor* tmp;
    int arm_bash = 0, arm_cut = 0;
    bool cut_through = true;      // to determine if cutting damage penetrates multiple layers of armour
    int bash_absorb = 0;      // to determine if lower layers of armour get damaged

    // CBMS absorb damage first before hitting armour
    if (has_active_bionic("bio_ads"))
    {
        if (dam > 0 && power_level > 1)
        {
            dam -= rng(1, 8);
            power_level--;
        }
        if (cut > 0 && power_level > 1)
        {
            cut -= rng(0, 4);
            power_level--;
        }
        if (dam < 0)
            dam = 0;
        if (cut < 0)
            cut = 0;
    }

    // determines how much damage is absorbed by armour
    // zero if damage misses a covered part
    int bash_reduction = 0;
    int cut_reduction = 0;

    // See, we do it backwards, iterating inwards
    for (int i = worn.size() - 1; i >= 0; i--)
    {
        tmp = dynamic_cast<it_armor*>(worn[i].type);
        if (tmp->covers & mfb(bp))
        {
            // first determine if damage is at a covered part of the body
            // probability given by coverage
            if (rng(0, 100) < tmp->coverage)
            {
                // hit a covered part of the body, so now determine if armour is damaged
                arm_bash = worn[i].bash_resist();
                arm_cut  = worn[i].cut_resist();
                // also determine how much damage is absorbed by armour
                // factor of 3 to normalise for material hardness values
                bash_reduction = arm_bash / 3;
                cut_reduction = arm_cut / 3;

                // power armour first  - to depreciate eventually
                if (worn[i].type->is_power_armor())
                {
                    if (cut > arm_cut * 2 || dam > arm_bash * 2)
                    {
                        add_msg_if_player(m_bad, _("Your %s is damaged!"), worn[i].tname().c_str());
                        worn[i].damage++;
                    }
                }
                else // normal armour
                {
                    // determine how much the damage exceeds the armour absorption
                    // bash damage takes into account preceding layers
                    int diff_bash = (dam - arm_bash - bash_absorb < 0) ? -1 : (dam - arm_bash);
                    int diff_cut  = (cut - arm_cut  < 0) ? -1 : (cut - arm_cut);
                    bool armor_damaged = false;
                    std::string pre_damage_name = worn[i].tname();

                    // armour damage occurs only if damage exceeds armour absorption
                    // plus a luck factor, even if damage is below armour absorption (2% chance)
                    if ((dam > arm_bash && !one_in(diff_bash)) ||
                        (!worn[i].has_flag ("STURDY") && diff_bash == -1 && one_in(50)))
                    {
                        armor_damaged = true;
                        worn[i].damage++;
                    }
                    bash_absorb += arm_bash;

                    // cut damage falls through to inner layers only if preceding layer was damaged
                    if (cut_through)
                    {
                        if ((cut > arm_cut && !one_in(diff_cut)) ||
                            (!worn[i].has_flag ("STURDY") && diff_cut == -1 && one_in(50)))
                        {
                            armor_damaged = true;
                            worn[i].damage++;
                        }
                        else // layer of clothing was not damaged, so stop cutting damage from penetrating
                        {
                            cut_through = false;
                        }
                    }

                    // now check if armour was completely destroyed and display relevant messages
                    if (worn[i].damage >= 5)
                    {
                      //~ %s is armor name
                      add_memorial_log(pgettext("memorial_male", "Worn %s was completely destroyed."),
                                       pgettext("memorial_female", "Worn %s was completely destroyed."),
                                       worn[i].tname().c_str());
                        add_msg_player_or_npc(m_bad, _("Your %s is completely destroyed!"),
                                                     _("<npcname>'s %s is completely destroyed!"),
                                                     worn[i].tname().c_str() );
                        worn.erase(worn.begin() + i);
                    } else if (armor_damaged) {
                        std::string damage_verb = diff_bash > diff_cut ? tmp->bash_dmg_verb() :
                                                                         tmp->cut_dmg_verb();
                        add_msg_if_player( m_bad, _("Your %s is %s!"), pre_damage_name.c_str(),
                                                  damage_verb.c_str());
                    }
                } // end of armour damage code
            }
        }
        // reduce damage accordingly
        dam -= bash_reduction;
        cut -= cut_reduction;
    }
    // now account for CBMs and mutations
    if (has_bionic("bio_carbon"))
    {
        dam -= 2;
        cut -= 4;
    }
    if (bp == bp_head && has_bionic("bio_armor_head"))
    {
        dam -= 3;
        cut -= 3;
    }
    else if (bp == bp_arms && has_bionic("bio_armor_arms"))
    {
        dam -= 3;
        cut -= 3;
    }
    else if (bp == bp_torso && has_bionic("bio_armor_torso"))
    {
        dam -= 3;
        cut -= 3;
    }
    else if (bp == bp_legs && has_bionic("bio_armor_legs"))
    {
        dam -= 3;
        cut -= 3;
    }
    else if (bp == bp_eyes && has_bionic("bio_armor_eyes"))
    {
        dam -= 3;
        cut -= 3;
    }
    if (has_trait("THICKSKIN")) {
        cut--;
    }
    if (has_trait("THINSKIN")) {
        cut++;
    }
    if (has_trait("SCALES")) {
        cut -= 2;
    }
    if (has_trait("THICK_SCALES")) {
        cut -= 4;
    }
    if (has_trait("SLEEK_SCALES")) {
        cut -= 1;
    }
    if (has_trait("FEATHERS")) {
        dam--;
    }
    if (has_trait("AMORPHOUS")) {
        dam--;
        if (!(has_trait("INT_SLIME"))) {
            dam -= 3;
        }
    }
    if (bp == bp_arms && has_trait("ARM_FEATHERS")) {
        dam--;
    }
    if (has_trait("FUR") || has_trait("LUPINE_FUR") || has_trait("URSINE_FUR")) {
        dam--;
    }
    if (bp == bp_head && has_trait("LYNX_FUR")) {
        dam--;
    }
    if (has_trait("FAT")) {
        cut --;
    }
    if (has_trait("CHITIN")) {
        cut -= 2;
    }
    if (has_trait("CHITIN2")) {
        dam--;
        cut -= 4;
    }
    if (has_trait("CHITIN3")) {
        dam -= 2;
        cut -= 8;
    }
    if (has_trait("PLANTSKIN")) {
        dam--;
    }
    if (has_trait("BARK")) {
        dam -= 2;
    }
    if (bp == bp_feet && has_trait("HOOVES")) {
        cut--;
    }
    if (has_trait("LIGHT_BONES")) {
        dam *= 1.4;
    }
    if (has_trait("HOLLOW_BONES")) {
        dam *= 1.8;
    }

    // apply martial arts armor buffs
    dam -= mabuff_arm_bash_bonus();
    cut -= mabuff_arm_cut_bonus();

    if (dam < 0)
        dam = 0;
    if (cut < 0)
        cut = 0;
}

int player::get_env_resist(body_part bp)
{
    int ret = 0;
    for (int i = 0; i < worn.size(); i++) {
        if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp) ||
             (bp == bp_eyes && // Head protection works on eyes too (e.g. baseball cap)
             (dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_head))) {
            ret += (dynamic_cast<it_armor*>(worn[i].type))->env_resist;
        }
    }

    if (bp == bp_mouth && has_bionic("bio_purifier") && ret < 5) {
        ret += 2;
        if (ret > 5) {
            ret = 5;
        }
    }

    if (bp == bp_eyes && has_bionic("bio_armor_eyes") && ret < 5) {
        ret += 2;
        if (ret > 5) {
            ret = 5;
        }
    }
    return ret;
}

bool player::wearing_something_on(body_part bp) const
{
    for (int i = 0; i < worn.size(); i++) {
        if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp))
            return true;
    }
    return false;
}

bool player::is_wearing_shoes() const
{
    for (int i = 0; i < worn.size(); i++) {
        const item *worn_item = &worn[i];
        const it_armor *worn_armor = dynamic_cast<it_armor*>(worn_item->type);

        if (worn_armor->covers & mfb(bp_feet) &&
            (!worn_item->has_flag("BELTED")) &&
            (!worn_item->has_flag("SKINTIGHT"))) {
            return true;
        }
    }
    return false;
}

bool player::is_wearing_power_armor(bool *hasHelmet) const {
    bool result = false;
    for (size_t i = 0; i < worn.size(); i++) {
        it_armor *armor = dynamic_cast<it_armor*>(worn[i].type);
        if (armor == NULL || !armor->is_power_armor()) {
            continue;
        }
        if (hasHelmet == NULL) {
            // found power armor, helmet not requested, cancel loop
            return true;
        }
        // found power armor, continue search for helmet
        result = true;
        if (armor->covers & mfb(bp_head)) {
            *hasHelmet = true;
            return true;
        }
    }
    return result;
}

int player::adjust_for_focus(int amount)
{
    int effective_focus = focus_pool;
    if (has_trait("FASTLEARNER"))
    {
        effective_focus += 15;
    }
    if (has_trait("SLOWLEARNER"))
    {
        effective_focus -= 15;
    }
    double tmp = amount * (effective_focus / 100.0);
    int ret = int(tmp);
    if (rng(0, 100) < 100 * (tmp - ret))
    {
        ret++;
    }
    return ret;
}

void player::practice( Skill *s, int amount, int cap )
{
    SkillLevel& level = skillLevel(s);
    // Double amount, but only if level.exercise isn't a small negative number?
    if (level.exercise() < 0) {
        if (amount >= -level.exercise()) {
            amount -= level.exercise();
        } else {
            amount += amount;
        }
    }

    bool isSavant = has_trait("SAVANT");

    Skill *savantSkill = NULL;
    SkillLevel savantSkillLevel = SkillLevel();

    if (isSavant) {
        for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
             aSkill != Skill::skills.end(); ++aSkill) {
            if (skillLevel(*aSkill) > savantSkillLevel) {
                savantSkill = *aSkill;
                savantSkillLevel = skillLevel(*aSkill);
            }
        }
    }

    amount = adjust_for_focus(amount);

    if (has_trait("PACIFIST") && s->is_combat_skill()) {
        if(!one_in(3)) {
          amount = 0;
        }
    }
    if (has_trait("PRED2") && s->is_combat_skill()) {
        if(one_in(3)) {
          amount *= 2;
        }
    }
    if (has_trait("PRED3") && s->is_combat_skill()) {
        amount *= 2;
    }

    if (has_trait("PRED4") && s->is_combat_skill()) {
        amount *= 3;
    }

    if (isSavant && s != savantSkill) {
        amount /= 2;
    }

    if (skillLevel(s) > cap) { //blunt grinding cap implementation for crafting
        amount = 0;
        int curLevel = skillLevel(s);
        if(is_player() && one_in(5)) {//remind the player intermittently that no skill gain takes place
            add_msg(m_info, _("This task is too simple to train your %s beyond %d."),
                    s->name().c_str(), curLevel);
        }
    }

    if (amount > 0 && level.isTraining()) {
        int oldLevel = skillLevel(s);
        skillLevel(s).train(amount);
        int newLevel = skillLevel(s);
        if (is_player() && newLevel > oldLevel) {
            add_msg(m_good, _("Your skill in %s has increased to %d!"), s->name().c_str(), newLevel);
        }
        if(is_player() && newLevel > cap) {
            //inform player immediately that the current recipe can't be used to train further
            add_msg(m_info, _("You feel that %s tasks of this level are becoming trivial."),
                    s->name().c_str());
        }

        int chance_to_drop = focus_pool;
        focus_pool -= chance_to_drop / 100;
        // Apex Predators don't think about much other than killing.
        // They don't lose Focus when practicing combat skills.
        if ((rng(1, 100) <= (chance_to_drop % 100)) && (!(has_trait("PRED4") &&
                                                          s->is_combat_skill()))) {
            focus_pool--;
        }
    }

    skillLevel(s).practice();
}

void player::practice( std::string s, int amount, int cap )
{
    Skill *aSkill = Skill::skill(s);
    practice( aSkill, amount, cap );
}

bool player::knows_recipe(const recipe *rec) const
{
    // do we know the recipe by virtue of it being autolearned?
    if( rec->autolearn ) {
        // Can the skill being trained can handle the difficulty of the task
        bool meets_requirements = false;
        if(rec->skill_used == NULL || get_skill_level(rec->skill_used) >= rec->difficulty){
            meets_requirements = true;
            //If there are required skills, insure their requirements are met, or we can't craft
            if(!rec->required_skills.empty()){
                for( auto iter = rec->required_skills.cbegin();
                     iter != rec->required_skills.cend(); ++iter ){
                    if( get_skill_level(iter->first) < iter->second ){
                        meets_requirements = false;
                    }
                }
            }
        }
        if(meets_requirements){
            return true;
        }
    }

    if( learned_recipes.find( rec->ident ) != learned_recipes.end() ) {
        return true;
    }

    return false;
}

int player::has_recipe( const recipe *r, const inventory &crafting_inv ) const
{
    // Iterate over the nearby items and see if there's a book that has the recipe.
    const_invslice slice = crafting_inv.const_slice();
    int difficulty = -1;
    for( auto stack = slice.cbegin(); stack != slice.cend(); ++stack ) {
        // We are only checking qualities, so we only care about the first item in the stack.
        const item &candidate = (*stack)->front();
        if( candidate.is_book() && items_identified.count(candidate.type->id) ) {
            it_book *book_type = dynamic_cast<it_book *>(candidate.type);
            for( auto book_recipe = book_type->recipes.cbegin();
                 book_recipe != book_type->recipes.cend(); ++book_recipe ) {
                // Does it have the recipe, and do we meet it's requirements?
                if( book_recipe->first->ident == r->ident &&
                    ( book_recipe->first->skill_used == NULL ||
                      get_skill_level(book_recipe->first->skill_used) >= book_recipe->second ) &&
                    ( difficulty == -1 || book_recipe->second < difficulty ) ) {
                    difficulty = book_recipe->second;
                }
            }
        }
    }
    return difficulty;
}

void player::learn_recipe(recipe *rec)
{
    learned_recipes[rec->ident] = rec;
}

void player::assign_activity(activity_type type, int moves, int index, int pos, std::string name)
{
    if (backlog.type == type && backlog.index == index && backlog.position == pos &&
        backlog.name == name) {
        add_msg_if_player( _("You resume your task."));
        activity = backlog;
        backlog = player_activity();
    } else {
        activity = player_activity(type, moves, index, pos, name);
    }
    if (this->moves <= activity.moves_left) {
        activity.moves_left -= this->moves;
    }
    this->moves = 0;
    activity.warned_of_proximity = false;
}

bool player::has_activity(const activity_type type)
{
    if (activity.type == type) {
        return true;
    }

    return false;
}

void player::cancel_activity()
{
    if (activity.is_suspendable()) {
        backlog = activity;
    }
    activity.type = ACT_NULL;
}

std::vector<item*> player::has_ammo(ammotype at)
{
    std::vector<item*> result = inv.all_ammo(at);
    if (weapon.is_of_ammo_type_or_contains_it(at)) {
        result.push_back(&weapon);
    }
    for (size_t i = 0; i < worn.size(); i++) {
        if (worn[i].is_of_ammo_type_or_contains_it(at)) {
            result.push_back(&worn[i]);
        }
    }
    return result;
}

std::string player::weapname(bool charges)
{
 if (!(weapon.is_tool() &&
       dynamic_cast<it_tool*>(weapon.type)->max_charges <= 0) &&
     weapon.charges >= 0 && charges) {
  std::stringstream dump;
  int spare_mag = weapon.has_gunmod("spare_mag");
  dump << weapon.tname().c_str();
  if (!(weapon.has_flag("NO_AMMO") || weapon.has_flag("RELOAD_AND_SHOOT"))) {
   dump << " (" << weapon.charges;
   if( -1 != spare_mag )
   dump << "+" << weapon.contents[spare_mag].charges;
   for (int i = 0; i < weapon.contents.size(); i++)
   if (weapon.contents[i].is_gunmod() &&
     weapon.contents[i].has_flag("MODE_AUX"))
    dump << "+" << weapon.contents[i].charges;
   dump << ")";
  }
  return dump.str();
 } else if (weapon.is_container()) {
  std::stringstream dump;
  dump << weapon.tname().c_str();
  if(weapon.contents.size() == 1) {
   dump << " (" << weapon.contents[0].charges << ")";
  }
  return dump.str();
 } else if (weapon.is_null()) {
  return _("fists");
 } else
  return weapon.tname();
}

void player::wield_contents(item *container, bool force_invlet,
                            std::string /*skill_used*/, int /*volume_factor*/)
{
    if(!(container->contents.empty())) {
        item& weap = container->contents[0];
        inv.assign_empty_invlet(weap, force_invlet);
        wield(&(i_add(weap)));
        container->contents.erase(container->contents.begin());
    } else {
        debugmsg("Tried to wield contents of empty container (player::wield_contents)");
    }
}

void player::store(item* container, item* put, std::string skill_used, int volume_factor)
{
    int lvl = skillLevel(skill_used);
    moves -= (lvl == 0) ? ((volume_factor + 1) * put->volume()) : (volume_factor * put->volume()) / lvl;
    container->put_in(i_rem(put));
}

nc_color encumb_color(int level)
{
 if (level < 0)
  return c_green;
 if (level == 0)
  return c_ltgray;
 if (level < 4)
  return c_yellow;
 if (level < 7)
  return c_ltred;
 return c_red;
}

SkillLevel& player::skillLevel(std::string ident)
{
    return _skills[Skill::skill(ident)];
}

SkillLevel& player::skillLevel(Skill *_skill)
{
    return _skills[_skill];
}

SkillLevel player::get_skill_level(Skill *_skill) const
{
    for (std::map<Skill*,SkillLevel>::const_iterator it = _skills.begin();
            it != _skills.end(); ++it) {
        if (it->first == _skill) {
            return it->second;
        }
    }
    return SkillLevel();
}

SkillLevel player::get_skill_level(const std::string &ident) const
{
    Skill *sk = Skill::skill(ident);
    return get_skill_level(sk);
}

void player::copy_skill_levels(const player *rhs)
{
    _skills = rhs->_skills;
}

void player::set_skill_level(Skill* _skill, int level)
{
    skillLevel(_skill).level(level);
}

void player::set_skill_level(std::string ident, int level)
{
    skillLevel(ident).level(level);
}

void player::boost_skill_level(Skill* _skill, int level)
{
    skillLevel(_skill).level(level+skillLevel(_skill));
}

void player::boost_skill_level(std::string ident, int level)
{
    skillLevel(ident).level(level+skillLevel(ident));
}

void player::setID (int i)
{
    this->id = i;
}

int player::getID () const
{
    return this->id;
}

bool player::uncanny_dodge(bool is_u)
{
    if( this->power_level < 3 || !this->has_active_bionic("bio_uncanny_dodge") ) { return false; }
    point adjacent = adjacent_tile();
    power_level -= 3;
    if (adjacent.x != posx || adjacent.y != posy)
    {
        posx = adjacent.x;
        posy = adjacent.y;
        if (is_u)
            add_msg(_("Time seems to slow down and you instinctively dodge!"));
        else
            add_msg(_("Your target dodges... so fast!"));
        return true;
    }
    if (is_u)
        add_msg(_("You try to dodge but there's no room!"));
    return false;
}

// adjacent_tile() returns a safe, unoccupied adjacent tile. If there are no such tiles, returns player position instead.
point player::adjacent_tile()
{
    std::vector<point> ret;
    field_entry *cur = NULL;
    field tmpfld;
    trap_id curtrap;
    int dangerous_fields;
    for (int i=posx-1; i <= posx+1; i++)
    {
        for (int j=posy-1; j <= posy+1; j++)
        {
            if (i == posx && j == posy) continue;       // don't consider player position
            curtrap=g->m.tr_at(i, j);
            if (g->mon_at(i, j) == -1 && g->npc_at(i, j) == -1 && g->m.move_cost(i, j) > 0 && (curtrap == tr_null || traplist[curtrap]->is_benign()))        // only consider tile if unoccupied, passable and has no traps
            {
                dangerous_fields = 0;
                tmpfld = g->m.field_at(i, j);
                for(std::map<field_id, field_entry*>::iterator field_list_it = tmpfld.getFieldStart(); field_list_it != tmpfld.getFieldEnd(); ++field_list_it)
                {
                    cur = field_list_it->second;
                    if (cur != NULL && cur->is_dangerous())
                        dangerous_fields++;
                }
                if (dangerous_fields == 0)
                {
                    ret.push_back(point(i, j));
                }
            }
        }
    }
    if (ret.size())
        return ret[rng(0, ret.size()-1)];   // return a random valid adjacent tile
    else
        return point(posx, posy);           // or return player position if no valid adjacent tiles
}

// --- Library functions ---
// This stuff could be moved elsewhere, but there
// doesn't seem to be a good place to put it right now.

// Basic logistic function.
double player::logistic(double t)
{
    return 1 / (1 + exp(-t));
}

// Logistic curve [-6,6], flipped and scaled to
// range from 1 to 0 as pos goes from min to max.
double player::logistic_range(int min, int max, int pos)
{
    const double LOGI_CUTOFF = 4;
    const double LOGI_MIN = logistic(-LOGI_CUTOFF);
    const double LOGI_MAX = logistic(+LOGI_CUTOFF);
    const double LOGI_RANGE = LOGI_MAX - LOGI_MIN;
    // Anything beyond [min,max] gets clamped.
    if (pos < min)
    {
        return 1.0;
    }
    else if (pos > max)
    {
        return 0.0;
    }

    // Normalize the pos to [0,1]
    double range = max - min;
    double unit_pos = (pos - min) / range;

    // Scale and flip it to [+LOGI_CUTOFF,-LOGI_CUTOFF]
    double scaled_pos = LOGI_CUTOFF - 2 * LOGI_CUTOFF * unit_pos;

    // Get the raw logistic value.
    double raw_logistic = logistic(scaled_pos);

    // Scale the output to [0,1]
    return (raw_logistic - LOGI_MIN) / LOGI_RANGE;
}

// Calculates portions favoring x, then y, then z
void player::calculate_portions(int &x, int &y, int &z, int maximum)
{
    z = std::min(z, std::max(maximum - x - y, 0));
    y = std::min(y, std::max(maximum - x , 0));
    x = std::min(x, std::max(maximum, 0));
}

void player::environmental_revert_effect()
{
    illness.clear();
    addictions.clear();
    morale.clear();

    for (int part = 0; part < num_hp_parts; part++) {
        hp_cur[part] = hp_max[part];
    }
    hunger = 0;
    thirst = 0;
    fatigue = 0;
    health = 0;
    stim = 0;
    pain = 0;
    pkill = 0;
    radiation = 0;

    recalc_sight_limits();
}

bool player::is_invisible() const {
    static const std::string str_bio_cloak("bio_cloak"); // This function used in monster::plan_moves
    static const std::string str_bio_night("bio_night");
    return (
        has_active_bionic(str_bio_cloak) ||
        has_active_bionic(str_bio_night) ||
        has_active_optcloak() ||
        has_trait("DEBUG_CLOAK") ||
        has_artifact_with(AEP_INVISIBLE)
    );
}

int player::visibility( bool, int ) const { // 0-100 %
    if ( is_invisible() ) {
        return 0;
    }
    // todo:
    // if ( dark_clothing() && light check ...
    return 100;
}

void player::set_destination(const std::vector<point> &route)
{
    auto_move_route = route;
}

void player::clear_destination()
{
    auto_move_route.clear();
    next_expected_position.x = -1;
    next_expected_position.y = -1;
}

bool player::has_destination() const
{
    return !auto_move_route.empty();
}

std::vector<point> &player::get_auto_move_route()
{
    return auto_move_route;
}

action_id player::get_next_auto_move_direction()
{
    if (!has_destination()) {
        return ACTION_NULL;
    }

    if (next_expected_position.x != -1) {
        if (posx != next_expected_position.x || posy != next_expected_position.y) {
            // We're off course, possibly stumbling or stuck, cancel auto move
            return ACTION_NULL;
        }
    }

    next_expected_position = auto_move_route.front();
    auto_move_route.erase(auto_move_route.begin());

    int dx = next_expected_position.x - posx;
    int dy = next_expected_position.y - posy;

    if (abs(dx) > 1 || abs(dy) > 1) {
        // Should never happen, but check just in case
        return ACTION_NULL;
    }

    return get_movement_direction_from_delta(dx, dy);
}

void player::shift_destination(int shiftx, int shifty)
{
    if (next_expected_position.x != -1) {
        next_expected_position.x += shiftx;
        next_expected_position.y += shifty;
    }

    for (std::vector<point>::iterator it = auto_move_route.begin(); it != auto_move_route.end(); it++) {
        it->x += shiftx;
        it->y += shifty;
    }
}

bool player::has_weapon() {
    return !unarmed_attack();
}

m_size player::get_size() {
    return MS_MEDIUM;
}

int player::get_hp( hp_part bp )
{
    if( bp < num_hp_parts ) {
        return hp_cur[bp];
    }
    int hp_total = 0;
    for( int i = 0; i < num_hp_parts; ++i ) {
        hp_total += hp_cur[i];
    }
    return hp_total;
}

int player::get_hp_max( hp_part bp )
{
    if( bp < num_hp_parts ) {
        return hp_max[bp];
    }
    int hp_total = 0;
    for( int i = 0; i < num_hp_parts; ++i ) {
        hp_total += hp_max[i];
    }
    return hp_total;
}

field_id player::playerBloodType() {
    if (player::has_trait("THRESH_PLANT"))
        return fd_blood_veggy;
    if (player::has_trait("THRESH_INSECT") || player::has_trait("THRESH_SPIDER"))
        return fd_blood_insect;
    if (player::has_trait("THRESH_CEPHALOPOD"))
        return fd_blood_invertebrate;
    return fd_blood;
}

Creature *player::auto_find_hostile_target(int range, int &boo_hoo, int &fire_t)
{
    if (is_player()) {
        debugmsg("called player::auto_find_hostile_target for player themself!");
        return NULL;
    }
    int t;
    monster *target = NULL;
    const int iff_dist = 24; // iff check triggers at this distance
    int iff_hangle = 15; // iff safety margin (degrees). less accuracy, more paranoia
    int closest = range + 1;
    int u_angle = 0;         // player angle relative to turret
    boo_hoo = 0;         // how many targets were passed due to IFF. Tragically.
    bool iff_trig = false;   // player seen and within range of stray shots
    int pldist = rl_dist(posx, posy, g->u.posx, g->u.posy);
    if (pldist < iff_dist && g->sees_u(posx, posy, t)) {
        iff_trig = true;
        if (pldist < 3) {
            iff_hangle = (pldist == 2 ? 30 : 60);    // granularity increases with proximity
        }
        u_angle = g->m.coord_to_angle(posx, posy, g->u.posx, g->u.posy);
    }
    for (int i = 0; i < g->num_zombies(); i++) {
        monster *m = &g->zombie(i);
        if (m->friendly != 0) {
            // friendly to the player, not a target for us
            continue;
        }
        if (!sees(m, t)) {
            // can't see nor sense it
            continue;
        }
        int dist = rl_dist(posx, posy, m->posx(), m->posy());
        if (dist >= closest) {
            // Have a better target anyway, ignore this one.
            continue;
        }
        if (iff_trig) {
            int tangle = g->m.coord_to_angle(posx, posy, m->posx(), m->posy());
            int diff = abs(u_angle - tangle);
            if (diff + iff_hangle > 360 || diff < iff_hangle) {
                // Player is in the way
                boo_hoo++;
                continue;
            }
        }
        target = m;
        closest = dist;
        fire_t = t;
    }
    return target;
}

bool player::sees(int x, int y)
{
    int dummy = 0;
    return sees(x, y, dummy);
}

bool player::sees(int x, int y, int &t)
{
    const int s_range = sight_range(g->light_level());
    static const std::string str_bio_night("bio_night");
    const int wanted_range = rl_dist(posx, posy, x, y);

    if (wanted_range < clairvoyance()) {
        return true;
    }
    bool can_see = false;
    if (wanted_range <= s_range ||
        (wanted_range <= sight_range(DAYLIGHT_LEVEL) &&
            g->m.light_at(x, y) >= LL_LOW)) {
        if (is_player()) {
            // uses the seen cache in map
            can_see = g->m.pl_sees(posx, posy, x, y, wanted_range);
        } else if (g->m.light_at(x, y) >= LL_LOW) {
            can_see = g->m.sees(posx, posy, x, y, wanted_range, t);
        } else {
            can_see = g->m.sees(posx, posy, x, y, s_range, t);
        }
    }
    if (has_active_bionic(str_bio_night) && wanted_range < 15 && wanted_range > sight_range(1)) {
        return false;
    }
    return can_see;
}

bool player::sees(Creature *critter)
{
    int dummy = 0;
    return sees(critter, dummy);
}

bool player::sees(Creature *critter, int &t)
{
    if (!is_player() && critter->is_hallucination()) {
        // hallucinations are only visible for the player
        return false;
    }
    const int cx = critter->xpos();
    const int cy = critter->ypos();
    int dist = rl_dist(posx, posy, cx, cy);
    if (dist <= 3 && has_trait("ANTENNAE")) {
        return true;
    }
    if (dist > 1 && critter->digging() && !has_active_bionic("bio_ground_sonar")) {
        return false; // Can't see digging monsters until we're right next to them
    }
    if (g->m.is_divable(cx, cy) && critter->is_underwater() && !is_underwater()) {
        //Monster is in the water and submerged, and we're out of/above the water
        return false;
    }
    return sees(cx, cy, t);
}

bool player::can_pickup(bool print_msg) const
{
    if (weapon.has_flag("NO_PICKUP")) {
        if (print_msg && const_cast<player*>(this)->is_player()) {
            add_msg(m_info, _("You cannot pick up items with your %s!"), weapon.tname().c_str());
        }
        return false;
    }
    return true;
}

bool player::has_container_for(const item &newit)
{
    if (!newit.made_of(LIQUID)) {
        // Currently only liquids need a container
        return true;
    }
    int charges = newit.charges;
    LIQUID_FILL_ERROR tmperr;
    charges -= weapon.get_remaining_capacity_for_liquid(newit, tmperr);
    for (size_t i = 0; i < worn.size() && charges > 0; i++) {
        charges -= worn[i].get_remaining_capacity_for_liquid(newit, tmperr);
    }
    for (size_t i = 0; i < inv.size() && charges > 0; i++) {
        const std::list<item>&items = inv.const_stack(i);
        // Assume that each item in the stack has the same remaining capacity
        charges -= items.front().get_remaining_capacity_for_liquid(newit, tmperr) * items.size();
    }
    return charges <= 0;
}




nc_color player::bodytemp_color(int bp)
{
  nc_color color =  c_ltgray; // default
    if (bp == bp_eyes) {
        color = c_ltgray;    // Eyes don't count towards warmth
    } else if (temp_conv[bp] >  BODYTEMP_SCORCHING) {
        color = c_red;
    } else if (temp_conv[bp] >  BODYTEMP_VERY_HOT) {
        color = c_ltred;
    } else if (temp_conv[bp] >  BODYTEMP_HOT) {
        color = c_yellow;
    } else if (temp_conv[bp] >  BODYTEMP_COLD) {
        color = c_green;
    } else if (temp_conv[bp] >  BODYTEMP_VERY_COLD) {
        color = c_ltblue;
    } else if (temp_conv[bp] >  BODYTEMP_FREEZING) {
        color = c_cyan;
    } else if (temp_conv[bp] <= BODYTEMP_FREEZING) {
        color = c_blue;
    }
    return color;
}

//message related stuff
void player::add_msg_if_player(const char* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    Messages::vadd_msg(msg, ap);
    va_end(ap);
};
void player::add_msg_player_or_npc(const char* player_str, const char* npc_str, ...)
{
    va_list ap;
    va_start(ap, npc_str);
    Messages::vadd_msg(player_str, ap);
    va_end(ap);
};
void player::add_msg_if_player(game_message_type type, const char* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    Messages::vadd_msg(type, msg, ap);
    va_end(ap);
};
void player::add_msg_player_or_npc(game_message_type type, const char* player_str, const char* npc_str, ...)
{
    va_list ap;
    va_start(ap, npc_str);
    Messages::vadd_msg(type, player_str, ap);
    va_end(ap);
};

bool player::knows_trap(int x, int y) const
{
    x += g->levx * SEEX;
    y += g->levy * SEEY;
    tripoint p(x, y, g->levz);
    return known_traps.count(p) > 0;
}

void player::add_known_trap(int x, int y, const std::string &t)
{
    x += g->levx * SEEX;
    y += g->levy * SEEY;
    tripoint p(x, y, g->levz);
    if (t == "tr_null") {
        known_traps.erase(p);
    } else {
        known_traps[p] = t;
    }
}

bool player::is_deaf() const
{
    return has_effect("deaf") || worn_with_flag("DEAF");
}

bool player::is_suitable_weapon( const item &it ) const
{
    // if style is selected, always prefer compatible weapons
    if( get_combat_style().has_weapon( it.typeId() ) ) {
        return true;
    }
    // Assume all martial art styles can use any UNARMED_WEAPON item.
    // This includes the brawling style
    if( style_selected != "style_none" ) {
        return it.has_flag( "UNARMED_WEAPON" );
    }
    return false;
}

void player::place_corpse()
{
    std::vector<item *> tmp = inv_dump();
    item body;
    body.make_corpse( "corpse", GetMType( "mon_null" ), calendar::turn, name );
    for( auto itm : tmp ) {
        g->m.add_item_or_charges( posx, posy, *itm );
    }
    for( auto & bio : my_bionics ) {
        if( item_controller->has_template( bio.id ) ) {
            body.put_in( item( bio.id, calendar::turn ) );
        }
    }
    int pow = max_power_level;
    while( pow >= 4 ) {
        if( pow >= 10 ) {
            pow -= 10;
            body.contents.push_back( item( "bio_power_storage_mkII", calendar::turn ) );
        } else {
            pow -= 4;
            body.contents.push_back( item( "bio_power_storage", calendar::turn ) );
        }
    }
    g->m.add_item_or_charges( posx, posy, body );
}
