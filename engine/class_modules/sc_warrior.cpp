// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
//
// TODO:
//  Sooner:
//   * Recheck the fixmes.
//   * Measure and implement the formula for rage from damage taken
//     (n.b., based on percentage of HP for unmitigated hit size).
//   * Watch Raging Blow and see if Blizzard fix the bug where it's
//     not refunding 80% of the rage cost if it misses.
//   * Consider testing the rest of the abilities for that too.
//   * Sanity check init_buffs() wrt durations and chances.
//  Later:
//   * Verify that Colossus Smash itself doesn't ignore armor.
//   * Get Heroic Strike to trigger properly "off gcd" using priority.
//   * Move the bleeds out of being warrior_attack_t to stop them
//     triggering effects or having special cases in the class.
//   * Prot? O_O
//
// NOTES:
//  Damage increase types per spell:
//
//   * battle_stance                    = 21156 = mod_damage_done% (0x7f)
//   * berserker_stance                 =  7381 = mod_damage_done% (0x7f)
//                                      = 46857 = ???
//   * cruelty                          = 12582 = add_flat_mod_spell_crit_chance (7)
//   * enrage (fury)                    = 14202 = mod_damage_done% (0x1)
//   * enrage (wrecking_crew)           = 57519 = mod_damage_done% (0x1)
//   * heavy_repercussions              = 86896 = add_flat_mod_spell_effect2 (12)
//   * hold_the_line                    = 84620 = mod_crit% (127)
//   * improved_revenge                 = 12799 = add_percent_mod_generic
//   * incite                           = 50687 = add_flat_mod_spell_crit_chance (7)
//                                      = 86627 = add_flat_mod_spell_crit_chance (7)
//   * juggernaut                       = 64976 = add_flat_mod_spell_crit_chance (7)
//   * lambs_to_the_slaughter           = 84586 = add_percent_mod_generic
//   * meat_cleaver                     = 85739 = add_percent_mod_generic
//   * rampage                          = 29801 = mod_crit% (7)
//   * recklessness                     =  1719 = add_flat_mod (7)
//   * rude_interruption                = 86663 = mod_damage_done% (0x7f)
//   * singleminded_fury                = 81099 = mod_damage_done% (0x7f)
//   * sword_and_board                  = 46953 = add_flat_mod_spell_crit_chance (7)
//   * thunderstruck                    = 87096 = add_percent_mod_generic
//   * war_acacdemy                     = 84572 = add_percent_mod_generic
//   * twohanded_weapon_specialization  = 12712 = mod_damage_done% (0x1)
//   * unshackled_fury                  = 76856 = add_percent_mod_spell_effect1/2
//
//   * glyph_of_bloodthirst             = 58367 = add_percent_mod
//   * glyph_of_devastate               = 58388 = add_flat_mod_spell_crit_chance (7)
//   * glyph_of_mortal_strike           = 58368 = add_percent_mod_generic
//   * glyph_of_overpower               = 58386 = add_percent_mod_generic
//   * glyph_of_raging_blow             = 58370 = add_flat_mod_spell_crit_chance (7)
//   * glyph_of_revenge                 = 58364 = add_percent_mod_generic
//   * glyph_of_shield_slam             = 58375 = add_percent_mod_generic
//   * glyph_of_slam                    = 58385 = add_flat_mod_spell_crit_chance (7)
//
// ==========================================================================

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Warrior
// ==========================================================================

struct warrior_t;

#define SC_WARRIOR 1

#if SC_WARRIOR == 1

enum warrior_stance { STANCE_BATTLE=1, STANCE_BERSERKER, STANCE_DEFENSE=4 };

struct warrior_td_t : public actor_pair_t
{
  dot_t* dots_deep_wounds;
  dot_t* dots_rend;

  buff_t* debuffs_colossus_smash;

  warrior_td_t( player_t* target, warrior_t* p );
};

struct warrior_t : public player_t
{
public:

  int instant_flurry_haste;
  int initial_rage;

  // Active
  action_t* active_deep_wounds;
  action_t* active_retaliation;
  action_t* active_opportunity_strike;
  warrior_stance  active_stance;

  // Buffs
  struct buffs_t
  {
    buff_t* battle_stance;
    buff_t* berserker_rage;
    buff_t* berserker_stance;
    buff_t* bloodsurge;
    buff_t* deadly_calm;
    buff_t* defensive_stance;
    buff_t* enrage;
    buff_t* flurry;
    buff_t* glyph_overpower;
    buff_t* hold_the_line;
    buff_t* incite;
    buff_t* last_stand;
    buff_t* meat_cleaver;
    buff_t* overpower;
    buff_t* raging_blow;
    buff_t* raging_wind;
    buff_t* recklessness;
    buff_t* retaliation;
    buff_t* shield_block;
    buff_t* sweeping_strikes;
    buff_t* sword_and_board;
    buff_t* taste_for_blood;
    buff_t* ultimatum;

    //check
    buff_t* rude_interruption;
    buff_t* thunderstruck;
    buff_t* victory_rush;
    buff_t* tier13_2pc_tank;
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* colossus_smash;
    cooldown_t* revenge;
    cooldown_t* shield_slam;
    cooldown_t* strikes_of_opportunity;
  } cooldown;

  // Gains
  struct gains_t
  {
    gain_t* avoided_attacks;
    gain_t* battle_shout;
    gain_t* charge;
    gain_t* commanding_shout;
    gain_t* defensive_stance;
    gain_t* enrage;
    gain_t* incoming_damage;
    gain_t* melee_main_hand;
    gain_t* melee_off_hand;
    gain_t* revenge;
    gain_t* shield_slam;
  } gain;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* colossus_smash;
    const spell_data_t* furious_sundering;
    const spell_data_t* hold_the_line;
    const spell_data_t* incite;
    const spell_data_t* overpower;
    const spell_data_t* raging_wind;
    const spell_data_t* recklessness;
    const spell_data_t* sweeping_strikes;
    const spell_data_t* unending_rage;
  } glyphs;

  // Mastery
  struct mastery_t
  {
    const spell_data_t* critical_block;
    const spell_data_t* strikes_of_opportunity;
    const spell_data_t* unshackled_fury;
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* munched_deep_wounds;
    proc_t* rolled_deep_wounds;
    proc_t* parry_haste;
    proc_t* strikes_of_opportunity;
    proc_t* sudden_death;
    proc_t* tier13_4pc_melee;
  } proc;

  // Random Number Generation
  struct rngs_t
  {
    rng_t* impending_victory;
    rng_t* strikes_of_opportunity;
    rng_t* sudden_death;
    rng_t* taste_for_blood;
  } rng;

  // Spec Passives
  struct spec_t
  {
    const spell_data_t* anger_management;
    const spell_data_t* bloodsurge;
    const spell_data_t* crazed_berserker;
    const spell_data_t* flurry;
    const spell_data_t* meat_cleaver;
    const spell_data_t* seasoned_soldier;
    const spell_data_t* single_minded_fury;
    const spell_data_t* sudden_death;
    const spell_data_t* sword_and_board;
    const spell_data_t* taste_for_blood;
    const spell_data_t* ultimatum;
    const spell_data_t* unwavering_sentinel;
  } spec;

  // Talents
  struct talents_t
  {
    const spell_data_t* juggernaut;
    const spell_data_t* double_time;
    const spell_data_t* warbringer;

    const spell_data_t* enraged_regeneration;
    const spell_data_t* second_wind;
    const spell_data_t* impending_victory;

    const spell_data_t* staggering_shout;
    const spell_data_t* piercing_howl;
    const spell_data_t* disrupting_shout;

    const spell_data_t* bladestorm;
    const spell_data_t* shockwave;
    const spell_data_t* dragon_roar;

    const spell_data_t* mass_spell_reflection;
    const spell_data_t* safeguard;
    const spell_data_t* vigilance;

    const spell_data_t* avatar;
    const spell_data_t* bloodbath;
    const spell_data_t* storm_bolt;
  } talents;

  // Up-Times
  benefit_t* uptimes_rage_cap;
private:
  target_specific_t<warrior_td_t> target_data;
