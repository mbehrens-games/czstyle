clear;
clc;

% vca clock: 3 khz

% vca phase incs (12 blocks, 16 entries per block)
vca_phase_shifts = [11 - (0:10), 0];
vca_phase_steps = [ones(1,12)];

vca_1hz_inc = exp(log(2) * 20) / 3000; % 8.12 fixed point phase
vca_base_f = 0.5 * (1 / 0.006); % shortest time is ~6 ms

vca_phase_incs = round(vca_base_f * vca_1hz_inc * exp(log(2) * (0:15)/16));

% vca rise & fall curve tables (256 indices, maps index to db attenuation)
vca_rise_curve = round(-256 * (log((0:255)/255) / log(2)));
vca_rise_curve(1) = 4095;

vca_fall_curve = round((4095/255) * (0:255));

% vca_rise_curve = round(4095 * exp(log(31 / 32) * (0:255)));

% vca fall curve multiplier (in lieu of table, 4 bit mantissa)
vca_fall_curve_mult = round(16 * (4095/255));

% vca sustain indices (100 values) (maps to 1st quarter of fall curve)
vca_sustain_indices = round(64 * ((1:100)/100));

% vca rise <-> fall curve remap (256 indices)
vca_fall_to_rise_remap = round(255 * exp(log(2) * vca_fall_curve * (-1/256)));
vca_rise_to_fall_remap = round(vca_rise_curve * (255/4095));

% vca speeds (100 values: 10 blocks, 10 values per block)
vca_speeds = round((9 - (0:9)) * (16 / 10));
vca_rise_speed_block_offset = 2;
vca_fall_speed_block_offset = 0;

% print out tables and constants
printf("VCA Phase Shifts Table: \n")
printf("  { ")
for m = 1:12
  printf("%2d", vca_phase_shifts(m))
  if (m < 12)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("VCA Phase Steps Table: \n")
printf("  { ")
for m = 1:12
  printf("%2d", vca_phase_steps(m))
  if (m < 12)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("VCA Phase Incs Table: \n");
for m = 1:2
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", vca_phase_incs(8 * (m - 1) + n))
    if ((m < 2) || (n < 8))
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

printf("VCA Fall Curve Multiplier: \n");
printf("#define APU_VCA_FALL_CURVE_MULT %d\n", vca_fall_curve_mult)
printf("\n")

printf("VCA Sustain Index Table: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%3d", vca_sustain_indices(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCA Fall to Rise Remap Table: \n");
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%3d", vca_fall_to_rise_remap(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCA Rise to Fall Remap Table: \n");
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%3d", vca_rise_to_fall_remap(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCA Speeds: \n");
printf("  { ")
for m = 1:10
  printf("%2d", vca_speeds(m))
  if (m < 10)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("VCA Speed Block Offsets: \n");
printf("#define APU_VCA_RISE_SPEED_OFFSET 2\n")
printf("#define APU_VCA_FALL_SPEED_OFFSET 0\n")
printf("\n")

