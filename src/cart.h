/*******************************************************************************
** cart.h (synth carts & patches)
*******************************************************************************/

#ifndef CART_H
#define CART_H

/* named constants and bounds for patch parameters */
enum
{
  PATCH_WAVE_VAL_SAW = 0, 
  PATCH_WAVE_VAL_SQUARE, 
  PATCH_WAVE_VAL_PULSE, 
  PATCH_WAVE_VAL_DOUBLE_SINE, 
  PATCH_WAVE_VAL_HALF_SAW, 
  PATCH_WAVE_VAL_RESONANCE_SAW, 
  PATCH_WAVE_VAL_RESONANCE_TRIANGLE, 
  PATCH_WAVE_VAL_RESONANCE_TRAPEZOID, 
  PATCH_NUM_WAVE_VALS 
};

enum
{
  PATCH_MOD_ENABLE_VAL_OFF = 0, 
  PATCH_MOD_ENABLE_VAL_ON, 
  PATCH_NUM_MOD_ENABLE_VALS 
};

#define PATCH_NUM_OCTAVE_VALS   7   /*  -3 to  3 */
#define PATCH_NUM_NOTE_VALS    15   /*  -7 to  7 */
#define PATCH_NUM_DETUNE_VALS 100   /* -50 to 49 */

enum
{
  PATCH_RING_MOD_VAL_OFF = 0, 
  PATCH_RING_MOD_VAL_ON, 
  PATCH_NUM_RING_MOD_VALS 
};

#define PATCH_NUM_MIX_VALS 100

enum
{
  PATCH_UNISON_MODE_VAL_ZERO_AND_PLUS = 0, 
  PATCH_UNISON_MODE_VAL_PLUS_AND_MINUS, 
  PATCH_NUM_UNISON_MODE_VALS 
};

#define PATCH_NUM_ENV_TIME_VALS       100
#define PATCH_NUM_ENV_LEVEL_VALS      100
#define PATCH_NUM_ENV_KEYSCALING_VALS 100

enum
{
  PATCH_VIBRATO_POLARITY_VAL_BI = 0, 
  PATCH_VIBRATO_POLARITY_VAL_UNI, 
  PATCH_NUM_VIBRATO_POLARITY_VALS 
};

enum
{
  PATCH_TREMOLO_MODE_VAL_AMP = 0, 
  PATCH_TREMOLO_MODE_VAL_BEND, 
  PATCH_NUM_TREMOLO_MODE_VALS 
};

enum
{
  PATCH_BOOST_MODE_VAL_AMP = 0, 
  PATCH_BOOST_MODE_VAL_BEND, 
  PATCH_NUM_BOOST_MODE_VALS 
};

enum
{
  PATCH_VELOCITY_MODE_VAL_AMP = 0, 
  PATCH_VELOCITY_MODE_VAL_BEND, 
  PATCH_NUM_VELOCITY_MODE_VALS 
};

enum
{
  PATCH_LFO_WAVEFORM_VAL_TRIANGLE = 0, 
  PATCH_LFO_WAVEFORM_VAL_SQUARE, 
  PATCH_LFO_WAVEFORM_VAL_SAW_UP, 
  PATCH_LFO_WAVEFORM_VAL_SAW_DOWN, 
  PATCH_NUM_LFO_WAVEFORM_VALS 
};

#define PATCH_NUM_LFO_DELAY_VALS   100
#define PATCH_NUM_LFO_SPEED_VALS   100
#define PATCH_NUM_LFO_DEPTH_VALS   100
#define PATCH_NUM_SENSITIVITY_VALS 100

enum
{
  PATCH_PITCH_WHEEL_MODE_VAL_PORTAMENTO = 0, 
  PATCH_PITCH_WHEEL_MODE_VAL_GLISSANDO, 
  PATCH_NUM_PITCH_WHEEL_MODE_VALS 
};

#define PATCH_NUM_PITCH_WHEEL_RANGE_VALS 12

enum
{
  PATCH_PORTAMENTO_MODE_VAL_PORTAMENTO = 0, 
  PATCH_PORTAMENTO_MODE_VAL_GLISSANDO, 
  PATCH_NUM_PORTAMENTO_MODE_VALS 
};

enum
{
  PATCH_PORTAMENTO_LEGATO_VAL_OFF = 0, 
  PATCH_PORTAMENTO_LEGATO_VAL_ON, 
  PATCH_NUM_PORTAMENTO_LEGATO_VALS 
};

enum
{
  PATCH_PORTAMENTO_FOLLOW_VAL_CONTINUE = 0, 
  PATCH_PORTAMENTO_FOLLOW_VAL_HAMMER, 
  PATCH_NUM_PORTAMENTO_FOLLOW_VALS 
};

#define PATCH_NUM_PORTAMENTO_TIME_VALS  100

