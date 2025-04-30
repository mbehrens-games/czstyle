/*******************************************************************************
** cart.c (carts & patches)
*******************************************************************************/

#include <stdio.h>

#include "cart.h"

/* cart bank */
cart G_cart_bank[CART_NUM_INDICES];

/* patch parameter bounds array */
unsigned char G_patch_param_bounds[PATCH_NUM_PARAMS] = 
  { /* line 1 */
    PATCH_NUM_WAVE_VALS, 
    PATCH_NUM_WAVE_VALS, 
    PATCH_NUM_ENV_LEVEL_VALS, 
    PATCH_NUM_DETUNE_VALS, 
    PATCH_NUM_MOD_ENABLE_VALS, 
    PATCH_NUM_MOD_ENABLE_VALS, 
    /* line 2 */
    PATCH_NUM_WAVE_VALS, 
    PATCH_NUM_WAVE_VALS, 
    PATCH_NUM_ENV_LEVEL_VALS, 
    PATCH_NUM_OCTAVE_VALS, 
    PATCH_NUM_NOTE_VALS, 
    PATCH_NUM_DETUNE_VALS, 
    PATCH_NUM_MOD_ENABLE_VALS, 
    PATCH_NUM_MOD_ENABLE_VALS, 
    /* output */
    PATCH_NUM_RING_MOD_VALS, 
    PATCH_NUM_MIX_VALS, 
    /* unison */
    PATCH_NUM_DETUNE_VALS, 
    /* amplitude envelope */
    PATCH_NUM_ENV_TIME_VALS, 
    PATCH_NUM_ENV_TIME_VALS, 
    PATCH_NUM_ENV_TIME_VALS, 
    PATCH_NUM_ENV_TIME_VALS, 
    PATCH_NUM_ENV_LEVEL_VALS, 
    PATCH_NUM_ENV_KEYSCALING_VALS, 
    PATCH_NUM_ENV_KEYSCALING_VALS, 
    /* bend envelope */
    PATCH_NUM_ENV_TIME_VALS, 
    PATCH_NUM_ENV_TIME_VALS, 
    PATCH_NUM_ENV_TIME_VALS, 
    PATCH_NUM_ENV_TIME_VALS, 
    PATCH_NUM_ENV_LEVEL_VALS, 
    PATCH_NUM_ENV_KEYSCALING_VALS, 
    PATCH_NUM_ENV_KEYSCALING_VALS, 
    /* velocity */
    PATCH_NUM_VELOCITY_OFFSET_VALS, 
    PATCH_NUM_VELOCITY_DEPTH_VALS, 
    /* vibrato */
    PATCH_NUM_VIBRATO_POLARITY_VALS, 
    PATCH_NUM_LFO_WAVEFORM_VALS, 
    PATCH_NUM_LFO_DELAY_VALS, 
    PATCH_NUM_LFO_SPEED_VALS, 
    PATCH_NUM_LFO_DEPTH_VALS, 
    PATCH_NUM_LFO_SENSITIVITY_VALS, 
    /* tremolo */
    PATCH_NUM_TREMOLO_MODE_VALS, 
    PATCH_NUM_LFO_WAVEFORM_VALS, 
    PATCH_NUM_LFO_DELAY_VALS, 
    PATCH_NUM_LFO_SPEED_VALS, 
    PATCH_NUM_LFO_DEPTH_VALS, 
    PATCH_NUM_LFO_SENSITIVITY_VALS, 
    /* transpose */
    PATCH_NUM_TRANSPOSE_VALS, 
    /* pitch wheel */
    PATCH_NUM_PITCH_WHEEL_MODE_VALS, 
    PATCH_NUM_PITCH_WHEEL_RANGE_VALS, 
    /* portamento */
    PATCH_NUM_PORTAMENTO_MODE_VALS, 
    PATCH_NUM_PORTAMENTO_LEGATO_VALS, 
    PATCH_NUM_PORTAMENTO_FOLLOW_VALS, 
    PATCH_NUM_PORTAMENTO_TIME_VALS 
  };