public:
  warrior_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, WARRIOR, name, r ),
    buff( buffs_t() ),
    cooldown( cooldowns_t() ),
    gain( gains_t() ),
    glyphs( glyphs_t() ),
    mastery( mastery_t() ),
    proc( procs_t() ),
    rng( rngs_t() ),
    spec( spec_t() ),
    talents( talents_t() )
  {

    target_data.init( "target_data", this );
    // Active
    active_deep_wounds        = 0;
    active_opportunity_strike = 0;
    active_stance             = STANCE_BATTLE;

    // Cooldowns
    cooldown.colossus_smash         = get_cooldown( "colossus_smash"         );
    cooldown.shield_slam            = get_cooldown( "shield_slam"            );
    cooldown.strikes_of_opportunity = get_cooldown( "strikes_of_opportunity" );
    cooldown.revenge                = get_cooldown( "revenge" );

    instant_flurry_haste = 1;
    initial_rage = 0;

    initial.distance = 3;
  }

  // Character Definition

  virtual warrior_td_t* get_target_data( player_t* target )
  {
    warrior_td_t*& td = target_data[ target ];
    if ( ! td ) td = new warrior_td_t( target, this );
    return td;
  }

  virtual void      init_spells();
  virtual void      init_defense();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      register_callbacks();
  virtual void      combat_begin();
  virtual double    composite_attack_hit();
  virtual double    composite_attack_crit( weapon_t* );
  virtual double    composite_mastery();
  virtual double    composite_attack_haste();
  virtual double    composite_player_multiplier( school_e school, action_t* a = NULL );
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual double    composite_tank_block();
  virtual double    composite_tank_crit_block();
  virtual double    composite_tank_crit( school_e school );
  virtual void      reset();
  virtual void      regen( timespan_t periodicity );
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_RAGE; }
  virtual role_e primary_role();
  virtual void      assess_damage( school_e, dmg_e, action_state_t* s );
  virtual void      copy_from( player_t* source );

  // Temporary
  virtual std::string set_default_talents()
  {
    switch ( specialization() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_talents();
  }

  virtual std::string set_default_glyphs()
  {
    switch ( specialization() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_glyphs();
  }
};

namespace { // UNNAMED NAMESPACE

// Template for common warrior action code. See priest_action_t.
template <class Base>
struct warrior_action_t : public Base
{
  int stancemask;

  typedef Base ab; // action base, eg. spell_t
  typedef warrior_action_t base_t;

  warrior_action_t( const std::string& n, warrior_t* player,
                 const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {
    ab::may_crit   = true;
  }

  warrior_t* cast() const { return static_cast<warrior_t*>( ab::player ); }

  warrior_td_t* cast_td( player_t* t = 0 ) { return cast() -> get_target_data( t ? t : ab::target ); }

  virtual bool ready()
  {
    if ( ! ab::ready() )
      return false;

    // Attack available in current stance?
    if ( ( stancemask & cast() -> active_stance ) == 0 )
      return false;

    return true;
  }
};

// ==========================================================================
// Warrior Attack
// ==========================================================================

struct warrior_attack_t : public warrior_action_t< melee_attack_t >
{
  warrior_attack_t( const std::string& n, warrior_t* p,
                    const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    may_crit   = true;
    may_glance = false;
  }

  warrior_attack_t( const std::string& n, uint32_t id, warrior_t* p ) :
    base_t( n, p, p -> find_spell( id ) )
  {
    may_crit   = true;
    may_glance = false;
  }

  virtual double armor()
  {
    warrior_td_t* td = cast_td();

    double a = base_t::armor();

    a *= 1.0 - td -> debuffs_colossus_smash -> value();

    return a;
  }

  virtual void   consume_resource();

  virtual void   execute();

  virtual double calculate_weapon_damage( double attack_power )
  {
    double dmg = base_t::calculate_weapon_damage( attack_power );

    // Catch the case where weapon == 0 so we don't crash/retest below.
    if ( dmg == 0 )
      return 0;

    warrior_t* p = cast();

    if ( weapon -> slot == SLOT_OFF_HAND )
    {
      dmg *= 1.0 + p -> spec.crazed_berserker -> effectN( 1 ).percent();

      if ( p -> dual_wield() )
      {
        if ( p -> main_hand_attack -> weapon -> group() == WEAPON_1H &&
             p ->  off_hand_attack -> weapon -> group() == WEAPON_1H )
          dmg *= 1.0 + p -> spec.single_minded_fury -> effectN( 2 ).percent();
      }
    }

    return dmg;
  }

  virtual void   assess_damage( dmg_e, action_state_t* );

  virtual double action_multiplier()
  {
    double am = base_t::action_multiplier();

    warrior_t* p = cast();

    if ( weapon && weapon -> group() == WEAPON_2H )
      am *= 1.0 + p -> spec.seasoned_soldier -> effectN( 1 ).percent();

    // --- Enrages ---
    if ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED )
      am *= 1.0 + p -> buff.enrage -> data().effectN( 1 ).percent();

    if ( p -> buff.enrage -> check () )
      am *= p -> composite_mastery() * p -> mastery.unshackled_fury -> effectN( 2 ).percent();

    // --- Passive Talents ---

    if ( p -> spec.single_minded_fury -> ok() && p -> dual_wield() )
    {
      if ( p -> main_hand_attack -> weapon -> group() == WEAPON_1H &&
           p ->  off_hand_attack -> weapon -> group() == WEAPON_1H )
      {
        am *= 1.0 + p -> spec.single_minded_fury -> effectN( 1 ).percent();
      }
    }

    // --- Buffs / Procs ---

    if ( p -> buff.rude_interruption -> up() )
      am *= 1.05;

    return am;
  }

  virtual double composite_crit()
  {
    double cc = base_t::composite_crit();

    warrior_t* p = cast();

    if ( special )
      cc += p -> buff.recklessness -> value();

    return cc;
  }
};


// ==========================================================================
// Static Functions
// ==========================================================================

// trigger_rage_gain ========================================================

static void trigger_rage_gain( warrior_attack_t* a )
{
  // MoP: base rage gain is 1.75 * weaponspeed and half that for off-hand
  // Battle stance: +100%
  // Berseker stance: no change
  // Defense stance: -100%

  if ( a -> proc )
    return;

  warrior_t* p = a -> cast();
  weapon_t*  w = a -> weapon;

  if ( p -> active_stance == STANCE_DEFENSE )
  {
    return;
  }
  double rage_gain = 1.75 * w -> swing_time.total_seconds();

  if ( p -> active_stance == STANCE_BATTLE )
  {
    rage_gain *= 1.0 + p -> buff.battle_stance -> data().effectN( 1 ).percent();
  }
  else if ( p -> active_stance == STANCE_BERSERKER )
  {
  }
  else   if ( p -> active_stance == STANCE_DEFENSE )
  {
    rage_gain *= 1.0 + p -> buff.defensive_stance -> data().effectN( 3 ).percent();
  }


  if ( w -> slot == SLOT_OFF_HAND )
    rage_gain /= 2.0;

  rage_gain = floor( rage_gain * 10 ) / 10.0;

  p -> resource_gain( RESOURCE_RAGE,
                      rage_gain,
                      w -> slot == SLOT_OFF_HAND ? p -> gain.melee_off_hand : p -> gain.melee_main_hand );
}

// trigger_retaliation ======================================================

static void trigger_retaliation( warrior_t* p, int school, int result )
{
  if ( ! p -> buff.retaliation -> up() )
    return;

  if ( school != SCHOOL_PHYSICAL )
    return;

  if ( ! ( result == RESULT_HIT || result == RESULT_CRIT || result == RESULT_BLOCK ) )
    return;

  if ( ! p -> active_retaliation )
  {
    struct retaliation_strike_t : public warrior_attack_t
    {
      retaliation_strike_t( warrior_t* p ) :
        warrior_attack_t( "retaliation_strike", p )
      {
        background = true;
        proc = true;
        trigger_gcd = timespan_t::zero();
        weapon = &( p -> main_hand_weapon );
        weapon_multiplier = 1.0;

        init();
      }
    };

    p -> active_retaliation = new retaliation_strike_t( p );
  }

  p -> active_retaliation -> execute();
  p -> buff.retaliation -> decrement();
}

// trigger_strikes_of_opportunity ===========================================

struct opportunity_strike_t : public warrior_attack_t
{
  opportunity_strike_t( warrior_t* p ) :
    warrior_attack_t( "opportunity_strike", 76858, p )
  {
    background = true;
  }
};

static void trigger_strikes_of_opportunity( warrior_attack_t* a )
{
  if ( a -> proc )
    return;

  warrior_t* p = a -> cast();

  if ( ! p -> mastery.strikes_of_opportunity -> ok() )
    return;

  if ( p -> cooldown.strikes_of_opportunity -> remains() > timespan_t::zero() )
    return;

  double chance = p -> composite_mastery() * p -> mastery.strikes_of_opportunity -> effectN( 2 ).percent() / 100.0;

  if ( ! p -> rng.strikes_of_opportunity -> roll( chance ) )
    return;

  p -> cooldown.strikes_of_opportunity -> start( timespan_t::from_seconds( 0.5 ) );

  assert( p -> active_opportunity_strike );

  if ( p -> sim -> debug )
    p -> sim -> output( "Opportunity Strike procced from %s", a -> name() );


  p -> proc.strikes_of_opportunity -> occur();
  p -> active_opportunity_strike -> execute();
}

// trigger_sudden_death =====================================================

static void trigger_sudden_death( warrior_attack_t* a )
{
  warrior_t* p = a -> cast();

  if ( a -> proc )
    return;

  if ( p -> rng.sudden_death -> roll ( p -> spec.sudden_death -> proc_chance() ) )
  {
    p -> cooldown.colossus_smash -> reset();
    p -> proc.sudden_death       -> occur();
  }
}

// trigger_enrage ===========================================================

static void trigger_enrage( warrior_t* p )
{
  // Raging blow only triggers when you are not already enraged, not even a
  // refresh
  if ( ! p -> buff.enrage -> check() )
    p -> buff.raging_blow -> trigger();
  p -> buff.enrage -> trigger();
  p -> resource_gain( RESOURCE_RAGE, p -> buff.enrage -> data().effectN( 1 ).resource( RESOURCE_RAGE ), p -> gain.enrage );
}

// trigger_flurry ===========================================================

static void trigger_flurry( warrior_attack_t* a, int stacks )
{
  warrior_t* p = a -> cast();

  bool up_before = p -> buff.flurry -> check() != 0;

  if ( stacks >= 0 )
    p -> buff.flurry -> trigger( stacks );
  else
    p -> buff.flurry -> decrement();

  if ( ! p -> instant_flurry_haste )
    return;

  // Flurry needs to haste the in-progress attacks, and flurry dropping
  // needs to de-haste them.

  bool up_after = p -> buff.flurry -> check() != 0;

  if ( up_before == up_after )
    return;

  sim_t *sim = p -> sim;

  // Default mult is the up -> down case
  // FIXME
  //double mult = 1 + util::talent_rank( p -> talents.flurry -> rank(), 3, 0.08, 0.16, 0.25 );
  double mult = 1;

  // down -> up case
  if ( ! up_before && up_after )
    mult = 1 / mult;

  // This mess would be a lot easier if we could give a time instead of
  // a delta to reschedule_execute().
  if ( p -> main_hand_attack )
  {
    event_t* mhe = p -> main_hand_attack -> execute_event;
    if ( mhe )
    {
      timespan_t delta;
      if ( mhe -> reschedule_time != timespan_t::zero() )
        delta = ( mhe -> reschedule_time - sim -> current_time ) * mult;
      else
        delta = ( mhe -> time - sim -> current_time ) * mult;
      p -> main_hand_attack -> reschedule_execute( delta );
    }
  }
  if ( p -> off_hand_attack )
  {
    event_t* ohe = p -> off_hand_attack -> execute_event;
    if ( ohe )
    {
      timespan_t delta;
      if ( ohe -> reschedule_time != timespan_t::zero() )
        delta = ( ohe -> reschedule_time - sim -> current_time ) * mult;
      else
        delta = ( ohe -> time - sim -> current_time ) * mult;
      p -> off_hand_attack -> reschedule_execute( delta );
    }
  }
}

// ==========================================================================
// Warrior Attacks
// ==========================================================================

// warrior_attack_t::assess_damage ==========================================

void warrior_attack_t::assess_damage( dmg_e dmg_type, action_state_t* s )
{
  base_t::assess_damage( dmg_type, s );

  /* warrior_t* p = cast();

  if ( t -> is_enemy() )
  {
    target_t* q =  t -> cast_target();

    if ( p -> buff.sweeping_strikes -> up() && q -> adds_nearby )
    {
      attack_t::additional_damage( q, amount, dmg_e, impact_result );
    }
  }*/
}

// warrior_attack_t::consume_resource =======================================

void warrior_attack_t::consume_resource()
{
  base_t::consume_resource();
  warrior_t* p = cast();

  if ( proc )
    return;

  if ( result == RESULT_CRIT )
  {
    // Triggered here so it's applied between melee hits and next schedule.
    trigger_flurry( this, 3 );
  }

  // Warrior attacks (non-AoE) which are are avoided by the target consume only 20%
  if ( resource_consumed > 0 && ! aoe && result_is_miss() )
  {
    double rage_restored = resource_consumed * 0.80;
    p -> resource_gain( RESOURCE_RAGE, rage_restored, p -> gain.avoided_attacks );
  }
}

// warrior_attack_t::execute ================================================

void warrior_attack_t::execute()
{
  base_t::execute();
  warrior_t* p = cast();

  if ( proc ) return;

  if ( result_is_hit( execute_state -> result ) )
  {
    trigger_sudden_death( this );

    trigger_strikes_of_opportunity( this );

  }
  else if ( result == RESULT_DODGE  )
  {
    p -> buff.overpower -> trigger();
  }
}

// Melee Attack =============================================================

struct melee_t : public warrior_attack_t
{
  int sync_weapons;

  melee_t( const char* name, warrior_t* p, int sw ) :
    warrior_attack_t( name, p, spell_data_t::nil() ),
    sync_weapons( sw )
  {
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();

    if ( p -> dual_wield() ) base_hit -= 0.19;
  }

  virtual double swing_haste()
  {
    double h = warrior_attack_t::swing_haste();

    warrior_t* p = cast();

    if ( p -> buff.flurry -> up() )
      h *= 1.0 / ( 1.0 + p -> buff.flurry -> data().effectN( 1 ).percent() );

    return h;
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = warrior_attack_t::execute_time();

    if ( player -> in_combat )
      return t;

    if ( weapon -> slot == SLOT_MAIN_HAND || sync_weapons )
      return timespan_t::from_seconds( 0.02 );

    // Before combat begins, unless we are under sync_weapons the OH is
    // delayed by half its swing time.

    return timespan_t::from_seconds( 0.02 ) + t / 2;
  }

  virtual void execute()
  {
    // Be careful changing where this is done.  Flurry that procs from melee
    // must be applied before the (repeating) event schedule, and the decrement
    // here must be done before it.
    trigger_flurry( this, -1 );

    warrior_attack_t::execute();

    if ( result != RESULT_MISS ) // Any attack that hits or is dodged/blocked/parried generates rage
      trigger_rage_gain( this );
  }

  virtual double action_multiplier()
  {
    double am = warrior_attack_t::action_multiplier();

    warrior_t* p = cast();

    if ( p -> specialization() == WARRIOR_FURY )
      am *= 1.0 + p -> spec.crazed_berserker -> effectN( 2 ).percent();

    return am;
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public warrior_attack_t
{
  int sync_weapons;

  auto_attack_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "auto_attack", p ), sync_weapons( 0 )
  {
    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> main_hand_attack = new melee_t( "melee_main_hand", p, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", p, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    p -> main_hand_attack -> schedule_execute();

    if ( p -> off_hand_attack )
      p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( p -> is_moving() )
      return false;

    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Bladestorm ===============================================================

struct bladestorm_tick_t : public warrior_attack_t
{
  bladestorm_tick_t( warrior_t* p, const char* name ) :
    warrior_attack_t( name, 50622, p )
  {
    background  = true;
    direct_tick = true;
    aoe         = -1;
  }
};

struct bladestorm_t : public warrior_attack_t
{
  attack_t* bladestorm_mh;
  attack_t* bladestorm_oh;

  bladestorm_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "bladestorm", p, p -> find_class_spell( "Bladestorm" ) ),
    bladestorm_mh( 0 ), bladestorm_oh( 0 )
  {
    // FIXME
    //check_talent( p -> talents.bladestorm -> rank() );

    parse_options( NULL, options_str );

    aoe       = -1;
    harmful   = false;
    channeled = true;
    tick_zero = true;

    bladestorm_mh = new bladestorm_tick_t( p, "bladestorm_mh" );
    bladestorm_mh -> weapon = &( player -> main_hand_weapon );
    add_child( bladestorm_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      bladestorm_oh = new bladestorm_tick_t( p, "bladestorm_oh" );
      bladestorm_oh -> weapon = &( player -> off_hand_weapon );
      add_child( bladestorm_oh );
    }
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = cast();

    if ( p -> main_hand_attack )
      p -> main_hand_attack -> cancel();

    if ( p ->  off_hand_attack )
      p -> off_hand_attack -> cancel();
  }

  virtual void tick( dot_t* d )
  {
    warrior_attack_t::tick( d );

    bladestorm_mh -> execute();

    if ( bladestorm_mh -> result_is_hit( execute_state -> result ) && bladestorm_oh )
    {
      bladestorm_oh -> execute();
    }
  }

  // Bladestorm is not modified by haste effects
  virtual double composite_haste() { return 1.0; }
  virtual double swing_haste() { return 1.0; }
};

// Bloodthirst Heal ==============================================================

struct bloodthirst_heal_t : public heal_t
{
  bloodthirst_heal_t( warrior_t* p ) :
    heal_t( "bloodthirst_heal", p, p -> find_class_spell( "Bloodthirst" ) )
  {
    // Add Field Dressing Talent, warrior heal etc.

    // Implemented as an actual heal because of spell callbacks ( for Hurricane, etc. )
    background= true;
    init();
  }

  virtual resource_e current_resource() { return RESOURCE_NONE; }
};

// Bloodthirst ==============================================================

struct bloodthirst_t : public warrior_attack_t
{
  bloodthirst_heal_t* bloodthirst_heal;
  bloodthirst_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "bloodthirst", p, p -> find_class_spell( "Bloodthirst" ) )
  {
    parse_options( NULL, options_str );

    weapon             = &( player -> main_hand_weapon );
    bloodthirst_heal   = new bloodthirst_heal_t( p );
  }


  virtual result_e calculate_result( double crit, unsigned target_level )
  {
    return warrior_attack_t::calculate_result( crit * 2.0, target_level );
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      warrior_t* p = cast();

      p -> buff.bloodsurge -> trigger( 3 );

      bloodthirst_heal -> execute();

      if ( p -> set_bonus.tier13_4pc_melee() && sim -> roll( p -> sets -> set( SET_T13_4PC_MELEE ) -> effectN( 1 ).percent() ) )
      {
        warrior_td_t* td = cast_td();
        td -> debuffs_colossus_smash -> trigger();
        p -> proc.tier13_4pc_melee -> occur();
      }

      p -> active_deep_wounds -> execute();
      if ( execute_state -> result == RESULT_CRIT )
        trigger_enrage( p );
    }
  }
};

// Charge ===================================================================

struct charge_t : public warrior_attack_t
{
  int use_in_combat;

  charge_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "charge", p, p -> find_class_spell( "Charge" ) ),
    use_in_combat( 0 ) // For now it's not usable in combat by default because we can't modell the distance/movement.
  {
    option_t options[] =
    {
      { "use_in_combat", OPT_BOOL, &use_in_combat },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown -> duration += p -> talents.juggernaut -> effectN( 3 ).time_value();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();

    if ( p -> position == POSITION_RANGED_FRONT )
      p -> position = POSITION_FRONT;
    else if ( ( p -> position == POSITION_RANGED_BACK ) || ( p -> position == POSITION_MAX ) )
      p -> position = POSITION_BACK;

    p -> resource_gain( RESOURCE_RAGE,
                        data().effectN( 2 ).resource( RESOURCE_RAGE ),
                        p -> gain.charge );
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( p -> in_combat )
    {
      // FIXME:
      /* if ( ! ( p -> talents.juggernaut -> rank() || p -> talents.warbringer -> rank() ) )
        return false;

      else */if ( ! use_in_combat )
        return false;

      if ( ( p -> position == POSITION_BACK ) || ( p -> position == POSITION_FRONT ) )
      {
        return false;
      }
    }

    return warrior_attack_t::ready();
  }
};

// Cleave ===================================================================

struct cleave_t : public warrior_attack_t
{
  cleave_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "cleave", p, p -> find_class_spell( "Cleave" ) )
  {
    parse_options( NULL, options_str );

    weapon = &( player -> main_hand_weapon );

    aoe = 1;
  }

  virtual double cost()
  {
    double c = warrior_attack_t::cost();
    warrior_t* p = cast();

    // Needs testing
    if ( p -> buff.deadly_calm -> check() )
      c += p -> buff.deadly_calm -> data().effectN( 1 ).resource( RESOURCE_RAGE );

    return c;
  }

  virtual double action_multiplier()
  {
    double am = warrior_attack_t::action_multiplier();

    warrior_t* p = cast();

    am *= 1.0 + p -> buff.taste_for_blood -> data().effectN( 1 ).percent() * p -> buff.taste_for_blood -> stack();

    return am;
  }

  virtual void execte()
  {
    warrior_t* p = cast();
    p -> buff.deadly_calm -> up();
    warrior_attack_t::execute();
    p -> buff.deadly_calm -> decrement();
  }
  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    warrior_t* p = cast();
    if ( result_is_hit( s -> result ) )
    {
      p -> buff.glyph_overpower -> trigger();
    }
  }
};

// Colossus Smash ===========================================================

struct colossus_smash_t : public warrior_attack_t
{
  colossus_smash_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "colossus_smash",  p, p -> find_class_spell( "Colossus Smash" ) )
  {
    parse_options( NULL, options_str );

    weapon = &( player -> main_hand_weapon );
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result) )
    {
      warrior_t* p = cast();
      warrior_td_t* td = cast_td( s -> target);
      td -> debuffs_colossus_smash -> trigger( 1, data().effectN( 2 ).percent() );

      if ( ! sim -> overrides.physical_vulnerability )
        s -> target -> debuffs.physical_vulnerability -> trigger();

      if ( p -> glyphs.colossus_smash -> ok() && ! sim -> overrides.weakened_armor )
          s -> target -> debuffs.weakened_armor -> trigger();

    }
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();

    if ( execute_state -> result == RESULT_CRIT )
      trigger_enrage( p );
  }
};

