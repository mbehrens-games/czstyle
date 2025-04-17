/*******************************************************************************
** voice.c (synth voice)
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "cart.h"
#include "clock.h"
#include "voice.h"

#define PI 3.14159265358979323846f

/* tuning */
#define VOICE_NOTE_LOWEST_AVAILABLE  ( 0 * 12 +  9) /* A negative 3 */
#define VOICE_NOTE_LOWEST_PLAYABLE   ( 3 * 12 +  9) /* A-0 */
#define VOICE_NOTE_HIGHEST_PLAYABLE  (11 * 12 +  0) /* C-8 */
#define VOICE_NOTE_HIGHEST_AVAILABLE (12 * 12 + 11) /* B-9 */

#define VOICE_NOTE_MIDDLE_C          ( 7 * 12 +  0) /* C-4 */

#define VOICE_TUNING_STEPS_PER_OCTAVE 1024
#define VOICE_TUNING_NUM_OCTAVES      13    /* -3 to 9 */

#define VOICE_TUNING_MAX_PITCH_INDEX  ( (VOICE_TUNING_NUM_OCTAVES - 1) * VOICE_TUNING_STEPS_PER_OCTAVE + \
                                        ((11 * VOICE_TUNING_STEPS_PER_OCTAVE) / 12))

/* sine wavetable, db to linear table constants */
#define VOICE_DB_STEP_12_BIT 0.01171875f

#define VOICE_DB_12_BIT_NUM_BITS  12
#define VOICE_DB_12_BIT_SIZE      (1 << VOICE_DB_12_BIT_NUM_BITS)
#define VOICE_DB_12_BIT_MASK      (VOICE_DB_12_BIT_SIZE - 1)

#define VOICE_MAX_VOLUME_DB       0
#define VOICE_MAX_ATTENUATION_DB  (VOICE_DB_12_BIT_SIZE - 1)

#define VOICE_MAX_VOLUME_LINEAR       32767
#define VOICE_MAX_ATTENUATION_LINEAR  0

#define VOICE_ENVELOPE_STEPS_PER_ROW  1024
#define VOICE_ENVELOPE_NUM_ROWS       13    /* 8 times per row, with 100 times total = 12.5 rows */

#define VOICE_ENVELOPE_MAX_TIME_INDEX (VOICE_ENVELOPE_NUM_ROWS * VOICE_ENVELOPE_STEPS_PER_ROW)

/* phase register */
#define VOICE_PHASE_REG_NUM_BITS  24
#define VOICE_PHASE_REG_SIZE      (1 << VOICE_PHASE_REG_NUM_BITS)
#define VOICE_PHASE_REG_MASK      (VOICE_PHASE_REG_SIZE - 1)

#define VOICE_PHASE_WAVE_NUM_BITS 11
#define VOICE_PHASE_WAVE_SIZE     (1 << VOICE_PHASE_WAVE_NUM_BITS)
#define VOICE_PHASE_WAVE_MASK     (VOICE_PHASE_WAVE_SIZE - 1)

#define VOICE_PHASE_MANTISSA_NUM_BITS  (VOICE_PHASE_REG_NUM_BITS - VOICE_PHASE_WAVE_NUM_BITS)

#define VOICE_1HZ_PHASE_INCREMENT ((float) VOICE_PHASE_REG_SIZE / CLOCK_SAMPLING_RATE)

/* oscillator constants */

/* envelope constants */
enum
{
  VOICE_ENV_STAGE_ATTACK = 1, 
  VOICE_ENV_STAGE_DECAY, 
  VOICE_ENV_STAGE_SUSTAIN, 
  VOICE_ENV_STAGE_RELEASE 
};

#define VOICE_ENV_KEYSCALING_DIVISOR  256
#define VOICE_ENV_TIME_KS_BREAKPOINT  (VOICE_NOTE_MIDDLE_C - 4 * 12 + 0)  /* C-0 */
#define VOICE_ENV_LEVEL_KS_BREAKPOINT (VOICE_NOTE_MIDDLE_C - 2 * 12 + 9)  /* A-2 */

/* lfo constants */
#define VOICE_LFO_WAVE_AMPLITUDE 512

/* sine wavetable (stores 1st half-period) */
static short S_voice_wavetable_sine[VOICE_PHASE_WAVE_SIZE / 2];

/* resonance window wavetable */
static short S_voice_wavetable_window[VOICE_PHASE_WAVE_SIZE / 2];

/* db to linear table */
static short S_voice_db_to_linear_table[VOICE_DB_12_BIT_SIZE];

/* oscillator phase increment table */
static unsigned int S_voice_wave_phase_increment_table[VOICE_TUNING_STEPS_PER_OCTAVE];

/* oscillator bend period table */
static unsigned int S_voice_wave_bend_period_table[VOICE_TUNING_STEPS_PER_OCTAVE];

/* envelope phase increment tables */
static unsigned int S_voice_env_attack_increment_table[VOICE_ENVELOPE_STEPS_PER_ROW];
static unsigned int S_voice_env_decay_increment_table[VOICE_ENVELOPE_STEPS_PER_ROW];

/* envelope time, level, and keyscaling tables */
static short S_voice_env_time_table[PATCH_NUM_ENV_TIME_VALS];
static short S_voice_env_level_table[PATCH_NUM_ENV_LEVEL_VALS];
static short S_voice_env_keyscaling_table[PATCH_NUM_ENV_KEYSCALING_VALS];