/*******************************************************************************
** cart_reset_all()
*******************************************************************************/
short int cart_reset_all()
{
  int m;

  /* reset carts */
  for (m = 0; m < CART_NUM_INDICES; m++)
    cart_reset_cart(m);

  return 0;
}

/*******************************************************************************
** cart_reset_patch()
*******************************************************************************/
short int cart_reset_patch(int cart_index, int patch_index)
{
  int m;

  cart*  c;
  patch* p;

  /* make sure that the cart & patch indices are valid */
  if ((cart_index < 0) || (cart_index >= CART_NUM_INDICES))
    return 1;

  if ((patch_index < 0) || (patch_index >= CART_NUM_PATCHES))
    return 1;

  /* obtain cart & patch pointers */
  c = &G_cart_bank[cart_index];
  p = &(c->patches[patch_index]);

  /* reset patch name */
  for (m = 0; m < PATCH_NAME_SIZE; m++)
    p->name[m] = 0;

  /* reset patch data */
  for (m = 0; m < PATCH_NUM_PARAMS; m++)
    p->values[m] = 0;

  /* set some parameters to non-zero default values */
  p->values[PATCH_PARAM_LINE_1_DETUNE]    = PATCH_NUM_DETUNE_VALS / 2;
  p->values[PATCH_PARAM_LINE_2_OCTAVE]    = PATCH_NUM_OCTAVE_VALS / 2;
  p->values[PATCH_PARAM_LINE_2_NOTE]      = PATCH_NUM_NOTE_VALS / 2;
  p->values[PATCH_PARAM_LINE_2_DETUNE]    = PATCH_NUM_DETUNE_VALS / 2;
  p->values[PATCH_PARAM_OUTPUT_MIX]       = PATCH_NUM_MIX_VALS / 2;
  p->values[PATCH_PARAM_UNISON_DETUNE]    = PATCH_NUM_DETUNE_VALS / 2;
  p->values[PATCH_PARAM_VELOCITY_OFFSET]  = PATCH_NUM_VELOCITY_OFFSET_VALS / 2;
  p->values[PATCH_PARAM_VELOCITY_DEPTH]   = PATCH_NUM_VELOCITY_DEPTH_VALS / 2;
  p->values[PATCH_PARAM_TRANSPOSE]        = PATCH_NUM_TRANSPOSE_VALS / 2;

  return 0;
}

/*******************************************************************************
** cart_validate_patch()
*******************************************************************************/
short int cart_validate_patch(int cart_index, int patch_index)
{
  int m;

  cart*  c;
  patch* p;

  /* make sure that the cart & patch indices are valid */
  if ((cart_index < 0) || (cart_index >= CART_NUM_INDICES))
    return 1;

  if ((patch_index < 0) || (patch_index >= CART_NUM_PATCHES))
    return 1;

  /* obtain cart & patch pointers */
  c = &G_cart_bank[cart_index];
  p = &(c->patches[patch_index]);

  /* validate patch name */
  for (m = 0; m < PATCH_NAME_SIZE; m++)
  {
    if (CART_CHARACTER_IS_VALID_IN_CART_OR_PATCH_NAME(p->name[m]))
      continue;
    else
      p->name[m] = ' ';
  }

  /* validate patch data */
  for (m = 0; m < PATCH_NUM_PARAMS; m++)
  {
    if (p->values[m] < 0)
      p->values[m] = 0;
    else if (p->values[m] >= G_patch_param_bounds[m])
      p->values[m] = G_patch_param_bounds[m] - 1;
  }

  return 0;
}