// Concussion Blow ==========================================================

struct concussion_blow_t : public warrior_attack_t
{
  concussion_blow_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "concussion_blow", p, p -> find_class_spell( "Concussion Blow" ) )
  {
    // FIXME:
    // check_talent( p -> talents.concussion_blow -> rank() );

    parse_options( NULL, options_str );

    direct_power_mod  = data().effectN( 3 ).percent();
  }
};

// Deep Wounds ==============================================================

struct deep_wounds_t : public warrior_attack_t
{
  deep_wounds_t( warrior_t* p ) :
    warrior_attack_t( "deep_wounds", p, p -> find_spell( 115767 ) )
  {
    background = true;
    proc = true;
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    tick_power_mod = data().extra_coeff();
  }
};

// Devastate ================================================================

struct devastate_t : public warrior_attack_t
{
  devastate_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "devastate", p, p -> find_class_spell( "Devastate" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = cast();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p -> buff.sword_and_board -> trigger() )
      {
        p -> cooldown.shield_slam -> reset();
      }
    }

    if ( p -> buff.deadly_calm -> check() )
      p -> buff.incite -> trigger();

    p -> active_deep_wounds -> execute();
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( ! sim -> overrides.weakened_blows )
      s -> target -> debuffs.weakened_blows -> trigger();
  }
};

