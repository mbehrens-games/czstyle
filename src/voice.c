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

/* phase register */
#define VOICE_PHASE_REG_NUM_BITS  24
#define VOICE_PHASE_REG_SIZE      (1 << VOICE_PHASE_REG_NUM_BITS)
#define VOICE_PHASE_REG_MASK      (VOICE_PHASE_REG_SIZE - 1)

#define VOICE_PHASE_WAVE_NUM_BITS 11
#define VOICE_PHASE_WAVE_SIZE     (1 << VOICE_PHASE_WAVE_NUM_BITS)
#define VOICE_PHASE_WAVE_MASK     (VOICE_PHASE_WAVE_SIZE - 1)

#define VOICE_PHASE_MANTISSA_NUM_BITS  (VOICE_PHASE_REG_NUM_BITS - VOICE_PHASE_WAVE_NUM_BITS)

#define VOICE_PHASE_WAVE_SIZE_HALF    (VOICE_PHASE_WAVE_SIZE / 2)
#define VOICE_PHASE_WAVE_SIZE_QUARTER (VOICE_PHASE_WAVE_SIZE / 4)

#define VOICE_1HZ_PHASE_INCREMENT ((float) VOICE_PHASE_REG_SIZE / CLOCK_SAMPLING_RATE)

/* sine wavetable, db to linear table constants */
#define VOICE_DB_STEP_12_BIT 0.01171875f

#define VOICE_DB_12_BIT_NUM_BITS  12
#define VOICE_DB_12_BIT_SIZE      (1 << VOICE_DB_12_BIT_NUM_BITS)
#define VOICE_DB_12_BIT_MASK      (VOICE_DB_12_BIT_SIZE - 1)

#define VOICE_MAX_VOLUME_DB       0
#define VOICE_MAX_ATTENUATION_DB  (VOICE_DB_12_BIT_SIZE - 1)

#define VOICE_MAX_VOLUME_LINEAR       32767
#define VOICE_MAX_ATTENUATION_LINEAR  0

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

/* envelopes */
#define VOICE_ENVELOPE_STEPS_PER_ROW  1024
#define VOICE_ENVELOPE_NUM_ROWS       13    /* 8 times per row, with 100 times total = 12.5 rows */

#define VOICE_ENVELOPE_MAX_TIME_INDEX (VOICE_ENVELOPE_NUM_ROWS * VOICE_ENVELOPE_STEPS_PER_ROW)

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

/* db to linear table */
static short S_voice_db_to_linear_table[VOICE_DB_12_BIT_SIZE];

/* sine wavetable (stores 1st half-period) */
static short S_voice_wavetable_sine[VOICE_PHASE_WAVE_SIZE / 2];

/* resonance window wavetable */
static short S_voice_wavetable_window[VOICE_PHASE_WAVE_SIZE / 2];

/* wave mix table */
static short S_voice_wave_mix_table[PATCH_NUM_MIX_VALS + 1];

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
static unsigned int S_voice_lfo_speed_table[PATCH_NUM_LFO_SPEED_VALS];

/* lfo delay, sensitivity tables */
static short S_voice_lfo_delay_table[PATCH_NUM_LFO_DELAY_VALS];

