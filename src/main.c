/*******************************************************************************
** CZStyle (prototype code for Felisynth) - No Shinobi Knows Me 2025
*******************************************************************************/

/*******************************************************************************
** main.c
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "cart.h"
#include "clock.h"
#include "voice.h"

#define PI 3.141592653589793f

#define TEST_BUFFER_SIZE (3 * CLOCK_SAMPLING_RATE) /* 3 seconds at 32khz */

static short S_waveform_test_buffer[TEST_BUFFER_SIZE];

/*******************************************************************************
** play_note()
*******************************************************************************/
short int play_midi_note(int note)
{
  int k;

  voice* v;

  /* obtain voice pointer (using voice index 0 for this driver) */
  v = &G_voice_bank[0];

  /* trigger the note */
  voice_note_on(0, note);

  /* reset sample buffer */
  for (k = 0; k < TEST_BUFFER_SIZE; k++)
    S_waveform_test_buffer[k] = 0;

  /* generate samples for this note */
  for (k = 0; k < TEST_BUFFER_SIZE; k++)
  {
    voice_update_all();
    S_waveform_test_buffer[k] = v->level;
  }

  return 0;
}

/*******************************************************************************
** export_buffer()
*******************************************************************************/
short int export_buffer(char* filename)
{
  FILE* fp;

  char id_field[4];

  unsigned int   chunk_size;
  unsigned int   header_subchunk_size;
  unsigned int   data_subchunk_size;

  unsigned short audio_format;
  unsigned short num_channels;
  unsigned int   sampling_rate;
  unsigned int   byte_rate;
  unsigned short sample_size;
  unsigned short bit_resolution;

  unsigned int num_samples;

  /* make sure filename exists */
  if (filename == NULL)
    return 1;

  /* set and compute values */
  audio_format = 1;
  num_channels = 1;
  sampling_rate = CLOCK_SAMPLING_RATE;
  bit_resolution = 16;
  sample_size = num_channels * (bit_resolution / 8);
  byte_rate = sampling_rate * sample_size;

  header_subchunk_size = 16;
  data_subchunk_size = TEST_BUFFER_SIZE * sample_size;
  chunk_size = 4 + (8 + header_subchunk_size) + (8 + data_subchunk_size);

  /* open file */
  fp = fopen(filename, "wb");

  if (fp == NULL)
    return 1;

  /* write 'RIFF' */
  strncpy(&id_field[0], "RIFF", 4);

  if (fwrite(id_field, 1, 4, fp) < 4)
  {
    fclose(fp);
    return 1;
  }

  /* write chunk size */
  if (fwrite(&chunk_size, 4, 1, fp) < 1)
  {
    fclose(fp);
    return 1;
  }

  /* write 'WAVE' */
  strncpy(&id_field[0], "WAVE", 4);

  if (fwrite(id_field, 1, 4, fp) < 4)
  {
    fclose(fp);
    return 1;
  }

  /* write 'fmt ' */
  strncpy(&id_field[0], "fmt ", 4);

  if (fwrite(id_field, 1, 4, fp) < 4)
  {
    fclose(fp);
    return 1;
  }

  /* write header subchunk size */
  if (fwrite(&header_subchunk_size, 4, 1, fp) < 1)
  {
    fclose(fp);
    return 1;
  }

  /* write header subchunk */
  if (fwrite(&audio_format, 2, 1, fp) < 1)
  {
    fclose(fp);
    return 1;
  }

  if (fwrite(&num_channels, 2, 1, fp) < 1)
  {
    fclose(fp);
    return 1;
  }

  if (fwrite(&sampling_rate, 4, 1, fp) < 1)
  {
    fclose(fp);
    return 1;
  }

  if (fwrite(&byte_rate, 4, 1, fp) < 1)
  {
    fclose(fp);
    return 1;
  }

  if (fwrite(&sample_size, 2, 1, fp) < 1)
  {
    fclose(fp);
    return 1;
  }

  if (fwrite(&bit_resolution, 2, 1, fp) < 1)
  {
    fclose(fp);
    return 1;
  }

  /* write 'data' */
  strncpy(&id_field[0], "data", 4);

  if (fwrite(id_field, 1, 4, fp) < 4)
  {
    fclose(fp);
    return 1;
  }

  /* write data subchunk size */
  if (fwrite(&data_subchunk_size, 4, 1, fp) < 1)
  {
    fclose(fp);
    return 1;
  }

  /* determine number of samples */
  num_samples = TEST_BUFFER_SIZE;

  /* write data subchunk */
  if (fwrite(&S_waveform_test_buffer[0], 2, num_samples, fp) < num_samples)
  {
    fclose(fp);
    return 1;
  }

  /* close file */
  fclose(fp);

  return 0;
}