// Execute ==================================================================

struct execute_t : public warrior_attack_t
{
  execute_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "execute", p, p -> find_class_spell( "Execute" ) )
  {
    parse_options( NULL, options_str );

    // Include the weapon so we benefit from racials
    weapon             = &( player -> main_hand_weapon );
    weapon_multiplier  = 0;
    direct_power_mod   = data().extra_coeff();
  }

  virtual bool ready()
  {
    if ( target -> health_percentage() > 20 )
      return false;

    return warrior_attack_t::ready();
  }
};

// Heroic Strike ============================================================

struct heroic_strike_t : public warrior_attack_t
{
  heroic_strike_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "heroic_strike", p, p -> find_class_spell( "Heroic Strike" ) )
  {
    parse_options( NULL, options_str );

    weapon = &( player -> main_hand_weapon );
    harmful           = true;

    // FIX ME: The 140% seems to be hardcoded in the tooltip
    if ( weapon -> group() == WEAPON_1H ||
         weapon -> group() == WEAPON_SMALL )
      base_multiplier *= 1.40;
  }

  virtual double cost()
  {
    double c = warrior_attack_t::cost();
    warrior_t* p = cast();

    // Needs testing
    if ( p -> set_bonus.tier13_2pc_melee() )
      c -= 5.0;

    if ( p -> buff.deadly_calm -> check() )
      c += p -> buff.deadly_calm -> data().effectN( 1 ).resource( RESOURCE_RAGE );

    if ( p -> buff.incite -> check() )
      c += p -> buff.incite  -> data().effectN( 1 ).resource( RESOURCE_RAGE );

    return c;
  }

  virtual double action_multiplier()
  {
    double am = warrior_attack_t::action_multiplier();

    warrior_t* p = cast();

    am *= 1.0 + p -> buff.taste_for_blood -> data().effectN( 1 ).percent() * p -> buff.taste_for_blood -> stack();

    return am;
  }


  virtual void execute()
  {

    warrior_t* p = cast();
    p -> buff.deadly_calm -> up();

    warrior_attack_t::execute();

    //p -> buff.glyph_of_incite -> expire();

    p -> buff.deadly_calm -> decrement();
  }
};

// Heroic Leap ==============================================================

struct heroic_leap_t : public warrior_attack_t
{
  heroic_leap_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "heroic_leap", p, p -> find_class_spell( "Heroic Leap" ) )
  {
    parse_options( NULL, options_str );

    aoe = -1;
    may_dodge = may_parry = false;
    harmful = true; // This should be defaulted to true, but it's not

    // Damage is stored in a trigger spell
    const spell_data_t* dmg_spell = p -> dbc.spell( data().effectN( 3 ).trigger_spell_id() );
    base_dd_min = p -> dbc.effect_min( dmg_spell -> effectN( 1 ).id(), p -> level );
    base_dd_max = p -> dbc.effect_max( dmg_spell -> effectN( 1 ).id(), p -> level );
    direct_power_mod = dmg_spell -> extra_coeff();

    // Heroic Leap can trigger procs from either weapon
    proc_ignores_slot = true;
  }
};

// Mortal Strike ============================================================

struct mortal_strike_t : public warrior_attack_t
{
  mortal_strike_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "mortal_strike", p, p -> find_class_spell( "Mortal Strike" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = cast();

    if ( result_is_hit( execute_state -> result ) )
    {
      warrior_td_t* td = cast_td();

      if ( p -> set_bonus.tier13_4pc_melee() && sim -> roll( p -> sets -> set( SET_T13_4PC_MELEE ) -> proc_chance() ) )
      {
        td -> debuffs_colossus_smash -> trigger();
        p -> proc.tier13_4pc_melee -> occur();
      }

      p -> active_deep_wounds -> execute();

      if ( execute_state -> result == RESULT_CRIT )
        trigger_enrage( p );
    }
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( sim -> overrides.mortal_wounds && result_is_hit( s -> result ) )
      s -> target -> debuffs.mortal_wounds -> trigger();
  }
};

// Overpower ================================================================

struct overpower_t : public warrior_attack_t
{
  double bonus_crit;
  overpower_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "overpower", p, p -> find_class_spell( "Overpower" ) )
  {
    parse_options( NULL, options_str );

    may_dodge  = false;
    may_parry  = false;
    may_block  = false; // The Overpower cannot be blocked, dodged or parried.
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    warrior_attack_t::execute();
    if ( p -> rng.taste_for_blood -> roll( p -> spec.taste_for_blood -> effectN( 1 ).percent() ) )
    {
      p -> buff.overpower -> trigger();
      p -> buff.taste_for_blood -> trigger();
    }
    else
    {
      p -> buff.overpower -> expire();
    }
  }

  virtual result_e calculate_result( double crit, unsigned target_level )
  {
    return warrior_attack_t::calculate_result( crit + data().effectN( 3 ).percent(), target_level );
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( ! p -> buff.overpower -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Pummel ===================================================================

struct pummel_t : public warrior_attack_t
{
  pummel_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "pummel", p, p -> find_class_spell( "Pummel" ) )
  {
    parse_options( NULL, options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Raging Blow ==============================================================

struct raging_blow_attack_t : public warrior_attack_t
{
  raging_blow_attack_t( warrior_t* p, const char* name, const spell_data_t* s  ) :
    warrior_attack_t( name, p, s )
  {
    may_miss = may_dodge = may_parry = false;
    background = true;
  }

  virtual void execute()
  {
    warrior_t* p = cast();
    aoe = p -> buff.meat_cleaver -> stack();

    warrior_attack_t::execute();
  }
};

struct raging_blow_t : public warrior_attack_t
{
  raging_blow_attack_t* mh_attack;
  raging_blow_attack_t* oh_attack;

  raging_blow_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "raging_blow", p, p -> find_class_spell( "Raging Blow" ) ),
    mh_attack( 0 ), oh_attack( 0 )
  {
    // FIXME:
    // check_talent( p -> talents.raging_blow -> rank() );

    // Parent attack is only to determine miss/dodge/parry
    base_dd_min = base_dd_max = 0;
    weapon_multiplier = direct_power_mod = 0;
    may_crit = false;
    weapon = &( p -> main_hand_weapon ); // Include the weapon for racial expertise

    parse_options( NULL, options_str );

    mh_attack = new raging_blow_attack_t( p, "raging_blow_mh", data().effectN( 1 ).trigger() );
    mh_attack -> weapon = &( p -> main_hand_weapon );
    add_child( mh_attack );

    oh_attack = new raging_blow_attack_t( p, "raging_blow_oh", data().effectN( 2 ).trigger() );
    oh_attack -> weapon = &( p -> off_hand_weapon );
    add_child( oh_attack );

    // Needs weapons in both hands
    if ( p -> main_hand_weapon.type == WEAPON_NONE ||
         p -> off_hand_weapon.type == WEAPON_NONE )
      background = true;
  }

  virtual void execute()
  {
    attack_t::execute();
    warrior_t* p = cast();

    if ( result_is_hit( execute_state -> result ) )
    {
      mh_attack -> execute();
      oh_attack -> execute();
      p -> buff.raging_wind -> trigger();
    }

    p -> buff.raging_blow -> expire();
    p -> buff.meat_cleaver -> expire();
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( ! p -> buff.raging_blow -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Revenge ==================================================================

struct revenge_t : public warrior_attack_t
{
  stats_t* absorb_stats;

  revenge_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "revenge", p, p -> find_class_spell( "Revenge" ) ),
    absorb_stats( 0 )
  {
    parse_options( NULL, options_str );

      direct_power_mod = data().extra_coeff();

    // Needs testing
    if ( p -> set_bonus.tier13_2pc_tank() )
    {
      absorb_stats = p -> get_stats( name_str + "_tier13_2pc" );
      absorb_stats -> type = STATS_ABSORB;
    }
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();

    if ( p -> active_stance == STANCE_DEFENSE )
      p -> resource_gain( RESOURCE_RAGE, data().effectN( 2 ).resource( RESOURCE_RAGE ), p -> gain.revenge );
  }
  virtual double action_multiplier()
  {
    double am = warrior_attack_t::action_multiplier();

    warrior_t* p = cast();

    if ( p -> buff.hold_the_line -> up() )
      am *= 1.0 + p -> buff.hold_the_line -> data().effectN( 1 ).percent();

    return am;
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    warrior_t* p = cast();

    // Needs testing
    if ( result_is_hit( s -> result ) )
    {
      if ( absorb_stats )
      {
        double amount = 0.20 * s -> result_amount;
        p -> buff.tier13_2pc_tank -> trigger( 1, amount );
        absorb_stats -> add_result( amount, amount, ABSORB, s -> result );
        absorb_stats -> add_execute( timespan_t::zero() );
      }

      p -> active_deep_wounds -> execute();
    }
  }
};

// Shattering Throw =========================================================

// TO-DO: Only a shell at the moment. Needs testing for damage etc.
struct shattering_throw_t : public warrior_attack_t
{
  shattering_throw_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "shattering_throw", p, p -> find_class_spell( "Shattering Throw" ) )
  {
    parse_options( NULL, options_str );

    direct_power_mod = data().extra_coeff();
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      s -> target -> debuffs.shattering_throw -> trigger();
  }

  virtual bool ready()
  {
    if ( target -> debuffs.shattering_throw -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Shield Slam ==============================================================

struct shield_slam_t : public warrior_attack_t
{
  double rage_gain;
  shield_slam_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "shield_slam", p, p -> find_class_spell( "Shield Slam" ) ),
    rage_gain( 0.0 )
  {
    check_spec( WARRIOR_PROTECTION );

    parse_options( NULL, options_str );

    rage_gain = data().effectN( 3 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE );

    stats -> add_child( player -> get_stats( "shield_slam_combust" ) );
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();
    if (  p -> buff.sword_and_board -> up() )
    {
      p -> resource_gain( RESOURCE_RAGE, 
                          rage_gain + p -> buff.sword_and_board -> data().effectN( 2 ).resource( RESOURCE_RAGE ),
                          p -> gain.shield_slam );
      p -> buff.sword_and_board -> expire();
    }
    else
      p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gain.shield_slam );
  }


  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    warrior_t* p = cast();

    if ( result_is_hit( s -> result ) )
      p -> buff.ultimatum -> trigger();

  }

  virtual double cost()
  {
    warrior_t* p = cast();

    if ( p -> buff.sword_and_board -> check() )
      return 0;

    return warrior_attack_t::cost();
  }
};

// Shockwave ================================================================

struct shockwave_t : public warrior_attack_t
{
  shockwave_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "shockwave", p, p -> find_class_spell( "Shockwave" ) )
  {
    // FIXME:
    // check_talent( p -> talents.shockwave -> rank() );

    parse_options( NULL, options_str );

    direct_power_mod  = data().effectN( 3 ).percent();
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
  }
};

// Slam =====================================================================

struct slam_t : public warrior_attack_t
{
  slam_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "slam", p, p -> find_class_spell( "Slam" ) )
  {
    parse_options( NULL, options_str );

    weapon = &( p -> main_hand_weapon );
  }
};

// Sunder Armor =============================================================

struct sunder_armor_t : public warrior_attack_t
{
  sunder_armor_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "sunder_armor", 7386, p )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p -> glyphs.furious_sundering -> effectN( 1 ).percent();
    background = ( sim -> overrides.weakened_armor != 0 );
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( ! sim -> overrides.weakened_armor )
        s -> target -> debuffs.weakened_armor -> trigger();
    }
  }
};