/* lfo phase increment table */
static unsigned int S_voice_lfo_phase_increment_table[PATCH_NUM_LFO_SPEED_VALS];

/* voice bank */
voice G_voice_bank[VOICE_NUM_VOICES];

/*******************************************************************************
** voice_reset_all()
*******************************************************************************/
short int voice_reset_all()
{
  int k;
  int m;

  voice* v;

  /* reset all voices */
  for (k = 0; k < VOICE_NUM_VOICES; k++)
  {
    /* obtain voice pointer */
    v = &G_voice_bank[k];

    /* cart & patch indices */
    v->cart_index = 0;
    v->patch_index = 0;

    /* currently playing note */
    v->base_note = 0;

    /* oscillator pairs */
    for (m = 0; m < VOICE_NUM_OSC_PAIRS; m++)
    {
      v->pair_pitch_index[m] = 0;

      v->pair_flag[m] = 0x00;

      v->pair_wave_phase[m] = 0;
      v->pair_res_phase[m] = 0;
    }

    /* envelopes */
    for (m = 0; m < VOICE_NUM_ENVS; m++)
    {
      v->env_stage[m] = VOICE_ENV_STAGE_RELEASE;

      v->env_phase[m] = 0;

      v->env_attenuation[m] = VOICE_MAX_ATTENUATION_DB;
    }

    /* lfo */
    for (m = 0; m < VOICE_NUM_LFOS; m++)
    {
      v->lfo_delay_cycles[m] = 0;

      v->lfo_phase[m] = 0;
      v->lfo_increment[m] = S_voice_lfo_phase_increment_table[0];
    }

    /* midi controller positions */
    v->pitch_wheel_pos = 0;
    v->vibrato_wheel_pos = 0;
    v->tremolo_wheel_pos = 0;
    v->boost_wheel_pos = 0;
    v->note_velocity_pos = 0;

    /* output level */
    v->level = 0;
  }

  return 0;
}

/*******************************************************************************
** voice_load_patch()
*******************************************************************************/
short int voice_load_patch( short voice_index, 
                            short cart_index, short patch_index)
{
  voice* v;

  /* make sure that the voice index is valid */
  if ((voice_index < 0) || (voice_index >= VOICE_NUM_VOICES))
    return 1;

  /* make sure that the cart & patch indices are valid */
  if ((cart_index < 0) || (cart_index >= CART_NUM_INDICES))
    return 1;

  if ((patch_index < 0) || (patch_index >= CART_NUM_PATCHES))
    return 1;

  /* obtain voice pointer */
  v = &G_voice_bank[voice_index];

  /* set cart and patch indices */
  v->cart_index = cart_index;
  v->patch_index = patch_index;

  return 0;
}

/*******************************************************************************
** voice_note_on()
*******************************************************************************/
short int voice_note_on(short voice_index, short midi_note)
{
  int m;

  voice* v;

  cart* c;
  patch* p;

  short converted_note;
  short detuned_cents;

  /* make sure that the voice index is valid */
  if ((voice_index < 0) || (voice_index >= VOICE_NUM_VOICES))
    return 1;

  /* determine note */
  converted_note = midi_note - 60 + VOICE_NOTE_MIDDLE_C;

  /* if note is out of range, ignore */
  if ((converted_note < VOICE_NOTE_LOWEST_PLAYABLE) || (converted_note > VOICE_NOTE_HIGHEST_PLAYABLE))
    return 0;

  /* obtain voice pointer */
  v = &G_voice_bank[voice_index];

  /* obtain cart & patch pointers */
  c = &G_cart_bank[v->cart_index];
  p = &(c->patches[v->patch_index]);

  /* set base note */
  v->base_note = converted_note;

  /* oscillator pairs */
  for (m = 0; m < VOICE_NUM_OSC_PAIRS; m++)
  {
    /* apply coarse detune */
    detuned_cents = 100 * converted_note;

    if ((m == VOICE_OSC_PAIR_LINE_2_UNISON_1) || 
        (m == VOICE_OSC_PAIR_LINE_2_UNISON_2))
    {
      detuned_cents += 1200 * p->values[PATCH_PARAM_LINE_2_OCTAVE];
      detuned_cents -= 1200 * (PATCH_NUM_OCTAVE_VALS / 2);

      detuned_cents +=  100 * p->values[PATCH_PARAM_LINE_2_NOTE];
      detuned_cents -=  100 * (PATCH_NUM_NOTE_VALS / 2);
    }

    /* apply fine detune */
    if ((m == VOICE_OSC_PAIR_LINE_1_UNISON_1) || 
        (m == VOICE_OSC_PAIR_LINE_1_UNISON_2))
    {
      detuned_cents += p->values[PATCH_PARAM_LINE_1_DETUNE];
      detuned_cents -= PATCH_NUM_DETUNE_VALS / 2;
    }
    else
    {
      detuned_cents += p->values[PATCH_PARAM_LINE_2_DETUNE];
      detuned_cents -= PATCH_NUM_DETUNE_VALS / 2;
    }

    if ((m == VOICE_OSC_PAIR_LINE_1_UNISON_1) || 
        (m == VOICE_OSC_PAIR_LINE_2_UNISON_1))
    {
      detuned_cents += p->values[PATCH_PARAM_UNISON_DETUNE];
      detuned_cents -= PATCH_NUM_DETUNE_VALS / 2;
    }
    else
    {
      detuned_cents -= p->values[PATCH_PARAM_UNISON_DETUNE];
      detuned_cents += PATCH_NUM_DETUNE_VALS / 2;
    }

    /* determine pitch index */
    v->pair_pitch_index[m] = (detuned_cents * VOICE_TUNING_STEPS_PER_OCTAVE) / 1200;

    if (v->pair_pitch_index[m] < 0)
      v->pair_pitch_index[m] = 0;
    else if (v->pair_pitch_index[m] >= VOICE_TUNING_MAX_PITCH_INDEX)
      v->pair_pitch_index[m] = VOICE_TUNING_MAX_PITCH_INDEX - 1;

    v->pair_flag[m] = 0x00;

    v->pair_wave_phase[m] = 0;
    v->pair_res_phase[m] = 0;
  }

  /* envelopes */
  for (m = 0; m < VOICE_NUM_ENVS; m++)
  {
    v->env_stage[m] = VOICE_ENV_STAGE_ATTACK;
    v->env_phase[m] = 0;
  }

  /* lfos */
  v->lfo_delay_cycles[VOICE_LFO_VIBRATO] = p->values[PATCH_PARAM_VIBRATO_DELAY];
  v->lfo_increment[VOICE_LFO_VIBRATO] = S_voice_lfo_phase_increment_table[p->values[PATCH_PARAM_VIBRATO_SPEED]];
  v->lfo_phase[VOICE_LFO_VIBRATO] = 0;

  v->lfo_delay_cycles[VOICE_LFO_TREMOLO] = p->values[PATCH_PARAM_TREMOLO_DELAY];
  v->lfo_increment[VOICE_LFO_TREMOLO] = S_voice_lfo_phase_increment_table[p->values[PATCH_PARAM_TREMOLO_SPEED]];
  v->lfo_phase[VOICE_LFO_TREMOLO] = 0;

  return 0;
}

