clear;
clc;

% envelope clock: 16 khz

% envelope rates: 16 blocks, 8 rates per block (128 total)
% envelope levels: 10 bit attenuation (0 to 1023)

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

% patch param: envelope adsr rate (100 values)
env_adsr_rate_map = [round(127 * ((99:-1:0)/99))];

% patch param: envelope tl and sl (100 values)
env_total_level_map = [1023, round(8 * 104 * (98:-1:0)/99)];
env_sustain_level_map = [1023, round(32 * 14 * (99:-1:1)/99)];

% patch param: rate keyscaling (100 values)
% on the ym2612: rks 0 means the rate doubles every 8 octaves
%                rks 1 means the rate doubles every 4 octaves
%                rks 2 means the rate doubles every 2 octaves
%                rks 3 means the rate doubles every 1 octave
% the rate doubles for every 8 steps in the rate table (1 block),
% and there are 96 notes in 8 octaves
% so we have multipliers from 8/96 (slowest) to 64/96 (fastest)
% these reduce to 1/12 (or 2^0 / 12) and 8/12 (or 2^3 / 12)
env_rate_ks_map = round(256 * (exp(log(2) * (3 * (0:99)/99))/12));

% patch param: level keyscaling (100 values)
% the level halves for every 64 steps in the envelope index,
% and there are 96 notes in 8 octaves
% so we have multipliers from 64/96 (slowest) to 512/96 (fastest)
% these reduce to 2/3 (or 2^1 / 3) and 16/3 (or 2^4 / 3)
env_level_ks_map = round(256 * (exp(log(2) * (1 + 3 * (0:99)/99))/3));

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

printf("Envelope ADSR Rate Map: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%3d", env_adsr_rate_map(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Envelope Total Level Map: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%4d", env_total_level_map(10 * (m - 1) + n))
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

printf("Envelope Rate Keyscaling Map: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%4d", env_rate_ks_map(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Envelope Level Keyscaling Map: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%4d", env_level_ks_map(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

