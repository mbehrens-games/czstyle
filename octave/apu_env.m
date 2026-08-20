clear;
clc;

% envelope clock: 16 khz

% envelope rates: 16 blocks, 8 rates per block (128 total)
% starting envelope period is 1
% starting envelope pattern step is 1
% if the block is less than the base, period is left shifted by the difference
% if the block is greater than the base, step is left shifted by the difference
env_rate_base_block = 11;

% envelope patterns
env_patterns = [0x0000, 0x0080, 0x0808, 0x0888, ...
                0x2222, 0x22A2, 0x2A2A, 0x2AAA, ...
                0x5555, 0x55D5, 0x5D5D, 0x5DDD, ...
                0x7777, 0x77F7, 0x7F7F, 0x7FFF];

% envelope param time to rate mapping (100 values)
env_rise_time_map = round(16 + 112 * (99:-1:0)/100);
env_fall_time_map = round( 0 + 112 * (99:-1:0)/100);

% envelope levels: 10 bit attenuation (0 to 1023)

% envelope param sustain level mapping (100 values)
env_sustain_level_map = [1023, round(512 * (99:-1:1)/99)];

% envelope slope adjustments (for decay & sustain stages) (not needed?)
env_decay_slopes = round(1024 * (env_sustain_level_map / 1023));
env_sustain_slopes = round(1024 * ((1023 - env_sustain_level_map) / 1023));

% print out tables and constants
printf("Envelope Rate Base Block: \n");
printf("#define APU_ENV_RATE_BASE_BLOCK %d\n", env_rate_base_block)
printf("\n")

printf("Envelope Step Patterns (as 16 Bit Ints) Table: \n");
for m = 1:2
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("0x%04X", env_patterns(8 * (m - 1) + n))
    if ((m < 2) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Envelope Rise Time Map: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%3d", env_rise_time_map(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Envelope Fall Time Map: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%3d", env_fall_time_map(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Envelope Sustain Level Map: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%4d", env_sustain_level_map(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Envelope Decay Slope Increments Table: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%4d", env_decay_slopes(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Envelope Sustain Slope Increments Table: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%4d", env_sustain_slopes(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")
%}