/*******************************************************************************
** voice_note_off()
*******************************************************************************/
short int voice_note_off(short voice_index)
{
  int m;

  voice* v;

  cart* c;
  patch* p;

  /* make sure that the voice index is valid */
  if ((voice_index < 0) || (voice_index >= VOICE_NUM_VOICES))
    return 1;

  /* obtain voice pointer */
  v = &G_voice_bank[voice_index];

  /* obtain cart & patch pointers */
  c = &G_cart_bank[v->cart_index];
  p = &(c->patches[v->patch_index]);

  /* envelopes */
  for (m = 0; m < VOICE_NUM_ENVS; m++)
  {
    /* if this envelope is already released, continue */
    if (v->env_stage[m] == VOICE_ENV_STAGE_RELEASE)
      continue;

    /* set to release stage */
    v->env_stage[m] = VOICE_ENV_STAGE_RELEASE;
    v->env_phase[m] = 0;
  }

  return 0;
}

/*******************************************************************************
** voice_update_all()
*******************************************************************************/
short int voice_update_all()
{
  int k;
  int m;

  voice* v;

  cart* c;
  patch* p;

  short lfo_level[VOICE_NUM_LFOS];
  short env_level[VOICE_NUM_ENVS];

  short output_db[VOICE_NUM_OSC_PAIRS];
  short output_flag[VOICE_NUM_OSC_PAIRS];

  long  table_index;
  short row;
  short step;

  short periods;
  short hold_level;

  short waveform;
  short res_offset;

  unsigned int bend_period;

  unsigned int wave_index;
  unsigned int remap_index;

  int wheel_base;
  int wheel_extra;

  /* update all voices */
  for (k = 0; k < VOICE_NUM_VOICES; k++)
  {
    v = &G_voice_bank[k];

    /* obtain cart & patch pointers */
    c = &G_cart_bank[v->cart_index];
    p = &(c->patches[v->patch_index]);

#if 0
    /* update lfos */
    for (m = 0; m < VOICE_NUM_LFOS; m++)
    {
      v->lfo_phase[m] += v->lfo_increment[m];

      /* wraparound phase register */
      if (v->lfo_phase[m] >= VOICE_PHASE_REG_SIZE)
        v->lfo_phase[m] &= VOICE_PHASE_REG_MASK;;

      /* update delay cycles if necessary */
      if (v->lfo_delay_cycles[m] > 0)
      {
        v->lfo_delay_cycles[m] -= 1;
        v->lfo_phase[m] = 0;

        lfo_level[m] = 0;
        continue;
      }

      /* determine masked phase */
      wave_phase = (v->lfo_phase[m] >> VOICE_PHASE_MANTISSA_NUM_BITS) & VOICE_PHASE_WAVE_MASK;

      /* determine lfo level */
      if (m == VOICE_LFO_VIBRATO)
        waveform = p->values[PATCH_PARAM_VIBRATO_WAVEFORM];
      else
        waveform = p->values[PATCH_PARAM_TREMOLO_WAVEFORM];

      if (waveform == PATCH_LFO_WAVEFORM_VAL_TRIANGLE)
      {
        if (wave_phase < 512)
          lfo_level[m] = (wave_phase * VOICE_LFO_WAVE_AMPLITUDE) / 512;
        else if (wave_phase < 1024)
          lfo_level[m] = ((1024 - wave_phase) * VOICE_LFO_WAVE_AMPLITUDE) / 512;
        else if (wave_phase < 3 * 512)
          lfo_level[m] = -((wave_phase - 1024) * VOICE_LFO_WAVE_AMPLITUDE) / 512;
        else
          lfo_level[m] = -((2048 - wave_phase) * VOICE_LFO_WAVE_AMPLITUDE) / 512;
      }
      else if (waveform == PATCH_LFO_WAVEFORM_VAL_SQUARE)
      {
        if (wave_phase < 1024)
          lfo_level[m] = VOICE_LFO_WAVE_AMPLITUDE;
        else
          lfo_level[m] = -VOICE_LFO_WAVE_AMPLITUDE;
      }
      else if (waveform == PATCH_LFO_WAVEFORM_VAL_SAW_UP)
      {
        if (wave_phase < 1024)
          lfo_level[m] = -((1024 - wave_phase) * VOICE_LFO_WAVE_AMPLITUDE) / 1024;
        else
          lfo_level[m] = ((wave_phase - 1024) * VOICE_LFO_WAVE_AMPLITUDE) / 1024;
      }
      else if (waveform == PATCH_LFO_WAVEFORM_VAL_SAW_DOWN)
      {
        if (wave_phase < 1024)
          lfo_level[m] = ((1024 - wave_phase) * VOICE_LFO_WAVE_AMPLITUDE) / 1024;
        else
          lfo_level[m] = -((wave_phase - 1024) * VOICE_LFO_WAVE_AMPLITUDE) / 1024;
      }

      if (m == VOICE_LFO_TREMOLO)
        lfo_level[m] = (lfo_level[m] + VOICE_LFO_WAVE_AMPLITUDE) / 2;
    }
#endif

#if 0
    /* determine vibrato adjustment */
    wheel_extra = lfo_level[VOICE_LFO_VIBRATO];

    wheel_extra *= S_voice_vibrato_max_table[p->values[PATCH_PARAM_VIBRATO_SENSITIVITY]];
    wheel_extra /= VOICE_LFO_WAVE_AMPLITUDE;

    wheel_base = wheel_extra;
    wheel_base *= p->values[PATCH_PARAM_VIBRATO_DEPTH];
    wheel_base /= G_patch_param_bounds[PATCH_PARAM_VIBRATO_DEPTH];

    wheel_extra -= wheel_base;

    vibrato_adjustment = wheel_base;
    vibrato_adjustment += 
      (wheel_extra * v->vibrato_wheel_pos) / 100;

    /* determine tremolo adjustment */

    /* add on boost adjustment */
#endif

    /* update envelopes */
    for (m = 0; m < VOICE_NUM_ENVS; m++)
    {
      /* update phase */
      if ((m == VOICE_ENV_LINE_1_AMPLITUDE) || 
          (m == VOICE_ENV_LINE_2_AMPLITUDE))
      {
        if (v->env_stage[m] == VOICE_ENV_STAGE_ATTACK)
          table_index = S_voice_env_time_table[p->values[PATCH_PARAM_AMP_ENV_ATTACK]];
        else if (v->env_stage[m] == VOICE_ENV_STAGE_DECAY)
          table_index = S_voice_env_time_table[p->values[PATCH_PARAM_AMP_ENV_DECAY]];
        else if (v->env_stage[m] == VOICE_ENV_STAGE_SUSTAIN)
          table_index = S_voice_env_time_table[p->values[PATCH_PARAM_AMP_ENV_SUSTAIN]];
        else
          table_index = S_voice_env_time_table[p->values[PATCH_PARAM_AMP_ENV_RELEASE]];
      }
      else
      {
        if (v->env_stage[m] == VOICE_ENV_STAGE_ATTACK)
          table_index = S_voice_env_time_table[p->values[PATCH_PARAM_BEND_ENV_ATTACK]];
        else if (v->env_stage[m] == VOICE_ENV_STAGE_DECAY)
          table_index = S_voice_env_time_table[p->values[PATCH_PARAM_BEND_ENV_DECAY]];
        else if (v->env_stage[m] == VOICE_ENV_STAGE_SUSTAIN)
          table_index = S_voice_env_time_table[p->values[PATCH_PARAM_BEND_ENV_SUSTAIN]];
        else
          table_index = S_voice_env_time_table[p->values[PATCH_PARAM_BEND_ENV_RELEASE]];
      }

      row = table_index / VOICE_ENVELOPE_STEPS_PER_ROW;
      step = table_index % VOICE_ENVELOPE_STEPS_PER_ROW;

      if (row < (VOICE_ENVELOPE_NUM_ROWS - 1))
        v->env_phase[m] += S_voice_env_decay_increment_table[step] >> (VOICE_ENVELOPE_NUM_ROWS - 1 - row);
      else
        v->env_phase[m] += S_voice_env_decay_increment_table[step];

      /* wraparound phase register */
      if (v->env_phase[m] >= VOICE_PHASE_REG_SIZE)
      {
        periods = v->env_phase[m] >> VOICE_PHASE_REG_NUM_BITS;

        v->env_phase[m] &= VOICE_PHASE_REG_MASK;
      }
      else
        periods = 0;

      /* if a period has elapsed, update the envelope */
      while (periods > 0)
      {
        periods -= 1;

        if (v->env_stage[m] == VOICE_ENV_STAGE_ATTACK)
          v->env_attenuation[m] = (127 * v->env_attenuation[m]) / 128;
        else
          v->env_attenuation[m] += 1;

        /* bound attenuation */
        if (v->env_attenuation[m] < 0)
          v->env_attenuation[m] = 0;
        else if (v->env_attenuation[m] > VOICE_MAX_ATTENUATION_DB)
          v->env_attenuation[m] = VOICE_MAX_ATTENUATION_DB;

        /* determine hold level */
        if ((m == VOICE_ENV_LINE_1_AMPLITUDE) || 
            (m == VOICE_ENV_LINE_2_AMPLITUDE))
        {
          hold_level = S_voice_env_level_table[p->values[PATCH_PARAM_AMP_ENV_HOLD]];
        }
        else
          hold_level = S_voice_env_level_table[p->values[PATCH_PARAM_BEND_ENV_HOLD]];

        /* check for stage changes */
        if ((v->env_stage[m] == VOICE_ENV_STAGE_ATTACK) && 
            (v->env_attenuation[m] == 0))
        {
          v->env_stage[m] = VOICE_ENV_STAGE_DECAY;
          v->env_phase[m] = 0;
        }
        else if ( (v->env_stage[m] == VOICE_ENV_STAGE_DECAY) && 
                  (v->env_attenuation[m] >= hold_level))
        {
          v->env_stage[m] = VOICE_ENV_STAGE_SUSTAIN;
          v->env_phase[m] = 0;
        }
      }

      /* update level */
      env_level[m] = v->env_attenuation[m];

      /* bound level */
      if (env_level[m] < 0)
        env_level[m] = 0;
      else if (env_level[m] > VOICE_MAX_ATTENUATION_DB)
        env_level[m] = VOICE_MAX_ATTENUATION_DB;
    }

    /* update oscillator pairs */
    for (m = 0; m < VOICE_NUM_OSC_PAIRS; m++)
    {
      /* determine resonance pitch and bend period from bend envelope */
      if ((m == VOICE_OSC_PAIR_LINE_1_UNISON_1) || 
          (m == VOICE_OSC_PAIR_LINE_1_UNISON_2))
      {
        res_offset = 4095 - env_level[VOICE_ENV_LINE_1_BEND];
      }
      else
        res_offset = 4095 - env_level[VOICE_ENV_LINE_2_BEND];

#if 0
      res_offset *= VOICE_TUNING_STEPS_PER_OCTAVE;
      res_offset /= VOICE_ENVELOPE_STEPS_PER_ROW;
#endif

      bend_period = S_voice_wave_bend_period_table[res_offset % VOICE_TUNING_STEPS_PER_OCTAVE];

      if ((res_offset / VOICE_TUNING_STEPS_PER_OCTAVE) > 0)
        bend_period = bend_period >> (res_offset / VOICE_TUNING_STEPS_PER_OCTAVE);

      /* determine wave pitch index */
      table_index = v->pair_pitch_index[m];

      if (table_index < 0)
        table_index = 0;
      else if (table_index >= VOICE_TUNING_MAX_PITCH_INDEX)
        table_index = VOICE_TUNING_MAX_PITCH_INDEX - 1;

      row = table_index / VOICE_TUNING_STEPS_PER_OCTAVE;
      step = table_index % VOICE_TUNING_STEPS_PER_OCTAVE;

      /* add phase increment (wave) */
      if (row < (VOICE_TUNING_NUM_OCTAVES - 1))
      {
        v->pair_wave_phase[m] += 
          S_voice_wave_phase_increment_table[step] >> (VOICE_TUNING_NUM_OCTAVES - 1 - row);
      }
      else
      {
        v->pair_wave_phase[m] += 
          S_voice_wave_phase_increment_table[step];
      }

      /* wraparound phase register (wave) */
      if (v->pair_wave_phase[m] >= VOICE_PHASE_REG_SIZE)
      {
        v->pair_wave_phase[m] &= VOICE_PHASE_REG_MASK;

        v->pair_res_phase[m] = v->pair_wave_phase[m];
        v->pair_flag[m] ^= 0x01;
      }

      /* adjust for resonance pitch */
      table_index += res_offset;

      if (table_index < 0)
        table_index = 0;
      else if (table_index >= VOICE_TUNING_MAX_PITCH_INDEX)
        table_index = VOICE_TUNING_MAX_PITCH_INDEX - 1;

      row = table_index / VOICE_TUNING_STEPS_PER_OCTAVE;
      step = table_index % VOICE_TUNING_STEPS_PER_OCTAVE;

      /* add phase increment (resonance) */
      if (row < (VOICE_TUNING_NUM_OCTAVES - 1))
      {
        v->pair_res_phase[m] += 
          S_voice_wave_phase_increment_table[step] >> (VOICE_TUNING_NUM_OCTAVES - 1 - row);
      }
      else
      {
        v->pair_res_phase[m] += 
          S_voice_wave_phase_increment_table[step];
      }

      /* wraparound phase register (resonance) */
      if (v->pair_res_phase[m] >= VOICE_PHASE_REG_SIZE)
        v->pair_res_phase[m] &= VOICE_PHASE_REG_MASK;

      /* determine waveform, and limit the bend amount in the case of double sine */
      if ((m == VOICE_OSC_PAIR_LINE_1_UNISON_1) || 
          (m == VOICE_OSC_PAIR_LINE_1_UNISON_2))
      {
        if (v->pair_flag[m] & 0x01)
          waveform = p->values[PATCH_PARAM_LINE_1_WAVE_2];
        else
          waveform = p->values[PATCH_PARAM_LINE_1_WAVE_1];

        if ((p->values[PATCH_PARAM_LINE_1_WAVE_1] == PATCH_WAVE_VAL_DOUBLE_SINE) || 
            (p->values[PATCH_PARAM_LINE_1_WAVE_2] == PATCH_WAVE_VAL_DOUBLE_SINE))
        {
          if (bend_period >= (VOICE_PHASE_WAVE_SIZE / 2))
            bend_period = VOICE_PHASE_WAVE_SIZE / 2;
        }
      }
      else
      {
        if (v->pair_flag[m] & 0x01)
          waveform = p->values[PATCH_PARAM_LINE_2_WAVE_2];
        else
          waveform = p->values[PATCH_PARAM_LINE_2_WAVE_1];

        if ((p->values[PATCH_PARAM_LINE_2_WAVE_1] == PATCH_WAVE_VAL_DOUBLE_SINE) || 
            (p->values[PATCH_PARAM_LINE_2_WAVE_2] == PATCH_WAVE_VAL_DOUBLE_SINE))
        {
          if (bend_period >= (VOICE_PHASE_WAVE_SIZE / 2))
            bend_period = VOICE_PHASE_WAVE_SIZE / 2;
        }
      }

      /* set wavetable indices */
      wave_index = (v->pair_wave_phase[m] >> VOICE_PHASE_MANTISSA_NUM_BITS) & VOICE_PHASE_WAVE_MASK;
      remap_index = wave_index;

      /* saw */
      if (waveform == PATCH_WAVE_VAL_SAW)
      {
        if (wave_index < (bend_period / 4))
        {
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= bend_period;
        }
        else if (wave_index < (VOICE_PHASE_WAVE_SIZE - (bend_period / 4)))
        {
          remap_index -= bend_period / 4;
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= 2 * VOICE_PHASE_WAVE_SIZE - bend_period;
          remap_index += VOICE_PHASE_WAVE_SIZE / 4;
        }
        else
        {
          remap_index -= VOICE_PHASE_WAVE_SIZE - (bend_period / 4);
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= bend_period;
          remap_index += (3 * VOICE_PHASE_WAVE_SIZE) / 4;
        }
      }
      /* square */
      else if (waveform == PATCH_WAVE_VAL_SQUARE)
      {
        if (wave_index < (bend_period / 4))
        {
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= bend_period;
        }
        else if (wave_index < ((VOICE_PHASE_WAVE_SIZE / 2) - (bend_period / 4)))
          remap_index = VOICE_PHASE_WAVE_SIZE / 4;
        else if (wave_index < ((VOICE_PHASE_WAVE_SIZE / 2) + (bend_period / 4)))
        {
          remap_index -= (VOICE_PHASE_WAVE_SIZE / 2) - (bend_period / 4);
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= bend_period;
          remap_index += VOICE_PHASE_WAVE_SIZE / 4;
        }
        else if (wave_index < (VOICE_PHASE_WAVE_SIZE - (bend_period / 4)))
          remap_index = (3 * VOICE_PHASE_WAVE_SIZE) / 4;
        else
        {
          remap_index -= VOICE_PHASE_WAVE_SIZE - (bend_period / 4);
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= bend_period;
          remap_index += (3 * VOICE_PHASE_WAVE_SIZE) / 4;
        }
      }
      /* pulse */
      else if (waveform == PATCH_WAVE_VAL_PULSE)
      {
        if (wave_index < ((3 * bend_period) / 4))
        {
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= bend_period;
        }
        else if (wave_index < (VOICE_PHASE_WAVE_SIZE - (bend_period / 4)))
          remap_index = (3 * VOICE_PHASE_WAVE_SIZE) / 4;
        else
        {
          remap_index -= VOICE_PHASE_WAVE_SIZE - (bend_period / 4);
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= bend_period;
          remap_index += (3 * VOICE_PHASE_WAVE_SIZE) / 4;
        }
      }
      /* double sine */
      else if (waveform == PATCH_WAVE_VAL_DOUBLE_SINE)
      {
        if (wave_index < ((3 * bend_period) / 4))
        {
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= bend_period;
        }
        else if (wave_index < (VOICE_PHASE_WAVE_SIZE - (bend_period / 4)))
        {
          remap_index -= (3 * bend_period) / 4;
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= VOICE_PHASE_WAVE_SIZE - bend_period;
          remap_index += (3 * VOICE_PHASE_WAVE_SIZE) / 4;
          remap_index %= VOICE_PHASE_WAVE_SIZE;
        }
        else
        {
          remap_index -= VOICE_PHASE_WAVE_SIZE - (bend_period / 4);
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= bend_period;
          remap_index += (3 * VOICE_PHASE_WAVE_SIZE) / 4;
        }
      }
      /* half saw */
      else if (waveform == PATCH_WAVE_VAL_HALF_SAW)
      {
        if (wave_index < (bend_period / 4))
        {
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= bend_period;
        }
        else if (wave_index < ((VOICE_PHASE_WAVE_SIZE / 2) - (bend_period / 4)))
          remap_index = VOICE_PHASE_WAVE_SIZE / 4;
        else if (wave_index < (VOICE_PHASE_WAVE_SIZE - (bend_period / 4)))
        {
          remap_index -= (VOICE_PHASE_WAVE_SIZE / 2) - (bend_period / 4);
          remap_index += VOICE_PHASE_WAVE_SIZE / 4;
        }
        else
        {
          remap_index -= VOICE_PHASE_WAVE_SIZE - (bend_period / 4);
          remap_index *= VOICE_PHASE_WAVE_SIZE;
          remap_index /= bend_period;
          remap_index += (3 * VOICE_PHASE_WAVE_SIZE) / 4;
        }
      }
      /* resonance waveforms */
      else if ( (waveform == PATCH_WAVE_VAL_RESONANCE_SAW)      || 
                (waveform == PATCH_WAVE_VAL_RESONANCE_TRIANGLE) || 
                (waveform == PATCH_WAVE_VAL_RESONANCE_TRAPEZOID))
      {
        remap_index = (v->pair_res_phase[m] >> VOICE_PHASE_MANTISSA_NUM_BITS) & VOICE_PHASE_WAVE_MASK;
      }

      /* wavetable lookup */
      output_db[m] = S_voice_wavetable_sine[remap_index % (VOICE_PHASE_WAVE_SIZE / 2)];

      if ((m == VOICE_OSC_PAIR_LINE_1_UNISON_1) || 
          (m == VOICE_OSC_PAIR_LINE_1_UNISON_2))
      {
        output_db[m] += env_level[VOICE_ENV_LINE_1_AMPLITUDE];
      }
      else
      {
        output_db[m] += env_level[VOICE_ENV_LINE_2_AMPLITUDE];
      }

      /* resonance window */
      if (waveform == PATCH_WAVE_VAL_RESONANCE_SAW)
        output_db[m] += S_voice_wavetable_window[(wave_index / 2) % (VOICE_PHASE_WAVE_SIZE / 2)];
      else if (waveform == PATCH_WAVE_VAL_RESONANCE_TRIANGLE)
      {
        if (wave_index < (VOICE_PHASE_WAVE_SIZE / 2))
          output_db[m] += S_voice_wavetable_window[((VOICE_PHASE_WAVE_SIZE / 2) - wave_index) % (VOICE_PHASE_WAVE_SIZE / 2)];
        else
          output_db[m] += S_voice_wavetable_window[(wave_index - (VOICE_PHASE_WAVE_SIZE / 2)) % (VOICE_PHASE_WAVE_SIZE / 2)];
      }
      else if (waveform == PATCH_WAVE_VAL_RESONANCE_TRAPEZOID)
      {
        if (wave_index < (VOICE_PHASE_WAVE_SIZE / 2))
          output_db[m] += 0;
        else
          output_db[m] += S_voice_wavetable_window[(wave_index - (VOICE_PHASE_WAVE_SIZE / 2)) % (VOICE_PHASE_WAVE_SIZE / 2)];
      }

      if (output_db[m] < 0)
        output_db[m] = 0;
      else if (output_db[m] > VOICE_MAX_ATTENUATION_DB)
        output_db[m] = VOICE_MAX_ATTENUATION_DB;

      /* set sign flag */
      if (remap_index < (VOICE_PHASE_WAVE_SIZE / 2))
        output_flag[m] &= ~0x01;
      else
        output_flag[m] |= 0x01;
    }

    /* for now, just output the main line */
    if (output_flag[VOICE_OSC_PAIR_LINE_1_UNISON_1] & 0x01)
      v->level = -S_voice_db_to_linear_table[output_db[VOICE_OSC_PAIR_LINE_1_UNISON_1]];
    else
      v->level = S_voice_db_to_linear_table[output_db[VOICE_OSC_PAIR_LINE_1_UNISON_1]];
  }

  return 0;
}