// Thunder Clap =============================================================

struct thunder_clap_t : public warrior_attack_t
{
  thunder_clap_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "thunder_clap", p, p -> find_class_spell( "Thunder Clap" ) )
  {
    parse_options( NULL, options_str );

    aoe               = -1;
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
    direct_power_mod  = data().extra_coeff();

    if ( p -> spec.unwavering_sentinel -> ok() )
      base_costs[ current_resource() ] *= 1.0 + p -> spec.unwavering_sentinel -> effectN( 2 ).percent();

    // TC can trigger procs from either weapon, even though it doesn't need a weapon
    proc_ignores_slot = true;
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    //warrior_t* p = cast();
    //warrior_td_t* td = cast_td();

    // FIXME:
    // if ( p -> talents.blood_and_thunder -> rank() && td -> dots_rend && td -> dots_rend -> ticking )
    //  td -> dots_rend -> refresh_duration();

    if ( ! sim -> overrides.weakened_blows )
      target -> debuffs.weakened_blows -> trigger();
  }
};

// Whirlwind ================================================================

struct whirlwind_t : public warrior_attack_t
{
  whirlwind_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "whirlwind", p, p -> find_class_spell( "Whirlwind" ) )
  {
    parse_options( NULL, options_str );

    aoe               = -1;
  }

  virtual void consume_resource() { }

  virtual double action_multiplier()
  {
    warrior_t* p = cast();

    double am = warrior_attack_t::action_multiplier();
    am *= 1.0 + p -> buff.raging_wind -> data().effectN( 1 ).percent();
    return am;
  }

  virtual void execute()
  {
    bool meat_cleaver = false;
    warrior_t* p = cast();

    // MH hit
    weapon = &( p -> main_hand_weapon );
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      meat_cleaver = true;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      weapon = &( p -> off_hand_weapon );
      warrior_attack_t::execute();
      if ( result_is_hit( execute_state -> result ) )
        meat_cleaver = true;
    }

    if ( meat_cleaver )
      p -> buff.meat_cleaver -> trigger();

    p -> buff.raging_wind -> expire();

    warrior_attack_t::consume_resource();
  }
};

// Wild Strike ==============================================================

struct wild_strike_t : public warrior_attack_t
{
  wild_strike_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "wild_strike", p, p -> find_class_spell( "Wild Strike" ) )
  {
    parse_options( NULL, options_str );

    weapon = &( player -> off_hand_weapon );
    harmful           = true;

    if ( player -> off_hand_weapon.type == WEAPON_NONE )
      background = true;

  }

  virtual double cost()
  {
    double c = warrior_attack_t::cost();
    warrior_t* p = cast();

    if ( p -> buff.bloodsurge -> check() )
      c += p -> buff.bloodsurge -> data().effectN( 2 ).resource( RESOURCE_RAGE );

    return c;
  }
  virtual void schedule_execute()
  {
    warrior_t* p = cast();

    if ( p -> buff.bloodsurge -> check() )
      trigger_gcd = timespan_t::from_seconds( 1.0 );
    else
      trigger_gcd = data().gcd();

    warrior_attack_t::schedule_execute();
  }
  virtual void execute()
  {

    warrior_t* p = cast();
    p -> buff.bloodsurge -> up();

    warrior_attack_t::execute();

    p -> buff.bloodsurge -> decrement();
  }
};

// Victory Rush =============================================================

struct victory_rush_t : public warrior_attack_t
{
  victory_rush_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "victory_rush", p, p -> find_class_spell( "Victory Rush" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( ! p -> buff.victory_rush -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// ==========================================================================
// Warrior Spells
// ==========================================================================

struct warrior_spell_t : public warrior_action_t< spell_t >
{
  warrior_spell_t( const std::string& n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
  }

  warrior_spell_t( const std::string& n, uint32_t id, warrior_t* p ) :
    base_t( n, p, p -> find_spell( id ) )
  {
  }

  virtual timespan_t gcd()
  {
    // Unaffected by haste
    return trigger_gcd;
  }
};

// Battle Shout =============================================================

struct battle_shout_t : public warrior_spell_t
{
  double rage_gain;

  battle_shout_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "battle_shout", p, p -> find_class_spell( "Battle Shout" ) ),
    rage_gain( 0.0 )
  {
    parse_options( NULL, options_str );

    harmful   = false;

    rage_gain = data().effectN( 3 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE );
    cooldown = player -> get_cooldown( "shout" );
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = cast();

    if ( ! sim -> overrides.attack_power_multiplier )
      sim -> auras.attack_power_multiplier -> trigger( 1, -1.0, -1.0, data().duration() );

    p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gain.battle_shout );
  }
};

// Commanding Shout =============================================================

struct commanding_shout_t : public warrior_spell_t
{
  double rage_gain;

  commanding_shout_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "commanding_shout", p, p -> find_class_spell( "Commanding Shout" ) ),
    rage_gain( 0.0 )
  {
    parse_options( NULL, options_str );

    harmful   = false;

    rage_gain = data().effectN( 2 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE );

    cooldown = player -> get_cooldown( "shout" );
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = cast();

    if ( ! sim -> overrides.stamina )
      sim -> auras.stamina -> trigger( 1, -1.0, -1.0, data().duration() );

    p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gain.commanding_shout );
  }
};

// Berserker Rage ===========================================================

struct berserker_rage_t : public warrior_spell_t
{

  berserker_rage_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "berserker_rage", p, p -> find_class_spell( "Berserker Rage" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = cast();

    p -> buff.berserker_rage -> trigger();
    trigger_enrage( p );
  }
};

// Deadly Calm ==============================================================

struct deadly_calm_t : public warrior_spell_t
{
  deadly_calm_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "deadly_calm", p, p -> find_class_spell( "Deadly Calm" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buff.deadly_calm -> trigger( 3 );
  }

};

// Recklessness =============================================================

struct recklessness_t : public warrior_spell_t
{
  double bonus_crit;
  recklessness_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "recklessness", p, p -> find_class_spell( "Recklessness" ) ),
    bonus_crit( 0.0 )
  {
    parse_options( NULL, options_str );

    harmful = false;
    bonus_crit = data().effectN( 1 ).percent();
    if ( p -> glyphs.recklessness -> ok() )
    {
      bonus_crit += p -> glyphs.recklessness -> effectN( 1 ).percent();
    }
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buff.recklessness -> trigger( 1, bonus_crit );
  }
};

// Retaliation ==============================================================

struct retaliation_t : public warrior_spell_t
{
  retaliation_t( warrior_t* p,  const std::string& options_str ) :
    warrior_spell_t( "retaliation", 20230, p )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buff.retaliation -> trigger( 20 );
  }
};

// Shield Block =============================================================

struct shield_block_buff_t : public buff_t
{
  shield_block_buff_t( warrior_t* p ) :
    buff_t( buff_creator_t( p, "shield_block", p -> find_spell( 2565 )  ) )
  { }
};

struct shield_block_t : public warrior_spell_t
{
  shield_block_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "shield_block", p, p -> find_class_spell( "Shield Block" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buff.shield_block -> trigger();
  }
};

// Stance ===================================================================

struct stance_t : public warrior_spell_t
{
  warrior_stance switch_to_stance;
  std::string stance_str;

  stance_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "stance", p ),
    switch_to_stance( STANCE_BATTLE ), stance_str( "" )
  {
    option_t options[] =
    {
      { "choose",  OPT_STRING, &stance_str     },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( ! stance_str.empty() )
    {
      if ( stance_str == "battle" )
        switch_to_stance = STANCE_BATTLE;
      else if ( stance_str == "berserker" || stance_str == "zerker" )
        switch_to_stance = STANCE_BERSERKER;
      else if ( stance_str == "def" || stance_str == "defensive" )
        switch_to_stance = STANCE_DEFENSE;
    }

    harmful = false;
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 1.0 );
  }

  virtual resource_e current_resource() { return RESOURCE_RAGE; }

  virtual void execute()
  {
    warrior_t* p = cast();

    switch ( p -> active_stance )
    {
    case STANCE_BATTLE:     p -> buff.battle_stance    -> expire(); break;
    case STANCE_BERSERKER:  p -> buff.berserker_stance -> expire(); break;
    case STANCE_DEFENSE:    p -> buff.defensive_stance -> expire(); break;
    }
    p -> active_stance = switch_to_stance;

    switch ( p -> active_stance )
    {
    case STANCE_BATTLE:     p -> buff.battle_stance    -> trigger(); break;
    case STANCE_BERSERKER:  p -> buff.berserker_stance -> trigger(); break;
    case STANCE_DEFENSE:    p -> buff.defensive_stance -> trigger(); break;
    }

    consume_resource();

    update_ready();
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( p -> active_stance == switch_to_stance )
      return false;

    return warrior_spell_t::ready();
  }
};

