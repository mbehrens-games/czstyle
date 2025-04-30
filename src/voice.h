/*******************************************************************************
** voice.h (synth voice)
*******************************************************************************/

#ifndef VOICE_H
#define VOICE_H

enum
{
  VOICE_OSC_PAIR_LINE_1_UNISON_1 = 0, 
  VOICE_OSC_PAIR_LINE_1_UNISON_2, 
  VOICE_OSC_PAIR_LINE_2_UNISON_1, 
  VOICE_OSC_PAIR_LINE_2_UNISON_2, 
  VOICE_NUM_OSC_PAIRS 
};

enum
{
  VOICE_ENV_LINE_1_AMPLITUDE = 0, 
  VOICE_ENV_LINE_1_BEND, 
  VOICE_ENV_LINE_2_AMPLITUDE, 
  VOICE_ENV_LINE_2_BEND, 
  VOICE_NUM_ENVS 
};

enum
{
  VOICE_LFO_VIBRATO = 0, 
  VOICE_LFO_TREMOLO, 
  VOICE_NUM_LFOS 
};

#define VOICE_NUM_VOICES 16

typedef struct voice
{
  /* cart & patch indices */
  short cart_index;
  short patch_index;

  /* currently playing note */
  short base_note;

  /* oscillator pairs */
  long pair_pitch_index[VOICE_NUM_OSC_PAIRS];

  unsigned char pair_flag[VOICE_NUM_OSC_PAIRS];

  unsigned int pair_wave_phase[VOICE_NUM_OSC_PAIRS];
  unsigned int pair_res_phase[VOICE_NUM_OSC_PAIRS];

  /* envelopes */
  short env_stage[VOICE_NUM_ENVS];

  unsigned int env_phase[VOICE_NUM_ENVS];

  short env_attenuation[VOICE_NUM_ENVS];

  /* lfos */
  short lfo_delay_cycles[VOICE_NUM_LFOS];

  unsigned int lfo_phase[VOICE_NUM_LFOS];

  /* midi controller positions */
  short pitch_wheel_pos;
  short vibrato_wheel_pos;
  short tremolo_wheel_pos;
  short note_velocity_pos;

  /* output level */
  int level;
} voice;

/* voice bank */
extern voice G_voice_bank[VOICE_NUM_VOICES];

/* function declarations */
short int voice_reset_all();

short int voice_load_patch( short voice_index, 
                            short cart_index, short patch_index);

short int voice_note_on(short voice_index, short midi_note, short velocity);
short int voice_note_off(short voice_index);

short int voice_update_all();

short int voice_generate_tables();

#endif
