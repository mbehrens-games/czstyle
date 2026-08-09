clear;
clc;

% pcm clock: 24 khz

% pcm phase incs
pcm_1hz_inc = exp(log(2) * 16) / 24000; % 16 bit mantissa
pcm_samp_rates = [8287, 8363, 11025, 22050];

pcm_phase_incs = round(pcm_1hz_inc * pcm_samp_rates);

% pcm level curve
pcm_curve = round(-256 * (log((2 * (0:127) + 1)/255) / log(2)));

% print out tables
printf("PCM Phase Incs Table: \n")
printf("  { ")
for m = 1:4
  printf("%5d", pcm_phase_incs(m))
  if (m < 4)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("PCM Curve Table: \n")
for m = 1:16
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", pcm_curve(8 * (m - 1) + n))
    if ((m < 16) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