/* the patch parameters */
enum
{
  /* line 1 */
  PATCH_PARAM_LINE_1_WAVE_1 = 0, 
  PATCH_PARAM_LINE_1_WAVE_2, 
  PATCH_PARAM_LINE_1_BEND_MAX, 
  PATCH_PARAM_LINE_1_PM_ENABLE, 
  PATCH_PARAM_LINE_1_AM_ENABLE, 
  PATCH_PARAM_LINE_1_DETUNE, 
  /* line 2 */
  PATCH_PARAM_LINE_2_WAVE_1, 
  PATCH_PARAM_LINE_2_WAVE_2, 
  PATCH_PARAM_LINE_2_BEND_MAX, 
  PATCH_PARAM_LINE_2_PM_ENABLE, 
  PATCH_PARAM_LINE_2_AM_ENABLE, 
  PATCH_PARAM_LINE_2_OCTAVE, 
  PATCH_PARAM_LINE_2_NOTE, 
  PATCH_PARAM_LINE_2_DETUNE, 
  /* output */
  PATCH_PARAM_OUTPUT_RING_MOD, 
  PATCH_PARAM_OUTPUT_MIX, 
  /* unison */
  PATCH_PARAM_UNISON_MODE, 
  PATCH_PARAM_UNISON_DETUNE, 
  /* amplitude envelope */
  PATCH_PARAM_AMP_ENV_ATTACK, 
  PATCH_PARAM_AMP_ENV_DECAY, 
  PATCH_PARAM_AMP_ENV_RELEASE, 
  PATCH_PARAM_AMP_ENV_HOLD, 
  PATCH_PARAM_AMP_ENV_SUSTAIN, 
  PATCH_PARAM_AMP_ENV_TIME_KS, 
  PATCH_PARAM_AMP_ENV_LEVEL_KS, 
  /* bend envelope */
  PATCH_PARAM_BEND_ENV_ATTACK, 
  PATCH_PARAM_BEND_ENV_DECAY, 
  PATCH_PARAM_BEND_ENV_RELEASE, 
  PATCH_PARAM_BEND_ENV_HOLD, 
  PATCH_PARAM_BEND_ENV_SUSTAIN, 
  PATCH_PARAM_BEND_ENV_TIME_KS, 
  PATCH_PARAM_BEND_ENV_LEVEL_KS, 
  /* vibrato */
  PATCH_PARAM_VIBRATO_POLARITY, 
  PATCH_PARAM_VIBRATO_WAVEFORM, 
  PATCH_PARAM_VIBRATO_DELAY, 
  PATCH_PARAM_VIBRATO_SPEED, 
  PATCH_PARAM_VIBRATO_DEPTH, 
  PATCH_PARAM_VIBRATO_SENSITIVITY, 
  /* tremolo */
  PATCH_PARAM_TREMOLO_MODE, 
  PATCH_PARAM_TREMOLO_WAVEFORM, 
  PATCH_PARAM_TREMOLO_DELAY, 
  PATCH_PARAM_TREMOLO_SPEED, 
  PATCH_PARAM_TREMOLO_DEPTH, 
  PATCH_PARAM_TREMOLO_SENSITIVITY, 
  /* boost */
  PATCH_PARAM_BOOST_MODE, 
  PATCH_PARAM_BOOST_SENSITIVITY, 
  /* velocity */
  PATCH_PARAM_VELOCITY_MODE, 
  PATCH_PARAM_VELOCITY_SENSITIVITY, 
  /* pitch wheel */
  PATCH_PARAM_PITCH_WHEEL_MODE, 
  PATCH_PARAM_PITCH_WHEEL_RANGE, 
  /* portamento */
  PATCH_PARAM_PORTAMENTO_MODE, 
  PATCH_PARAM_PORTAMENTO_LEGATO, 
  PATCH_PARAM_PORTAMENTO_FOLLOW, 
  PATCH_PARAM_PORTAMENTO_TIME, 
  PATCH_NUM_PARAMS
};

/* name strings: 16 characters max */
#define CART_NAME_SIZE  16
#define PATCH_NAME_SIZE 16

#define CART_CHARACTER_IS_VALID_IN_CART_OR_PATCH_NAME(c)                       \
  ( (c == ' ')                  ||                                             \
    (c == '\0')                 ||                                             \
    ((c >= '0') && (c <= '9'))  ||                                             \
    ((c >= 'A') && (c <= 'Z'))  ||                                             \
    ((c >= 'a') && (c <= 'z')))

/* check if a parameter is within bounds */
#define PATCH_PARAM_IS_VALID_LOOKUP_BY_NAME(name)                              \
  ( (p->values[PATCH_PARAM_##name] >= 0) &&                                    \
    (p->values[PATCH_PARAM_##name] <= G_patch_param_bounds[PATCH_PARAM_##name]))

#define PATCH_VALUE_IS_VALID_LOOKUP_BY_NAME(val, name)                         \
  ((val >= 0) && (val <= G_patch_param_bounds[PATCH_PARAM_##name]))

/* cart parameters */
enum
{
  CART_INDEX_MUSIC = 0, 
  CART_INDEX_SOUND_FX, 
  CART_NUM_INDICES 
};

#define CART_NUM_PATCHES 16

typedef struct patch
{
  char name[PATCH_NAME_SIZE];

  unsigned char values[PATCH_NUM_PARAMS];
} patch;

typedef struct cart
{
  char name[CART_NAME_SIZE];

  patch patches[CART_NUM_PATCHES];
} cart;

/* cart bank */
extern cart G_cart_bank[CART_NUM_INDICES];

/* patch parameter bounds array */
extern unsigned char G_patch_param_bounds[PATCH_NUM_PARAMS];

/* function declarations */
short int cart_reset_all();

short int cart_reset_patch(int cart_index, int patch_index);
short int cart_validate_patch(int cart_index, int patch_index);
short int cart_copy_patch(int dest_cart_index,  int dest_patch_index, 
                          int src_cart_index,   int src_patch_index);

short int cart_reset_cart(int cart_index);
short int cart_validate_cart(int cart_index);
short int cart_copy_cart(int dest_cart_index, int src_cart_index);

#endif
