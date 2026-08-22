clear;
clc;

% oscillator clock: 48 khz

% oscillator pitch table
% base pitch is C-2
% the table stores 48 values (1 octave in 1/4 semitone increments)
% this is inspired by the YM 2151 table
osc_pitch_base_block = 2;

osc_pitch_base_f = 440 * exp(log(2) * (-2 - 9/12));
osc_pitch_base_inc = osc_pitch_base_f * exp(log(2) * 20) / 48000;

osc_pitch_table = round(osc_pitch_base_inc * exp(log(2) * (0:47)/48));

% oscillator pitch deltas
% 16 steps between quarter semitones (values in pitch table)
% (delta * step) / 16 gets the change from the pitch table value for this step
for m = 1:47
  osc_pitch_deltas(m) = osc_pitch_table(m + 1) - osc_pitch_table(m);
endfor

osc_pitch_deltas(48) = (2 * osc_pitch_table(1)) - osc_pitch_table(48);

% oscillator detune amounts
% these are steps from the value in the pitch table
% as above, (delta * step) / 16 gets the change from the pitch table value
osc_pitch_detune = [0, 2, 4, 6];

% oscillator sine wavetable
osc_sine_table = round(-256 * (log(sin(2 * pi * ((1:2:511))/2048)) / log(2)));

% db to linear table (13 blocks, 256 entries per block)
% on the sega genesis, they are 11 bit left values shifted over to 13 bits
% so, the values are adjusted so so the lower 2 bits are always 0
db_to_linear_table = round(exp(log(2) * 13) * exp(log(1/2) * (1:256)/256));
db_to_linear_table = round(4 * floor(db_to_linear_table / 4));

osc_level_zero_block = 13;

% print out tables and constants
printf("Oscillator Pitch Base Block: \n");
printf("#define APU_OSC_PITCH_BASE_BLOCK %d\n", osc_pitch_base_block)
printf("\n")

printf("Oscillator Level Zero Block: \n");
printf("#define APU_OSC_LEVEL_ZERO_BLOCK %d\n", osc_level_zero_block)
printf("\n")

printf("Oscillator Pitch Table: \n")
for m = 1:12
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:4
    printf("%4d", osc_pitch_table(4 * (m - 1) + n))
    if ((m < 12) || (n < 4))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Oscillator Pitch Deltas: \n")
for m = 1:12
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:4
    printf("%2d", osc_pitch_deltas(4 * (m - 1) + n))
    if ((m < 12) || (n < 4))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Oscillator Detune Table: \n")
printf("  { ")
for m = 1:4
  printf("%d", osc_pitch_detune(m))
  if (m < 4)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("Oscillator Sine Wavetable: \n")
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", osc_sine_table(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("DB to Linear Table: \n")
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", db_to_linear_table(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