// Sweeping Strikes =========================================================

struct sweeping_strikes_t : public warrior_spell_t
{
  sweeping_strikes_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "sweeping_strikes", 12328, p )
  {
    // FIXME:
    // check_talent( p -> talents.sweeping_strikes -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buff.sweeping_strikes -> trigger();
  }
};

// Last Stand ===============================================================

struct last_stand_t : public warrior_spell_t
{
  last_stand_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "last_stand", 12975, p )
  {
    // FIXME:
    // check_talent( p -> talents.last_stand -> rank() );

    harmful = false;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buff.last_stand -> trigger();
  }
};

struct buff_last_stand_t : public buff_t
{
  int health_gain;

  buff_last_stand_t( warrior_t* p, const uint32_t id, const std::string& n ) :
    buff_t( buff_creator_t( p, n, p -> find_spell( id ) ) ), health_gain( 0 )
  { }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    health_gain = ( int ) floor( player -> resources.max[ RESOURCE_HEALTH ] * 0.3 );
    player -> stat_gain( STAT_MAX_HEALTH, health_gain );

    return buff_t::trigger( stacks, value, chance, duration );
  }

  virtual void expire()
  {
    player -> stat_loss( STAT_MAX_HEALTH, health_gain );

    buff_t::expire();
  }
};

} // UNNAMED NAMESPACE

// ==========================================================================
// Warrior Character Definition
// ==========================================================================

warrior_td_t::warrior_td_t( player_t* target, warrior_t* p  ) :
  actor_pair_t( target, p )
{
  debuffs_colossus_smash = buff_creator_t( *this, "colossus_smash" ).duration( timespan_t::from_seconds( 6.0 ) );
}

// warrior_t::create_action  ================================================

action_t* warrior_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "auto_attack"        ) return new auto_attack_t        ( this, options_str );
  if ( name == "battle_shout"       ) return new battle_shout_t       ( this, options_str );
  if ( name == "berserker_rage"     ) return new berserker_rage_t     ( this, options_str );
  if ( name == "bladestorm"         ) return new bladestorm_t         ( this, options_str );
  if ( name == "bloodthirst"        ) return new bloodthirst_t        ( this, options_str );
  if ( name == "charge"             ) return new charge_t             ( this, options_str );
  if ( name == "cleave"             ) return new cleave_t             ( this, options_str );
  if ( name == "colossus_smash"     ) return new colossus_smash_t     ( this, options_str );
  if ( name == "concussion_blow"    ) return new concussion_blow_t    ( this, options_str );
  if ( name == "deadly_calm"        ) return new deadly_calm_t        ( this, options_str );
  if ( name == "devastate"          ) return new devastate_t          ( this, options_str );
  if ( name == "execute"            ) return new execute_t            ( this, options_str );
  if ( name == "heroic_leap"        ) return new heroic_leap_t        ( this, options_str );
  if ( name == "heroic_strike"      ) return new heroic_strike_t      ( this, options_str );
  if ( name == "last_stand"         ) return new last_stand_t         ( this, options_str );
  if ( name == "mortal_strike"      ) return new mortal_strike_t      ( this, options_str );
  if ( name == "overpower"          ) return new overpower_t          ( this, options_str );
  if ( name == "pummel"             ) return new pummel_t             ( this, options_str );
  if ( name == "raging_blow"        ) return new raging_blow_t        ( this, options_str );
  if ( name == "recklessness"       ) return new recklessness_t       ( this, options_str );
  if ( name == "retaliation"        ) return new retaliation_t        ( this, options_str );
  if ( name == "revenge"            ) return new revenge_t            ( this, options_str );
  if ( name == "shattering_throw"   ) return new shattering_throw_t   ( this, options_str );
  if ( name == "shield_block"       ) return new shield_block_t       ( this, options_str );
  if ( name == "shield_slam"        ) return new shield_slam_t        ( this, options_str );
  if ( name == "shockwave"          ) return new shockwave_t          ( this, options_str );
  if ( name == "slam"               ) return new slam_t               ( this, options_str );
  if ( name == "stance"             ) return new stance_t             ( this, options_str );
  if ( name == "sunder_armor"       ) return new sunder_armor_t       ( this, options_str );
  if ( name == "sweeping_strikes"   ) return new sweeping_strikes_t   ( this, options_str );
  if ( name == "thunder_clap"       ) return new thunder_clap_t       ( this, options_str );
  if ( name == "victory_rush"       ) return new victory_rush_t       ( this, options_str );
  if ( name == "whirlwind"          ) return new whirlwind_t          ( this, options_str );

  return player_t::create_action( name, options_str );
}

// warrior_t::init_spells ===================================================

