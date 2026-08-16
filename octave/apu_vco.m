clear;
clc;

% vco pitch table (9 blocks, 1024 entries per block)

% base pitch is C-6
vco_pitch_base_f = 4 * 440 * exp(log(2) * (-9/12));

% last block is 8, base pitch is two blocks down
vco_pitch_base_block = 6;

% 8.12 fixed point phase, so 20 bits total, vco clock is 48 khz
vco_pitch_base_inc = vco_pitch_base_f * exp(log(2) * 20) / 48000;

vco_pitch_table = round(vco_pitch_base_inc * exp(log(2) * (0:1023)/1024));

% note to pitch table index mapping
vco_note_map = round(1024 * ((0:11)/12));

% vco sine wavetable
vco_sine = round(-256 * (log(sin(2 * pi * (0.5+(0:255))/1024)) / log(2)));

% vco level table (16 blocks, 256 entries per block)
vco_level_table = round(32768 * exp(log(1/2) * (0:255)/256));

% print out tables and constants
printf("VCO Pitch Base Block: \n");
printf("#define APU_VCO_PITCH_BASE_BLOCK %d\n", vco_pitch_base_block)
printf("\n")

printf("VCO Pitch Table: \n")
for m = 1:128
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", vco_pitch_table(8 * (m - 1) + n))
    if ((m < 128) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCO Note Map Table: \n")
printf("  { ")
for m = 1:12
  printf("%d", vco_note_map(m))
  if (m < 12)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("VCO Sine Wavetable: \n")
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", vco_sine(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCO Level Table: \n")
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", vco_level_table(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