/*******************************************************************************
** cart_copy_patch()
*******************************************************************************/
short int cart_copy_patch(int dest_cart_index,  int dest_patch_index, 
                          int src_cart_index,   int src_patch_index)
{
  int m;

  cart*  dest_c;
  patch* dest_p;

  cart*  src_c;
  patch* src_p;

  /* make sure the destination and source indices are different */
  if ((dest_cart_index == src_cart_index) && 
      (dest_patch_index == src_patch_index))
  {
    return 1;
  }

  /* make sure that the cart & patch indices are valid */
  if ((dest_cart_index < 0) || (dest_cart_index >= CART_NUM_INDICES))
    return 1;

  if ((dest_patch_index < 0) || (dest_patch_index >= CART_NUM_PATCHES))
    return 1;

  if ((src_cart_index < 0) || (src_cart_index >= CART_NUM_INDICES))
    return 1;

  if ((src_patch_index < 0) || (src_patch_index >= CART_NUM_PATCHES))
    return 1;

  /* obtain destination pointers */
  dest_c = &G_cart_bank[dest_cart_index];
  dest_p = &(dest_c->patches[dest_patch_index]);

  /* obtain source pointers */
  src_c = &G_cart_bank[src_cart_index];
  src_p = &(src_c->patches[src_patch_index]);

  /* copy patch name */
  for (m = 0; m < PATCH_NAME_SIZE; m++)
    dest_p->name[m] = src_p->name[m];

  /* copy patch data */
  for (m = 0; m < PATCH_NUM_PARAMS; m++)
    dest_p->values[m] = src_p->values[m];

  return 0;
}

/*******************************************************************************
** cart_reset_cart()
*******************************************************************************/
short int cart_reset_cart(int cart_index)
{
  int m;

  cart* c;

  /* make sure that the cart index is valid */
  if ((cart_index < 0) || (cart_index >= CART_NUM_INDICES))
    return 1;

  /* obtain cart pointer */
  c = &G_cart_bank[cart_index];

  /* reset cart name */
  for (m = 0; m < CART_NAME_SIZE; m++)
    c->name[m] = 0;

  /* reset all patches in cart */
  for (m = 0; m < CART_NUM_PATCHES; m++)
    cart_reset_patch(cart_index, m);

  return 0;
}

/*******************************************************************************
** cart_validate_cart()
*******************************************************************************/
short int cart_validate_cart(int cart_index)
{
  int m;

  cart* c;

  /* make sure that the cart index is valid */
  if ((cart_index < 0) || (cart_index >= CART_NUM_INDICES))
    return 1;

  /* obtain cart pointer */
  c = &G_cart_bank[cart_index];

  /* validate cart name */
  for (m = 0; m < CART_NAME_SIZE; m++)
  {
    if (CART_CHARACTER_IS_VALID_IN_CART_OR_PATCH_NAME(c->name[m]))
      continue;
    else
      c->name[m] = ' ';
  }

  /* validate all patches in cart */
  for (m = 0; m < CART_NUM_PATCHES; m++)
    cart_validate_patch(cart_index, m);

  return 0;
}

/*******************************************************************************
** cart_copy_cart()
*******************************************************************************/
short int cart_copy_cart(int dest_cart_index, int src_cart_index)
{
  int m;

  cart* dest_c;
  cart* src_c;

  /* make sure the destination and source indices are different */
  if (dest_cart_index == src_cart_index)
    return 1;

  /* make sure that the cart indices are valid */
  if ((dest_cart_index < 0) || (dest_cart_index >= CART_NUM_INDICES))
    return 1;

  if ((src_cart_index < 0) || (src_cart_index >= CART_NUM_INDICES))
    return 1;

  /* obtain source & destination pointers */
  dest_c = &G_cart_bank[dest_cart_index];
  src_c = &G_cart_bank[src_cart_index];

  /* copy cart name */
  for (m = 0; m < CART_NAME_SIZE; m++)
    dest_c->name[m] = src_c->name[m];

  /* copy all patches in cart */
  for (m = 0; m < CART_NUM_PATCHES; m++)
    cart_copy_patch(dest_cart_index, m, src_cart_index, m);

  return 0;
}