void warrior_t::init_spells()
{
  player_t::init_spells();

  // Mastery
  mastery.critical_block         = find_mastery_spell( WARRIOR_PROTECTION );
  mastery.strikes_of_opportunity = find_mastery_spell( WARRIOR_ARMS );
  mastery.unshackled_fury        = find_mastery_spell( WARRIOR_FURY );

  // Spec Passives
  spec.anger_management                 = find_specialization_spell( "Anger Management" );
  spec.bloodsurge                       = find_specialization_spell( "Bloodsurge" );
  spec.crazed_berserker                 = find_specialization_spell( "Crazed Berserker" );
  spec.flurry                           = find_specialization_spell( "Flurry" );
  spec.meat_cleaver                     = find_specialization_spell( "Meat Cleaver" );
  spec.seasoned_soldier                 = find_specialization_spell( "Seasoned Soldier" );
  spec.unwavering_sentinel              = find_specialization_spell( "Unwavering Sentinel" );
  spec.single_minded_fury               = find_specialization_spell( "Single-Minded Fury" );
  spec.sword_and_board                  = find_specialization_spell( "Sword and Board" );
  spec.sudden_death                     = find_specialization_spell( "Sudden Death" );
  spec.taste_for_blood                  = find_specialization_spell( "Taste for Blood" );
  spec.ultimatum                        = find_specialization_spell( "Ultimatum" );

  // Talents
  talents.juggernaut            = find_talent_spell( "Juggernaut" );
  talents.double_time           = find_talent_spell( "Double Time" );
  talents.warbringer            = find_talent_spell( "Warbringer" );

  talents.enraged_regeneration  = find_talent_spell( "Enraged Regeneration" );
  talents.second_wind           = find_talent_spell( "Second Wind" );
  talents.impending_victory     = find_talent_spell( "Impending Victory" );

  talents.staggering_shout      = find_talent_spell( "Staggering Shout" );
  talents.piercing_howl         = find_talent_spell( "Piercing Howl" );
  talents.disrupting_shout      = find_talent_spell( "Disrupting Shout" );

  talents.bladestorm            = find_talent_spell( "Bladestorm" );
  talents.shockwave             = find_talent_spell( "Shockwave" );
  talents.dragon_roar           = find_talent_spell( "Dragon Roar" );

  talents.mass_spell_reflection = find_talent_spell( "Mass Spell Reflection" );
  talents.safeguard             = find_talent_spell( "Safeguard" );
  talents.vigilance             = find_talent_spell( "Vigilance" );

  talents.avatar                = find_talent_spell( "Avatar" );
  talents.bloodbath             = find_talent_spell( "Bloodbath" );
  talents.storm_bolt            = find_talent_spell( "Storm Bolt" );

  // Glyphs
  glyphs.colossus_smash      = find_glyph_spell( "Glyph of Colossus Smash" );
  glyphs.furious_sundering   = find_glyph_spell( "Glyph of Forious Sundering" );
  glyphs.hold_the_line       = find_glyph_spell( "Glyph of Hold the Line" );
  glyphs.incite              = find_glyph_spell( "Glyph of Incite" );
  glyphs.overpower           = find_glyph_spell( "Glyph of Overpower" );
  glyphs.raging_wind         = find_glyph_spell( "Glyph of Raging Wind" );
  glyphs.recklessness        = find_glyph_spell( "Glyph of Recklessness" );
  glyphs.sweeping_strikes    = find_glyph_spell( "Glyph of Sweeping Strikes" );
  glyphs.unending_rage       = find_glyph_spell( "Glyph of Unending Rage" );

  // Active spells
  active_deep_wounds = new deep_wounds_t( this );

  if ( mastery.strikes_of_opportunity -> ok() )
    active_opportunity_strike = new opportunity_strike_t( this );


  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P     M2P     M4P     T2P     T4P    H2P    H4P
    {     0,     0, 105797, 105907, 105908, 105911,     0,     0 }, // Tier13
    {     0,     0, 123142, 123144, 123146, 123147,     0,     0 }, // Tier14
    {     0,     0,      0,      0,      0,      0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// warrior_t::init_defense ==================================================

void warrior_t::init_defense()
{
  player_t::init_defense();

  initial.parry_rating_per_strength = 0.27;
}

// warrior_t::init_base =====================================================

void warrior_t::init_base()
{
  player_t::init_base();

  resources.base[  RESOURCE_RAGE  ] = 100;
  if ( glyphs.unending_rage -> ok() )
    resources.base[  RESOURCE_RAGE  ] += glyphs.unending_rage -> effectN( 1 ).resource( RESOURCE_RAGE );

  initial.attack_power_per_strength = 2.0;
  initial.attack_power_per_agility  = 0.0;

  base.attack_power = level * 2 + 60;

  // FIXME! Level-specific!
  base.miss    = 0.05;
  base.parry   = 0.05;
  base.block   = 0.05;

  if ( specialization() == WARRIOR_PROTECTION )
    vengeance.enabled = true;

  diminished_kfactor    = 0.009560;
  diminished_dodge_capi = 0.01523660;
  diminished_parry_capi = 0.01523660;

  if ( spec.unwavering_sentinel -> ok() )
  {
    initial.attribute_multiplier[ ATTR_STAMINA ] *= 1.0  + spec.unwavering_sentinel -> effectN( 1 ).percent();
    initial.armor_multiplier *= 1.0 + spec.unwavering_sentinel-> effectN( 3 ).percent();
  }

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// warrior_t::init_scaling ==================================================

void warrior_t::init_scaling()
{
  player_t::init_scaling();

  if ( specialization() == WARRIOR_FURY )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = true;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors != 0;
    scales_with[ STAT_HIT_RATING2           ] = true;
  }

  if ( primary_role() == ROLE_TANK )
  {
    scales_with[ STAT_PARRY_RATING ] = true;
    scales_with[ STAT_BLOCK_RATING ] = true;
  }
}

// warrior_t::init_buffs ====================================================

void warrior_t::init_buffs()
{
  player_t::init_buffs();

  buff.battle_stance    = buff_creator_t( this, "battle_stance",    find_spell( 21156 ) );
  buff.berserker_rage   = buff_creator_t( this, "berserker_rage",   find_class_spell( "Berserker Rage" ) );
  buff.berserker_stance = buff_creator_t( this, "berserker_stance", find_spell( 7381 ) );
  buff.bloodsurge       = buff_creator_t( this, "bloodsurge",       spec.bloodsurge -> effectN( 1 ).trigger() )
                          .chance( ( spec.bloodsurge -> ok() ? spec.bloodsurge -> proc_chance() : 0 ) );
  buff.deadly_calm      = buff_creator_t( this, "deadly_calm",      find_class_spell( "Deadly Calm" ) );
  buff.defensive_stance = buff_creator_t( this, "defensive_stance", find_spell( 7376 ) );
  buff.enrage           = buff_creator_t( this, "enrage",           find_spell( 12880 ) );
  buff.flurry           = buff_creator_t( this, "flurry",           spec.flurry -> effectN( 1 ).trigger() )
                          .chance( spec.flurry -> proc_chance() );
  buff.glyph_overpower  = buff_creator_t( this, "glyph_of_overpower", glyphs.overpower -> effectN( 1 ).trigger() )
                          .chance( glyphs.overpower -> ok() ? glyphs.overpower -> proc_chance() : 0 );
  buff.hold_the_line    = buff_creator_t( this, "hold_the_line",    glyphs.hold_the_line -> effectN( 1 ).trigger() );
  buff.incite           = buff_creator_t( this, "incite",           glyphs.incite -> effectN( 1 ).trigger() )
                          .chance( glyphs.incite -> ok () ? glyphs.incite -> proc_chance() : 0 );
  buff.meat_cleaver     = buff_creator_t( this, "meat_cleaver",     spec.meat_cleaver -> effectN( 1 ).trigger() );
  buff.overpower        = buff_creator_t( this, "overpower",        spell_data_t::nil() )
                          .duration( timespan_t::from_seconds( 9.0 ) );
  buff.raging_blow      = buff_creator_t( this, "raging_blow",      find_spell( 131116 ) )
                          .max_stack( find_spell( 131116 ) -> effectN( 1 ).base_value() );
  buff.raging_wind      = buff_creator_t( this, "raging_wind",      glyphs.raging_wind -> effectN( 1 ).trigger() );
  buff.recklessness     = buff_creator_t( this, "recklessness",     find_class_spell( "Recklessness" ) )
                          .duration( find_class_spell( "Recklessness" ) -> duration() * ( 1.0 + ( glyphs.recklessness -> ok() ? glyphs.recklessness -> effectN( 2 ).percent() : 0 )  ) );
  buff.retaliation      = buff_creator_t( this, "retaliation", find_spell( 20230 ) );
  buff.taste_for_blood  = buff_creator_t( this, "taste_for_blood",  find_spell( 125831 ) );
  buff.shield_block     = new shield_block_buff_t( this );
  buff.sweeping_strikes = buff_creator_t( this, "sweeping_strikes",  find_class_spell( "Sweeping Strikes" ) );
  buff.sword_and_board  = buff_creator_t( this, "sword_and_board",   find_spell( 50227 ) )
                          .chance( spec.sword_and_board -> effectN( 1 ).percent() );
  buff.ultimatum        = buff_creator_t( this, "ultimatum",   spec.ultimatum -> effectN( 1 ).trigger() )
                          .chance( spec.ultimatum -> ok() ? spec.ultimatum -> proc_chance() : 0 );

  buff.last_stand       = new buff_last_stand_t( this, 12976, "last_stand" );
  buff.tier13_2pc_tank  = buff_creator_t( this, "tier13_2pc_tank", find_spell( 105909 ) );
  // FIX ME
  // absorb_buffs.push_back( buff.tier13_2pc_tank );

}

// warrior_t::init_gains ====================================================

void warrior_t::init_gains()
{
  player_t::init_gains();

  gain.avoided_attacks        = get_gain( "avoided_attacks"       );
  gain.battle_shout           = get_gain( "battle_shout"          );
  gain.charge                 = get_gain( "charge"                );
  gain.commanding_shout       = get_gain( "commanding_shout"      );
  gain.defensive_stance       = get_gain( "defensive_stance"      );
  gain.enrage                 = get_gain( "enrage"        );
  gain.incoming_damage        = get_gain( "incoming_damage"       );
  gain.melee_main_hand        = get_gain( "melee_main_hand"       );
  gain.melee_off_hand         = get_gain( "melee_off_hand"        );
  gain.revenge                = get_gain( "revenge"               );
  gain.shield_slam            = get_gain( "shield_slam" );
}

// warrior_t::init_procs ====================================================

void warrior_t::init_procs()
{
  player_t::init_procs();

  proc.munched_deep_wounds     = get_proc( "munched_deep_wounds"     );
  proc.rolled_deep_wounds      = get_proc( "rolled_deep_wounds"      );
  proc.parry_haste             = get_proc( "parry_haste"             );
  proc.strikes_of_opportunity  = get_proc( "strikes_of_opportunity"  );
  proc.sudden_death            = get_proc( "sudden_death"            );
  proc.tier13_4pc_melee        = get_proc( "tier13_4pc_melee"        );
}

// warrior_t::init_uptimes ==================================================

void warrior_t::init_benefits()
{
  player_t::init_benefits();

  uptimes_rage_cap    = get_benefit( "rage_cap" );
}

// warrior_t::init_rng ======================================================

void warrior_t::init_rng()
{
  player_t::init_rng();

  rng.impending_victory         = get_rng( "impending_victory"         );
  rng.strikes_of_opportunity    = get_rng( "strikes_of_opportunity"    );
  rng.sudden_death              = get_rng( "sudden_death"              );
  rng.taste_for_blood           = get_rng( "taste_for_blood"           );
}

// warrior_t::init_actions ==================================================

void warrior_t::init_actions()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    switch ( specialization() )
    {
    case WARRIOR_FURY:
    case WARRIOR_ARMS:
      // Flask
      if ( level > 85 )
        action_list_str += "/flask,type=winters_bite,precombat=1";
      else if ( level >= 80 )
        action_list_str += "/flask,type=titanic_strength,precombat=1";

      // Food
      if ( level > 85 )
        action_list_str += "/food,type=black_pepper_ribs_and_shrimp,precombat=1";
      else if ( level >= 80 )
        action_list_str += "/food,type=seafood_magnifique_feast,precombat=1";

      break;

    case WARRIOR_PROTECTION:
      // Flask
      if ( level >= 80 )
        action_list_str += "/flask,type=earth,precombat=1";
      else if ( level >= 75 )
        action_list_str += "/flask,type=steelskin,precombat=1";

      // Food
      if ( level >= 80 )
        action_list_str += "/food,type=great_pandaren_banquet,precombat=1";
      else if ( level >= 70 )
        action_list_str += "/food,type=beer_basted_crocolisk,precombat=1";

    break; default: break;
    }

    action_list_str += "/snapshot_stats,precombat=1";

    // Potion
    if ( specialization() == WARRIOR_ARMS )
    {
      if ( level > 85 )
        action_list_str += "/mogu_power_potion,precombat=1/mogu_power_potion,if=buff.recklessness.up|target.time_to_die<26";
      else if ( level >= 80 )
        action_list_str += "/golemblood_potion,precombat=1/golemblood_potion,if=buff.recklessness.up|target.time_to_die<26";
    }
    else if ( specialization() == WARRIOR_FURY )
    {
      if ( level > 85 )
        action_list_str += "/mogu_power_potion,precombat=1/mogu_power_potion,if=buff.bloodlust.react";
      else if ( level >= 80 )
        action_list_str += "/golemblood_potion,precombat=1/golemblood_potion,if=buff.bloodlust.react";
    }
    else
    {
      if ( level > 85 )
        action_list_str += "/mountains_potion,precombat=1/mountains_potion,if=health_pct<35&buff.mountains_potion.down";
      else if ( level >= 80 )
        action_list_str += "/earthen_potion,precombat=1/earthen_potion,if=health_pct<35&buff.earthen_potion.down";
    }

    action_list_str += "/auto_attack";

    // Usable Item
    int num_items = ( int ) items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }

    action_list_str += init_use_profession_actions();
    action_list_str += init_use_racial_actions();

    // Heroic Leap, for everyone but tanks
    if ( primary_role() != ROLE_TANK )
      action_list_str += "/heroic_leap,use_off_gcd=1,if=buff.colossus_smash.up";

    // Arms
    if ( specialization() == WARRIOR_ARMS )
    {
      action_list_str += "/deadly_calm,use_off_gcd=1";
      action_list_str += "/recklessness,if=target.health_pct>90|target.health_pct<=20,use_off_gcd=1";
      action_list_str += "/stance,choose=berserker,if=buff.taste_for_blood.down&dot.rend.remains>0&rage<=75,use_off_gcd=1";
      action_list_str += "/stance,choose=battle,use_off_gcd=1,if=!dot.rend.ticking";
      action_list_str += "/stance,choose=battle,use_off_gcd=1,if=(buff.taste_for_blood.up|buff.overpower.up)&rage<=75&cooldown.mortal_strike.remains>=1.5,use_off_gcd=1";
      action_list_str += "/cleave,if=target.adds>0,use_off_gcd=1";
      action_list_str += "/rend,if=!ticking";
      // Don't want to bladestorm during SS as it's only 1 extra hit per WW not per target
      action_list_str += "/bladestorm,if=target.adds>0&!buff.deadly_calm.up&!buff.sweeping_strikes.up";
      action_list_str += "/mortal_strike,if=target.health_pct>20";
      if ( level >= 81 ) action_list_str += "/colossus_smash,if=buff.colossus_smash.down";
      action_list_str += "/mortal_strike,if=target.health_pct<=20&(dot.rend.remains<3|buff.wrecking_crew.down|rage<=25|rage>=35)";
      action_list_str += "/execute,if=rage>90";
      action_list_str += "/overpower,if=buff.taste_for_blood.up|buff.overpower.up";
      action_list_str += "/execute";
      action_list_str += "/colossus_smash,if=buff.colossus_smash.remains<=1.5";
      action_list_str += "/slam,if=(rage>=35|buff.battle_trance.up|buff.deadly_calm.up)";
      action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.deadly_calm.up";
      action_list_str += "/heroic_strike,use_off_gcd=1,if=rage>85";
      action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.inner_rage.up&target.health_pct>20&(rage>=60|(set_bonus.tier13_2pc_melee&rage>=50))";
      action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.inner_rage.up&target.health_pct<=20&((rage>=60|(set_bonus.tier13_2pc_melee&rage>=50))|buff.battle_trance.up)";
      action_list_str += "/battle_shout,if=rage<60|!aura.attack_power_multiplier.up";
    }

    // Fury
    else if ( specialization() == WARRIOR_FURY )
    {
      action_list_str += "/stance,choose=berserker";
      if ( true /* titans grip */ )
      {
        action_list_str += "/recklessness,use_off_gcd=1";
        action_list_str += "/cleave,if=target.adds>0,use_off_gcd=1";
        action_list_str += "/whirlwind,if=target.adds>0";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=(rage>=85|(set_bonus.tier13_2pc_melee&buff.inner_rage.up&rage>=75))&target.health_pct>=20";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.battle_trance.up";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=(buff.glyph_of_incite.up|buff.colossus_smash.up)&(((rage>=50|(rage>=40&set_bonus.tier13_2pc_melee&buff.inner_rage.up))&target.health_pct>=20)|((rage>=75|(rage>=65&set_bonus.tier13_2pc_melee&buff.inner_rage.up))&target.health_pct<20))";
        action_list_str += "/execute,if=buff.executioner_talent.remains<1.5";
        if ( level >= 81 ) action_list_str += "/colossus_smash";
        action_list_str += "/execute,if=buff.executioner_talent.stack<5";
        action_list_str += "/bloodthirst";
        action_list_str += "/slam,if=buff.bloodsurge.react";
        action_list_str += "/execute,if=rage>=50";
        action_list_str += "/battle_shout,if=rage<70|!aura.attack_power_multiplier.up";
      }
      else
      {
        action_list_str += "/berserker_rage,use_off_gcd=1,if=rage<95";
        action_list_str += "/recklessness,use_off_gcd=1,if=buff.death_wish.up|target.time_to_die<13";
        action_list_str += "/cleave,if=target.adds>0,use_off_gcd=1";
        action_list_str += "/whirlwind,if=target.adds>0";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=set_bonus.tier13_2pc_melee&buff.inner_rage.up&rage>=60&target.health_pct>=20";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.battle_trance.up";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.colossus_smash.up&rage>50";
        action_list_str += "/bloodthirst";
        action_list_str += "/colossus_smash,if=buff.colossus_smash.down";
        action_list_str += "/execute,if=rage>=50&cooldown.bloodthirst.remains>0.2";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=(buff.glyph_of_incite.up|buff.colossus_smash.up)&((rage>=50|(rage>=40&set_bonus.tier13_2pc_melee&buff.inner_rage.up))&target.health_pct>=20)";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=(buff.glyph_of_incite.up|buff.colossus_smash.up)&((rage>=75|(rage>=65&set_bonus.tier13_2pc_melee&buff.inner_rage.up))&target.health_pct<20)";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=rage>=85";
        action_list_str += "/battle_shout,if=(rage<70&cooldown.bloodthirst.remains>0.2)|!aura.attack_power_multiplier.up";
      }
    }

    // Protection
    else if ( specialization() == WARRIOR_PROTECTION )
    {
      action_list_str += "/stance,choose=defensive";
      action_list_str += "/last_stand,if=health<30000";
      action_list_str += "/heroic_strike,if=rage>=50";
      action_list_str += "/inner_rage,if=rage>=85,use_off_gcd=1";
      action_list_str += "/berserker_rage,use_off_gcd=1";
      action_list_str += "/shield_block,sync=shield_slam";
      action_list_str += "/shield_slam";
      action_list_str += "/thunder_clap,if=dot.rend.remains<=3";
      action_list_str += "/rend,if=!ticking";
      if ( talents.shockwave -> ok() ) action_list_str += "/shockwave";
      action_list_str += "/concussion_blow";
      action_list_str += "/revenge";
      action_list_str += "/battle_shout";
    }

    // Default
    else
    {
      action_list_str += "/stance,choose=berserker/auto_attack";
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// warrior_t::combat_begin ==================================================

void warrior_t::combat_begin()
{
  player_t::combat_begin();

  // We (usually) start combat with zero rage.
  resources.current[ RESOURCE_RAGE ] = std::min( initial_rage, 100 );

  if ( active_stance == STANCE_BATTLE && ! buff.battle_stance -> check() )
    buff.battle_stance -> trigger();
}

// warrior_t::register_callbacks ==============================================

void warrior_t::register_callbacks()
{
  player_t::register_callbacks();
}

// warrior_t::reset =========================================================

void warrior_t::reset()
{
  player_t::reset();

  active_stance = STANCE_BATTLE;
}

// warrior_t::composite_attack_hit ==========================================

double warrior_t::composite_attack_hit()
{
  double ah = player_t::composite_attack_hit();

  return ah;
}

// warrior_t::composite_attack_crit =========================================

double warrior_t::composite_attack_crit( weapon_t* w )
{
  double c = player_t::composite_attack_crit( w );

  return c;
}

// warrior_t::composite_mastery =============================================

double warrior_t::composite_mastery()
{
  double m = player_t::composite_mastery();

  return m;
}

// warrior_t::composite_player_multiplier ===================================

double warrior_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  return m;
}

// warrior_t::matching_gear_multiplier ======================================

double warrior_t::matching_gear_multiplier( attribute_e attr )
{
  if ( ( attr == ATTR_STRENGTH ) && ( specialization() == WARRIOR_ARMS || specialization() == WARRIOR_FURY ) )
    return 0.05;

  if ( ( attr == ATTR_STAMINA ) && ( specialization() == WARRIOR_PROTECTION ) )
    return 0.05;

  return 0.0;
}

// warrior_t::composite_tank_block ==========================================

double warrior_t::composite_tank_block()
{
  double b = player_t::composite_tank_block();

  b += composite_mastery() * mastery.critical_block -> effectN( 3 ).percent() / 100.0;

  if ( buff.shield_block -> up() )
    b = 1.0;

  return b;
}

// warrior_t::composite_tank_crit_block =====================================

double warrior_t::composite_tank_crit_block()
{
  double b = player_t::composite_tank_crit_block();

  b += composite_mastery() * mastery.critical_block->effectN( 1 ).coeff() / 100.0;

  return b;
}

// warrior_t::composite_tank_crit ===========================================

double warrior_t::composite_tank_crit( const school_e school )
{
  double c = player_t::composite_tank_crit( school );

  
  c += spec.unwavering_sentinel -> effectN( 4 ).percent();

  return c;
}

// warrior_t::regen =========================================================

void warrior_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buff.defensive_stance -> check() )
    resource_gain( RESOURCE_RAGE, ( periodicity.total_seconds() / 3.0 ), gain.defensive_stance );

  uptimes_rage_cap -> update( resources.current[ RESOURCE_RAGE ] ==
                              resources.max    [ RESOURCE_RAGE] );
}

