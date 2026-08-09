clear;
clc;

% vca clock: 3 khz

% vca phase incs (12 blocks, 12 entries per block)
vca_1hz_inc = exp(log(2) * 20) / 3000; % 8.12 fixed point phase
vca_min_f = 1 / (0.006 * exp(log(2) * 12));

vca_phase_incs = round(vca_min_f * vca_1hz_inc * exp(log(2) * (0:143)/12));

% vca wavetables (256 indices)
vca_rise_curve = round(4095 * exp(log(31 / 32) * (0:255)));
vca_fall_curve = round((4095/255) * (0:255));

% vca speeds (32 values)
vca_fall_speeds = round((31 - (0:31)) * (120 / 32));
vca_rise_speeds = (2 * 12) + vca_fall_speeds;

% print out tables
printf("VCA Phase Incs Table: \n");
for m = 1:18
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", vca_phase_incs(8 * (m - 1) + n))
    if ((m < 18) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCA Rise Curve Table: \n");
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", vca_rise_curve(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCA Fall Curve Table: \n");
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", vca_fall_curve(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCA Rise Speeds: \n");
for m = 1:4
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%3d", vca_rise_speeds(8 * (m - 1) + n))
    if ((m < 4) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCA Fall Speeds: \n");
for m = 1:4
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%3d", vca_fall_speeds(8 * (m - 1) + n))
    if ((m < 4) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

