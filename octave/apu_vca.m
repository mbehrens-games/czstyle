clear;
clc;

% vca speed table (16 blocks, 256 entries per block)

% shortest rise time is ~6 ms at end of block
% multiply to 1/2 to get the start of the block
vca_speed_base_f = (1/2) * (1 / 0.006);

% rise blocks are 2 above the fall blocks
vca_speed_rise_low_block = 6;
vca_speed_fall_low_block = 4;

vca_speed_base_block = vca_speed_rise_low_block + 9;

% 8.12 fixed point phase, so 20 bits total, vca clock is 3 khz
vca_speed_base_inc = vca_speed_base_f * exp(log(2) * 20) / 3000;

vca_speed_table = round(vca_speed_base_inc * exp(log(2) * (0:255)/256));

% vca times (100 values)
vca_rise_time_map = round(256 * (vca_speed_rise_low_block + (99 - (0:99))/10));
vca_fall_time_map = round(256 * (vca_speed_fall_low_block + (99 - (0:99))/10));

% vca rise & fall curve tables (256 indices, maps index to db attenuation)
vca_rise_curve = [4095, round(-256 * (log((1:254)/255) / log(2))), 0];
vca_fall_curve = [4095, round(4095 * ((255 - (1:254))/255)), 0];

% vca_rise_curve = [4095, round(4095 * exp(log(31 / 32) * (1:255)))];

% vca rise <-> fall curve remap
vca_fall_to_rise_remap = round(255 * exp(log(2) * vca_fall_curve * (-1/256)));
vca_rise_to_fall_remap = round(255 * (1 - (vca_rise_curve / 4095)));

% vca sustain levels (100 values)
vca_sustain_level_map = round(255 - (16 + 64 * (99 - (0:99))/99));

% vca speed adjustments (for decay & sustain stages)
vca_decay_fracts = 256 ./ (255 - vca_sustain_level_map);
vca_sustain_fracts = 256 ./ vca_sustain_level_map;

vca_decay_speed_offsets = round(256 * (log(vca_decay_fracts) / log(2)));
vca_sustain_speed_offsets = round(256 * (log(vca_sustain_fracts) / log(2)));

% print out tables and constants
printf("VCA Speed Base Block: \n");
printf("#define APU_VCA_SPEED_BASE_BLOCK %d\n", vca_speed_base_block)
printf("\n")

printf("VCA Speed Table: \n");
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", vca_speed_table(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCA Rise Time Map: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%3d", vca_rise_time_map(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCA Fall Time Map: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%3d", vca_fall_time_map(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
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

printf("VCA Sustain Level Map Table: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%3d", vca_sustain_level_map(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCA Decay Speed Offsets Table: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%4d", vca_decay_speed_offsets(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCA Sustain Speed Offsets Table: \n");
for m = 1:10
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:10
    printf("%4d", vca_sustain_speed_offsets(10 * (m - 1) + n))
    if ((m < 10) || (n < 10))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