// warrior_t::primary_role() ================================================

role_e warrior_t::primary_role()
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
    return ROLE_ATTACK;

  if ( specialization() == WARRIOR_PROTECTION )
    return ROLE_TANK;

  return ROLE_ATTACK;
}

// warrior_t::assess_damage =================================================

void warrior_t::assess_damage( school_e school,
                                 dmg_e    dtype,
                                 action_state_t* s )
{
  if ( s -> result == RESULT_HIT    ||
       s -> result == RESULT_CRIT   ||
       s -> result == RESULT_GLANCE ||
       s -> result == RESULT_BLOCK  )
  {
    double rage_gain = s -> result_amount * 18.92 / resources.max[ RESOURCE_HEALTH ];

    resource_gain( RESOURCE_RAGE, rage_gain, gain.incoming_damage );
  }


  if ( s -> result == RESULT_DODGE ||
       s -> result == RESULT_PARRY )
  {
    cooldown.revenge -> reset();
  }


  if ( s -> result == RESULT_PARRY )
  {
    buff.hold_the_line -> trigger();

    if ( main_hand_attack && main_hand_attack -> execute_event )
    {
      timespan_t swing_time = main_hand_attack -> time_to_execute;
      timespan_t max_reschedule = ( main_hand_attack -> execute_event -> occurs() - 0.20 * swing_time ) - sim -> current_time;

      if ( max_reschedule > timespan_t::zero() )
      {
        main_hand_attack -> reschedule_execute( std::min( ( 0.40 * swing_time ), max_reschedule ) );
        proc.parry_haste -> occur();
      }
    }
  }

  trigger_retaliation( this, school, s -> result );

  player_t::assess_damage( school, dtype, s );
}

// warrior_t::create_options ================================================

void warrior_t::create_options()
{
  player_t::create_options();

  option_t warrior_options[] =
  {
    { "initial_rage",            OPT_INT,  &initial_rage            },
    { "instant_flurry_haste",    OPT_BOOL, &instant_flurry_haste    },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, warrior_options );
}

// warrior_t::copy_from =====================================================

void warrior_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warrior_t* p = debug_cast<warrior_t*>( source );

  initial_rage            = p -> initial_rage;
  instant_flurry_haste    = p -> instant_flurry_haste;
}

// warrior_t::decode_set ====================================================

int warrior_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  if ( strstr( s, "colossal_dragonplate" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "battleplate"   ) ||
                      strstr( s, "legplates"     ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    if ( is_melee ) return SET_T13_MELEE;
    if ( is_tank  ) return SET_T13_TANK;
  }

  if ( strstr( s, "resounding_rings" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "battleplate"   ) ||
                      strstr( s, "legplates"     ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    if ( is_melee ) return SET_T14_MELEE;
    if ( is_tank  ) return SET_T14_TANK;
  }

  if ( strstr( s, "_gladiators_plate_"   ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

#endif // SC_WARRIOR

// WARRIOR MODULE INTERFACE ================================================

struct warrior_module_t : public module_t
{
  warrior_module_t() : module_t( WARRIOR ) {}

  virtual player_t* create_player( sim_t* /*sim*/, const std::string& /*name*/, race_e /*r = RACE_NONE*/ )
  {
    return NULL; // new warrior_t( sim, name, r );
  }
  virtual bool valid() { return false; }
  virtual void init( sim_t* sim )
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      p -> debuffs.shattering_throw      = buff_creator_t( p, "shattering_throw", p -> find_spell( 64382 ) );
    }
  }
  virtual void combat_begin( sim_t* ) {}
  virtual void combat_end  ( sim_t* ) {}
};

} // UNNAMED NAMESPACE

module_t* module_t::warrior()
{
  static module_t* m = 0;
  if ( ! m ) m = new warrior_module_t();
  return m;
}
