clear;
clc;

% vco clock: 48 khz

% vco phase incs (9 blocks, 12 * 64 entries per block)
vco_phase_shifts = [6 - (0:5), zeros(1,3)];
vco_phase_steps = [ones(1,7), 2, 4];

vco_1hz_inc = exp(log(2) * 20) / 48000; % 10.10 fixed point phase
vco_c6 = 4 * 440 * exp(log(2) * (-9/12));

vco_phase_incs = round(vco_c6 * vco_1hz_inc * exp(log(2) * (0:767)/768));

% vco sine wavetable
vco_sine = round(-256 * (log(sin(2 * pi * (0.5+(0:255))/1024)) / log(2)));

% print out tables
printf("VCO Phase Shifts Table: \n")
printf("  { ")
for m = 1:9
  printf("%d", vco_phase_shifts(m))
  if (m < 9)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("VCO Phase Steps Table: \n")
printf("  { ")
for m = 1:9
  printf("%d", vco_phase_steps(m))
  if (m < 9)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("VCO Phase Incs Table: \n")
for m = 1:96
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", vco_phase_incs(8 * (m - 1) + n))
    if ((m < 96) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
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