/*******************************************************************************
** main()
*******************************************************************************/
int main(int argc, char *argv[])
{
  voice* v;

  cart* c;
  patch* p;

  /* compute tables */
  voice_generate_tables();

  /* reset carts & voices */
  cart_reset_all();
  voice_reset_all();

  voice_load_patch(0, 0, 0);

  /* obtain pointers */
  v = &G_voice_bank[0];

  c = &G_cart_bank[v->cart_index];
  p = &(c->patches[v->patch_index]);

  /* saw sweep */
  cart_reset_patch(0, 0);

  p->values[PATCH_PARAM_LINE_1_WAVE_1] = PATCH_WAVE_VAL_SAW;
  p->values[PATCH_PARAM_LINE_1_WAVE_2] = PATCH_WAVE_VAL_SAW;
  p->values[PATCH_PARAM_LINE_1_BEND_MAX]  = 99;

  p->values[PATCH_PARAM_AMP_ENV_ATTACK]   = 0;
  p->values[PATCH_PARAM_AMP_ENV_DECAY]    = 70;
  p->values[PATCH_PARAM_AMP_ENV_RELEASE]  = 50;
  p->values[PATCH_PARAM_AMP_ENV_HOLD]     = 75;
  p->values[PATCH_PARAM_AMP_ENV_SUSTAIN]  = 90;

  p->values[PATCH_PARAM_BEND_ENV_ATTACK]  = 0;
  p->values[PATCH_PARAM_BEND_ENV_DECAY]   = 30;
  p->values[PATCH_PARAM_BEND_ENV_RELEASE] = 50;
  p->values[PATCH_PARAM_BEND_ENV_HOLD]    = 50;
  p->values[PATCH_PARAM_BEND_ENV_SUSTAIN] = 70;

  /* midi note 60 is C-4 */
  play_midi_note(60);
  export_buffer("saw_sweep.wav");

  /* square sweep */
  cart_reset_patch(0, 0);

  p->values[PATCH_PARAM_LINE_1_WAVE_1] = PATCH_WAVE_VAL_SQUARE;
  p->values[PATCH_PARAM_LINE_1_WAVE_2] = PATCH_WAVE_VAL_SQUARE;
  p->values[PATCH_PARAM_LINE_1_BEND_MAX]  = 99;

  p->values[PATCH_PARAM_AMP_ENV_ATTACK]   = 0;
  p->values[PATCH_PARAM_AMP_ENV_DECAY]    = 70;
  p->values[PATCH_PARAM_AMP_ENV_RELEASE]  = 50;
  p->values[PATCH_PARAM_AMP_ENV_HOLD]     = 75;
  p->values[PATCH_PARAM_AMP_ENV_SUSTAIN]  = 90;

  p->values[PATCH_PARAM_BEND_ENV_ATTACK]  = 0;
  p->values[PATCH_PARAM_BEND_ENV_DECAY]   = 30;
  p->values[PATCH_PARAM_BEND_ENV_RELEASE] = 50;
  p->values[PATCH_PARAM_BEND_ENV_HOLD]    = 50;
  p->values[PATCH_PARAM_BEND_ENV_SUSTAIN] = 70;

  play_midi_note(60);
  export_buffer("square_sweep.wav");

  /* pulse sweep */
  cart_reset_patch(0, 0);

  p->values[PATCH_PARAM_LINE_1_WAVE_1] = PATCH_WAVE_VAL_PULSE;
  p->values[PATCH_PARAM_LINE_1_WAVE_2] = PATCH_WAVE_VAL_PULSE;
  p->values[PATCH_PARAM_LINE_1_BEND_MAX]  = 99;

  p->values[PATCH_PARAM_AMP_ENV_ATTACK]   = 0;
  p->values[PATCH_PARAM_AMP_ENV_DECAY]    = 70;
  p->values[PATCH_PARAM_AMP_ENV_RELEASE]  = 50;
  p->values[PATCH_PARAM_AMP_ENV_HOLD]     = 75;
  p->values[PATCH_PARAM_AMP_ENV_SUSTAIN]  = 90;

  p->values[PATCH_PARAM_BEND_ENV_ATTACK]  = 0;
  p->values[PATCH_PARAM_BEND_ENV_DECAY]   = 30;
  p->values[PATCH_PARAM_BEND_ENV_RELEASE] = 50;
  p->values[PATCH_PARAM_BEND_ENV_HOLD]    = 50;
  p->values[PATCH_PARAM_BEND_ENV_SUSTAIN] = 70;

  play_midi_note(60);
  export_buffer("pulse_sweep.wav");

  /* double sine */
  cart_reset_patch(0, 0);

  p->values[PATCH_PARAM_LINE_1_WAVE_1] = PATCH_WAVE_VAL_DOUBLE_SINE;
  p->values[PATCH_PARAM_LINE_1_WAVE_2] = PATCH_WAVE_VAL_DOUBLE_SINE;
  p->values[PATCH_PARAM_LINE_1_BEND_MAX]  = 99;

  p->values[PATCH_PARAM_AMP_ENV_ATTACK]   = 0;
  p->values[PATCH_PARAM_AMP_ENV_DECAY]    = 70;
  p->values[PATCH_PARAM_AMP_ENV_RELEASE]  = 50;
  p->values[PATCH_PARAM_AMP_ENV_HOLD]     = 75;
  p->values[PATCH_PARAM_AMP_ENV_SUSTAIN]  = 90;

  p->values[PATCH_PARAM_BEND_ENV_ATTACK]  = 0;
  p->values[PATCH_PARAM_BEND_ENV_DECAY]   = 30;
  p->values[PATCH_PARAM_BEND_ENV_RELEASE] = 50;
  p->values[PATCH_PARAM_BEND_ENV_HOLD]    = 50;
  p->values[PATCH_PARAM_BEND_ENV_SUSTAIN] = 70;

  play_midi_note(60);
  export_buffer("double_sine.wav");

  /* half saw */
  cart_reset_patch(0, 0);

  p->values[PATCH_PARAM_LINE_1_WAVE_1] = PATCH_WAVE_VAL_HALF_SAW;
  p->values[PATCH_PARAM_LINE_1_WAVE_2] = PATCH_WAVE_VAL_HALF_SAW;
  p->values[PATCH_PARAM_LINE_1_BEND_MAX]  = 99;

  p->values[PATCH_PARAM_AMP_ENV_ATTACK]   = 0;
  p->values[PATCH_PARAM_AMP_ENV_DECAY]    = 70;
  p->values[PATCH_PARAM_AMP_ENV_RELEASE]  = 50;
  p->values[PATCH_PARAM_AMP_ENV_HOLD]     = 75;
  p->values[PATCH_PARAM_AMP_ENV_SUSTAIN]  = 90;

  p->values[PATCH_PARAM_BEND_ENV_ATTACK]  = 0;
  p->values[PATCH_PARAM_BEND_ENV_DECAY]   = 30;
  p->values[PATCH_PARAM_BEND_ENV_RELEASE] = 50;
  p->values[PATCH_PARAM_BEND_ENV_HOLD]    = 50;
  p->values[PATCH_PARAM_BEND_ENV_SUSTAIN] = 70;

  play_midi_note(60);
  export_buffer("half_saw.wav");

  /* saw - pulse */
  cart_reset_patch(0, 0);

  p->values[PATCH_PARAM_LINE_1_WAVE_1] = PATCH_WAVE_VAL_SAW;
  p->values[PATCH_PARAM_LINE_1_WAVE_2] = PATCH_WAVE_VAL_PULSE;
  p->values[PATCH_PARAM_LINE_1_BEND_MAX]  = 99;

  p->values[PATCH_PARAM_AMP_ENV_ATTACK]   = 0;
  p->values[PATCH_PARAM_AMP_ENV_DECAY]    = 70;
  p->values[PATCH_PARAM_AMP_ENV_RELEASE]  = 50;
  p->values[PATCH_PARAM_AMP_ENV_HOLD]     = 75;
  p->values[PATCH_PARAM_AMP_ENV_SUSTAIN]  = 90;

  p->values[PATCH_PARAM_BEND_ENV_ATTACK]  = 0;
  p->values[PATCH_PARAM_BEND_ENV_DECAY]   = 30;
  p->values[PATCH_PARAM_BEND_ENV_RELEASE] = 50;
  p->values[PATCH_PARAM_BEND_ENV_HOLD]    = 50;
  p->values[PATCH_PARAM_BEND_ENV_SUSTAIN] = 70;

  play_midi_note(60);
  export_buffer("saw_pulse_swap.wav");

  /* saw - resonance */
  cart_reset_patch(0, 0);

  p->values[PATCH_PARAM_LINE_1_WAVE_1] = PATCH_WAVE_VAL_SAW;
  p->values[PATCH_PARAM_LINE_1_WAVE_2] = PATCH_WAVE_VAL_RESONANCE_SAW;
  p->values[PATCH_PARAM_LINE_1_BEND_MAX]  = 99;

  p->values[PATCH_PARAM_AMP_ENV_ATTACK]   = 0;
  p->values[PATCH_PARAM_AMP_ENV_DECAY]    = 70;
  p->values[PATCH_PARAM_AMP_ENV_RELEASE]  = 50;
  p->values[PATCH_PARAM_AMP_ENV_HOLD]     = 75;
  p->values[PATCH_PARAM_AMP_ENV_SUSTAIN]  = 90;

  p->values[PATCH_PARAM_BEND_ENV_ATTACK]  = 0;
  p->values[PATCH_PARAM_BEND_ENV_DECAY]   = 30;
  p->values[PATCH_PARAM_BEND_ENV_RELEASE] = 50;
  p->values[PATCH_PARAM_BEND_ENV_HOLD]    = 50;
  p->values[PATCH_PARAM_BEND_ENV_SUSTAIN] = 70;

  play_midi_note(60);
  export_buffer("saw_resonance.wav");

  /* square - resonance */
  cart_reset_patch(0, 0);

  p->values[PATCH_PARAM_LINE_1_WAVE_1] = PATCH_WAVE_VAL_SQUARE;
  p->values[PATCH_PARAM_LINE_1_WAVE_2] = PATCH_WAVE_VAL_RESONANCE_TRAPEZOID;
  p->values[PATCH_PARAM_LINE_1_BEND_MAX]  = 99;

  p->values[PATCH_PARAM_AMP_ENV_ATTACK]   = 0;
  p->values[PATCH_PARAM_AMP_ENV_DECAY]    = 70;
  p->values[PATCH_PARAM_AMP_ENV_RELEASE]  = 50;
  p->values[PATCH_PARAM_AMP_ENV_HOLD]     = 75;
  p->values[PATCH_PARAM_AMP_ENV_SUSTAIN]  = 90;

  p->values[PATCH_PARAM_BEND_ENV_ATTACK]  = 0;
  p->values[PATCH_PARAM_BEND_ENV_DECAY]   = 30;
  p->values[PATCH_PARAM_BEND_ENV_RELEASE] = 50;
  p->values[PATCH_PARAM_BEND_ENV_HOLD]    = 50;
  p->values[PATCH_PARAM_BEND_ENV_SUSTAIN] = 70;

  play_midi_note(60);
  export_buffer("square_resonance.wav");

  /* pulse - resonance */
  cart_reset_patch(0, 0);

  p->values[PATCH_PARAM_LINE_1_WAVE_1] = PATCH_WAVE_VAL_PULSE;
  p->values[PATCH_PARAM_LINE_1_WAVE_2] = PATCH_WAVE_VAL_RESONANCE_TRIANGLE;
  p->values[PATCH_PARAM_LINE_1_BEND_MAX]  = 99;

  p->values[PATCH_PARAM_AMP_ENV_ATTACK]   = 0;
  p->values[PATCH_PARAM_AMP_ENV_DECAY]    = 70;
  p->values[PATCH_PARAM_AMP_ENV_RELEASE]  = 50;
  p->values[PATCH_PARAM_AMP_ENV_HOLD]     = 75;
  p->values[PATCH_PARAM_AMP_ENV_SUSTAIN]  = 90;

  p->values[PATCH_PARAM_BEND_ENV_ATTACK]  = 0;
  p->values[PATCH_PARAM_BEND_ENV_DECAY]   = 30;
  p->values[PATCH_PARAM_BEND_ENV_RELEASE] = 50;
  p->values[PATCH_PARAM_BEND_ENV_HOLD]    = 50;
  p->values[PATCH_PARAM_BEND_ENV_SUSTAIN] = 70;

  play_midi_note(60);
  export_buffer("pulse_resonance.wav");


  return 0;
}