/*******************************************************************************
** voice_generate_tables()
*******************************************************************************/
short int voice_generate_tables()
{
  int m;

  double val;

  /* 12 bit envelope & waveform values for each bit, in db:                 */
  /* 3(8), 3(4), 3(2), 3(1), 3/2, 3/4, 3/8, 3/16, 3/32, 3/64, 3/128, 3/256  */

  /* db to linear scale conversion */
  S_voice_db_to_linear_table[0] = VOICE_MAX_VOLUME_LINEAR;
  S_voice_db_to_linear_table[VOICE_DB_12_BIT_SIZE - 1] = VOICE_MAX_ATTENUATION_LINEAR;

  for (m = 1; m < VOICE_DB_12_BIT_SIZE - 1; m++)
  {
    S_voice_db_to_linear_table[m] = 
      (short) ((VOICE_MAX_VOLUME_LINEAR * exp(-log(10) * (VOICE_DB_STEP_12_BIT / 10) * m)) + 0.5f);
  }

  /* sine wavetable (just has the 1st half period) */
  S_voice_wavetable_sine[0] = VOICE_MAX_ATTENUATION_DB;
  S_voice_wavetable_sine[VOICE_PHASE_WAVE_SIZE / 4] = VOICE_MAX_VOLUME_DB;

  for (m = 1; m < (VOICE_PHASE_WAVE_SIZE / 4); m++)
  {
    val = sin((2.0f * PI * m) / VOICE_PHASE_WAVE_SIZE);
    S_voice_wavetable_sine[m] = (short) ((10 * (-log(val) / log(10)) / VOICE_DB_STEP_12_BIT) + 0.5f);
    S_voice_wavetable_sine[(VOICE_PHASE_WAVE_SIZE / 2) - m] = S_voice_wavetable_sine[m];
  }

  /* resonance window wavetable (just has the 1st half period) */
  S_voice_wavetable_window[0] = VOICE_MAX_VOLUME_DB;

  for (m = 1; m < (VOICE_PHASE_WAVE_SIZE / 2); m++)
  {
    val = (VOICE_PHASE_WAVE_SIZE - (2.0f * m)) / VOICE_PHASE_WAVE_SIZE;
    S_voice_wavetable_window[m] = (short) ((10 * (-log(val) / log(10)) / VOICE_DB_STEP_12_BIT) + 0.5f);
  }

#if 0
  for (m = 0; m < (VOICE_PHASE_WAVE_SIZE / 2); m++)
    printf("Resonance Window Wavetable Index %d: %d\n", m, S_voice_wavetable_window[m]);
#endif

  /* oscillator phase increment table */
  /* this contains the phase increments for the highest octave, C9 - B9 */
  for (m = 0; m < VOICE_TUNING_STEPS_PER_OCTAVE; m++)
  {
    val = 440.0f * exp(-log(2) * (9.0f / 12.0f));
    val *= 32 * exp(log(2) * ((float) m / VOICE_TUNING_STEPS_PER_OCTAVE));

    S_voice_wave_phase_increment_table[m] = 
      (unsigned int) ((val * VOICE_1HZ_PHASE_INCREMENT) + 0.5f);
  }

#if 0
  for (m = 0; m < VOICE_TUNING_STEPS_PER_OCTAVE; m += 4)
    printf("Osc Phase Increment Index %d: %d\n", m, S_voice_wave_phase_increment_table[m]);
#endif

  /* oscillator bend period table */
  for (m = 0; m < VOICE_TUNING_STEPS_PER_OCTAVE; m++)
  {
    val = exp(-log(2) * ((float) m / VOICE_TUNING_STEPS_PER_OCTAVE));
    val *= VOICE_PHASE_WAVE_SIZE;

    S_voice_wave_bend_period_table[m] = (unsigned int) (val + 0.5f);
  }

#if 0
  for (m = 0; m < VOICE_TUNING_STEPS_PER_OCTAVE; m += 4)
    printf("Osc Bend Period Index %d: %d\n", m, S_voice_wave_bend_period_table[m]);
#endif

  /* envelope time, level, and keyscaling tables */
  for (m = 0; m < PATCH_NUM_ENV_TIME_VALS; m++)
    S_voice_env_time_table[m] = (VOICE_ENVELOPE_STEPS_PER_ROW * (PATCH_NUM_ENV_TIME_VALS - 1 - m + 4)) / 8;

  /* note that adding 32 to the 12-bit envelope   */
  /* is the same as adding 1 to a 7-bit envelope. */
  S_voice_env_level_table[0] = VOICE_MAX_ATTENUATION_DB;

  for (m = 1; m < PATCH_NUM_ENV_LEVEL_VALS; m++)
    S_voice_env_level_table[m] = (PATCH_NUM_ENV_LEVEL_VALS - 1 - m) * 16;

  for (m = 0; m < PATCH_NUM_ENV_KEYSCALING_VALS; m++)
  {
    val = exp(log(2) * ((3.0f * m) / PATCH_NUM_ENV_KEYSCALING_VALS));
    val *= VOICE_ENV_KEYSCALING_DIVISOR / 8;

    S_voice_env_keyscaling_table[m] = (short) (val + 0.5f);
  }

  /* envelope phase increment tables  */

  /* for the decay stage, the fastest rate should have a fall time of 8 ms  */
  /* this occurs over 4095 updates per fall time (with a 12 bit envelope)   */
  for (m = 0; m < VOICE_ENVELOPE_STEPS_PER_ROW; m++)
  {
    val = (VOICE_MAX_ATTENUATION_DB / 0.016f);
    val *= exp(log(2) * ((float) m / VOICE_ENVELOPE_STEPS_PER_ROW));

    S_voice_env_decay_increment_table[m] = 
      (int) ((val * VOICE_1HZ_PHASE_INCREMENT) + 0.5f);
  }

  /* for the attack stage, the fastest rate should have a rise time of 4 ms */
  /* this occurs over 518 updates per rise time (with a 12 bit envelope)    */
  for (m = 0; m < VOICE_ENVELOPE_STEPS_PER_ROW; m++)
  {
    val = 518 / 0.008f;
    val *= exp(log(2) * ((float) m / VOICE_ENVELOPE_STEPS_PER_ROW));

    S_voice_env_attack_increment_table[m] = 
      (int) ((val * VOICE_1HZ_PHASE_INCREMENT) + 0.5f);
  }

  /* lfo phase increment table */
  /* the frequencies range from 0.5 hz to 8.5 hz (in 100 steps) */
  for (m = 0; m < PATCH_NUM_LFO_SPEED_VALS; m++)
  {
    S_voice_lfo_phase_increment_table[m] = 
      (int) (((0.5f + ((8.0f * m) / PATCH_NUM_LFO_SPEED_VALS)) * VOICE_1HZ_PHASE_INCREMENT) + 0.5f);
  }

#if 0
  /* sensitivity tables */
  for (m = 0; m < PATCH_NUM_SENSITIVITY_VALS; m++)
    S_voice_vibrato_max_table[m] = (m + 1) * 2;

  for (m = 0; m < PATCH_NUM_SENSITIVITY_VALS; m++)
    S_voice_tremolo_max_table[m] = (m + 1) * 4 * 32;

  for (m = 0; m < PATCH_NUM_SENSITIVITY_VALS; m++)
    S_voice_boost_max_table[m] = (m + 1) * 1 * 32;

  for (m = 0; m < PATCH_NUM_SENSITIVITY_VALS; m++)
    S_voice_velocity_max_table[m] = (m + 1) * 1 * 32;
#endif

  return 0;
}