static short S_voice_vibrato_sensitivity_table[PATCH_NUM_LFO_SENSITIVITY_VALS];
static short S_voice_tremolo_sensitivity_table[PATCH_NUM_LFO_SENSITIVITY_VALS];

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
    }

    /* midi controller positions */
    v->pitch_wheel_pos = 0;
    v->vibrato_wheel_pos = 0;
    v->tremolo_wheel_pos = 0;
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
short int voice_note_on(short voice_index, short midi_note, short velocity)
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

  /* set note velocity */
  if ((velocity >= 0) && (velocity < 128))
    v->note_velocity_pos = velocity;
  else
    v->note_velocity_pos = 96;

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
  v->lfo_phase[VOICE_LFO_VIBRATO] = 0;

  v->lfo_delay_cycles[VOICE_LFO_TREMOLO] = p->values[PATCH_PARAM_TREMOLO_DELAY];
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

  short remap_pos;
  short lfo_max;

  short vibrato_adjustment;
  short tremolo_adjustment;
  short velocity_adjustment;

  short periods;
  short hold_level;

  short waveform;
  short res_offset;

  unsigned int bend_period;

  unsigned int wave_index;
  unsigned int remap_index;

  /* update all voices */
  for (k = 0; k < VOICE_NUM_VOICES; k++)
  {
    v = &G_voice_bank[k];

    /* obtain cart & patch pointers */
    c = &G_cart_bank[v->cart_index];
    p = &(c->patches[v->patch_index]);

    /* update lfos */
    for (m = 0; m < VOICE_NUM_LFOS; m++)
    {
      if (m == VOICE_LFO_VIBRATO)
        v->lfo_phase[m] += S_voice_lfo_speed_table[p->values[PATCH_PARAM_VIBRATO_SPEED]];
      else
        v->lfo_phase[m] += S_voice_lfo_speed_table[p->values[PATCH_PARAM_TREMOLO_SPEED]];

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

      /* set wavetable index */
      table_index = (v->lfo_phase[m] >> VOICE_PHASE_MANTISSA_NUM_BITS) & VOICE_PHASE_WAVE_MASK;

      /* determine lfo level */
      if (m == VOICE_LFO_VIBRATO)
      {
        lfo_level[m] = S_voice_vibrato_sensitivity_table[p->values[PATCH_PARAM_VIBRATO_SENSITIVITY]];

        if (p->values[PATCH_PARAM_VIBRATO_WAVEFORM] == PATCH_LFO_WAVEFORM_VAL_TRIANGLE)
        {
          if (table_index < VOICE_PHASE_WAVE_SIZE_QUARTER)
            lfo_level[m] = (table_index * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE_QUARTER;
          else if (table_index < (2 * VOICE_PHASE_WAVE_SIZE_QUARTER))
            lfo_level[m] = ((VOICE_PHASE_WAVE_SIZE_HALF - table_index) * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE_QUARTER;
          else if (table_index < (3 * VOICE_PHASE_WAVE_SIZE_QUARTER))
            lfo_level[m] = -((table_index - VOICE_PHASE_WAVE_SIZE_HALF) * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE_QUARTER;
          else
            lfo_level[m] = -((VOICE_PHASE_WAVE_SIZE - table_index) * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE_QUARTER;
        }
        else if (p->values[PATCH_PARAM_VIBRATO_WAVEFORM] == PATCH_LFO_WAVEFORM_VAL_TRIANGLE)
        {
          if (table_index >= VOICE_PHASE_WAVE_SIZE_HALF)
            lfo_level[m] = -lfo_level[m];
        }
        else if (p->values[PATCH_PARAM_VIBRATO_WAVEFORM] == PATCH_LFO_WAVEFORM_VAL_SAW_UP)
        {
          if (table_index < VOICE_PHASE_WAVE_SIZE_HALF)
            lfo_level[m] = (table_index * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE_HALF;
          else
            lfo_level[m] = -((VOICE_PHASE_WAVE_SIZE - table_index) * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE_HALF;;
        }
        else if (p->values[PATCH_PARAM_VIBRATO_WAVEFORM] == PATCH_LFO_WAVEFORM_VAL_SAW_DOWN)
        {
          if (table_index < VOICE_PHASE_WAVE_SIZE_HALF)
            lfo_level[m] = -(table_index * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE_HALF;
          else
            lfo_level[m] = ((VOICE_PHASE_WAVE_SIZE - table_index) * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE_HALF;;
        }
        else
          lfo_level[m] = 0;
      }
      else
      {
        lfo_level[m] = S_voice_tremolo_sensitivity_table[p->values[PATCH_PARAM_TREMOLO_SENSITIVITY]];

        if (p->values[PATCH_PARAM_TREMOLO_WAVEFORM] == PATCH_LFO_WAVEFORM_VAL_TRIANGLE)
        {
          if (table_index < VOICE_PHASE_WAVE_SIZE_HALF)
            lfo_level[m] = (table_index * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE_HALF;
          else
            lfo_level[m] = ((VOICE_PHASE_WAVE_SIZE - table_index) * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE_HALF;
        }
        else if (p->values[PATCH_PARAM_TREMOLO_WAVEFORM] == PATCH_LFO_WAVEFORM_VAL_SQUARE)
        {
          if (table_index >= VOICE_PHASE_WAVE_SIZE_HALF)
            lfo_level[m] = 0;
        }
        else if (p->values[PATCH_PARAM_TREMOLO_WAVEFORM] == PATCH_LFO_WAVEFORM_VAL_SAW_UP)
          lfo_level[m] = (table_index * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE;
        else if (p->values[PATCH_PARAM_TREMOLO_WAVEFORM] == PATCH_LFO_WAVEFORM_VAL_SAW_DOWN)
          lfo_level[m] = ((VOICE_PHASE_WAVE_SIZE - table_index) * lfo_level[m]) / VOICE_PHASE_WAVE_SIZE;
        else
          lfo_level[m] = 0;
      }
    }

    /* determine vibrato adjustment */
    remap_pos = 
      (v->vibrato_wheel_pos * (PATCH_NUM_LFO_DEPTH_VALS - 1 - p->values[PATCH_PARAM_VIBRATO_DEPTH])) / PATCH_NUM_LFO_DEPTH_VALS;

    remap_pos += 
      (128 * p->values[PATCH_PARAM_VIBRATO_DEPTH]) / PATCH_NUM_LFO_DEPTH_VALS;

    if (remap_pos < 0)
      remap_pos = 0;
    else if (remap_pos > 127)
      remap_pos = 127;

    vibrato_adjustment = (remap_pos * lfo_level[VOICE_LFO_VIBRATO]) / 128;

    /* determine tremolo adjustment */
    remap_pos = 
      (v->tremolo_wheel_pos * (PATCH_NUM_LFO_DEPTH_VALS - 1 - p->values[PATCH_PARAM_TREMOLO_DEPTH])) / PATCH_NUM_LFO_DEPTH_VALS;

    remap_pos += 
      (128 * p->values[PATCH_PARAM_TREMOLO_DEPTH]) / PATCH_NUM_LFO_DEPTH_VALS;

    if (remap_pos < 0)
      remap_pos = 0;
    else if (remap_pos > 127)
      remap_pos = 127;

    tremolo_adjustment = (remap_pos * lfo_level[VOICE_LFO_TREMOLO]) / 128;

    /* determine velocity adjustment */
    remap_pos = v->note_velocity_pos;

    remap_pos = 
      (remap_pos * 2 * p->values[PATCH_PARAM_VELOCITY_DEPTH]) / PATCH_NUM_VELOCITY_DEPTH_VALS;

    remap_pos -= 128;
    remap_pos += 
      (2 * 128 * p->values[PATCH_PARAM_VELOCITY_OFFSET]) / PATCH_NUM_VELOCITY_OFFSET_VALS;

    if (remap_pos < 0)
      remap_pos = 0;
    else if (remap_pos > 127)
      remap_pos = 127;

    velocity_adjustment = remap_pos * 32;

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

      /* apply overall bend envelope adjustment */
      if (m == VOICE_ENV_LINE_1_BEND)
        env_level[m] += S_voice_env_level_table[p->values[PATCH_PARAM_LINE_1_BEND_MAX]];
      else if (m == VOICE_ENV_LINE_2_BEND)
        env_level[m] += S_voice_env_level_table[p->values[PATCH_PARAM_LINE_2_BEND_MAX]];

      /* apply tremolo adjustment */
      if ((m == VOICE_ENV_LINE_1_AMPLITUDE) || 
          (m == VOICE_ENV_LINE_2_AMPLITUDE))
      {
        env_level[m] += tremolo_adjustment;
      }

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

      table_index += vibrato_adjustment;

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

      if (remap_index < (VOICE_PHASE_WAVE_SIZE / 2))
        output_flag[m] &= ~0x01;
      else
        output_flag[m] |= 0x01;

      /* apply ring mod */
      if (p->values[PATCH_PARAM_OUTPUT_RING_MOD] == PATCH_RING_MOD_VAL_ON)
      {
        if (m == VOICE_OSC_PAIR_LINE_2_UNISON_1)
        {
          output_db[m] += output_db[VOICE_OSC_PAIR_LINE_1_UNISON_1];
          output_flag[m] ^= output_flag[VOICE_OSC_PAIR_LINE_1_UNISON_1];
        }
        else if (m == VOICE_OSC_PAIR_LINE_2_UNISON_2)
        {
          output_db[m] += output_db[VOICE_OSC_PAIR_LINE_1_UNISON_2];
          output_flag[m] ^= output_flag[VOICE_OSC_PAIR_LINE_1_UNISON_2];
        }
      }

      /* apply envelope */
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

      /* bound output level */
      if (output_db[m] < 0)
        output_db[m] = 0;
      else if (output_db[m] > VOICE_MAX_ATTENUATION_DB)
        output_db[m] = VOICE_MAX_ATTENUATION_DB;
    }

    /* mixing */
    for (m = 0; m < VOICE_NUM_OSC_PAIRS; m++)
    {
      /* apply wave mix attenuations */
      if ((m == VOICE_OSC_PAIR_LINE_1_UNISON_1) || 
          (m == VOICE_OSC_PAIR_LINE_1_UNISON_2))
      {
        output_db[m] += S_voice_wave_mix_table[PATCH_NUM_MIX_VALS - p->values[PATCH_PARAM_OUTPUT_MIX]];
      }
      else if ( (m == VOICE_OSC_PAIR_LINE_2_UNISON_1) || 
                (m == VOICE_OSC_PAIR_LINE_2_UNISON_2))
      {
        output_db[m] += S_voice_wave_mix_table[p->values[PATCH_PARAM_OUTPUT_MIX]];
      }

      /* apply unison attenuations */
      if ((m == VOICE_OSC_PAIR_LINE_1_UNISON_1) || 
          (m == VOICE_OSC_PAIR_LINE_2_UNISON_1))
      {
        output_db[m] += S_voice_wave_mix_table[PATCH_NUM_MIX_VALS / 2];
      }
      else if ( (m == VOICE_OSC_PAIR_LINE_1_UNISON_2) || 
                (m == VOICE_OSC_PAIR_LINE_2_UNISON_2))
      {
        output_db[m] += S_voice_wave_mix_table[PATCH_NUM_MIX_VALS / 2];
      }

      /* bound output level */
      if (output_db[m] < 0)
        output_db[m] = 0;
      else if (output_db[m] > VOICE_MAX_ATTENUATION_DB)
        output_db[m] = VOICE_MAX_ATTENUATION_DB;
    }

    /* determine output voice level */
    v->level = 0;

    for (m = 0; m < VOICE_NUM_OSC_PAIRS; m++)
    {
      if (output_flag[m] & 0x01)
        v->level -= S_voice_db_to_linear_table[output_db[m]];
      else
        v->level += S_voice_db_to_linear_table[output_db[m]];
    }
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

  /* wave mix table */
  S_voice_wave_mix_table[0] = VOICE_MAX_ATTENUATION_DB;
  S_voice_wave_mix_table[PATCH_NUM_MIX_VALS] = VOICE_MAX_VOLUME_DB;

  for (m = 1; m < PATCH_NUM_MIX_VALS; m++)
  {
    val = ((float) m) / PATCH_NUM_MIX_VALS;
    S_voice_wave_mix_table[m] = (short) ((10 * (-log(val) / log(10)) / VOICE_DB_STEP_12_BIT) + 0.5f);
  }

  /* oscillator phase increment table */
  /* this contains the phase increments for the highest octave, C9 - B9 */
  for (m = 0; m < VOICE_TUNING_STEPS_PER_OCTAVE; m++)
  {
    val = 440.0f * exp(-log(2) * (9.0f / 12.0f));
    val *= 32 * exp(log(2) * ((float) m / VOICE_TUNING_STEPS_PER_OCTAVE));

    S_voice_wave_phase_increment_table[m] = 
      (unsigned int) ((val * VOICE_1HZ_PHASE_INCREMENT) + 0.5f);
  }

  /* oscillator bend period table */
  for (m = 0; m < VOICE_TUNING_STEPS_PER_OCTAVE; m++)
  {
    val = exp(-log(2) * ((float) m / VOICE_TUNING_STEPS_PER_OCTAVE));
    val *= VOICE_PHASE_WAVE_SIZE;

    S_voice_wave_bend_period_table[m] = (unsigned int) (val + 0.5f);
  }

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

  /* lfo speed table */
  /* the frequencies range from 0.5 hz to 8.5 hz (in 100 steps) */
  for (m = 0; m < PATCH_NUM_LFO_SPEED_VALS; m++)
  {
    S_voice_lfo_speed_table[m] = 
      (int) (((0.5f + ((8.0f * m) / PATCH_NUM_LFO_SPEED_VALS)) * VOICE_1HZ_PHASE_INCREMENT) + 0.5f);
  }

  /* lfo delay table */
  /* the delays range from 0 seconds to 1 second (in 100 steps) */
  for (m = 0; m < PATCH_NUM_LFO_DELAY_VALS; m++)
  {
    S_voice_lfo_delay_table[m] = 
      (int) (((((float) m) / PATCH_NUM_LFO_DELAY_VALS) * CLOCK_SAMPLING_RATE) + 0.5f);
  }

  /* lfo sensitivity tables */
  /* the vibrato sensitivities range from 2 cents to 200 cents (in 100 steps) */
  for (m = 0; m < PATCH_NUM_LFO_SENSITIVITY_VALS; m++)
  {
    S_voice_vibrato_sensitivity_table[m] = 
      (int) (((2 * (m + 1) * VOICE_TUNING_STEPS_PER_OCTAVE) / 1200.0f) + 0.5f);
  }

  /* the tremolo sensitivities range from 0 db to -36 db (in 100 steps) */
  for (m = 0; m < PATCH_NUM_LFO_SENSITIVITY_VALS; m++)
  {
    S_voice_tremolo_sensitivity_table[m] = 
      (int) (((((float) m) * ((3 * VOICE_MAX_ATTENUATION_DB) / 4)) / PATCH_NUM_LFO_SENSITIVITY_VALS) + 0.5f);
  }

  return 0;
}

