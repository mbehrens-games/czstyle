clear;
clc;

% db to linear table (13 blocks, 256 entries per block)
db_to_linear_table = round(exp(log(2) * 13) * exp(log(1/2) * (1:256)/256));

% dac table (9 bits to 16 bits)
% note: there is an extra step between 0 and -1 (ladder effect on sega genesis)
dac_table = [-round((32768 - 256) * ((255:-1:0)/255) + 256), ...
              round( 32767 * ((0:255)/255))];

% dac described as multipliers instead of a table (4 bit mantissa)
dac_pos_mult = round(16 * (32767 / 255));
dac_neg_mult = round(16 * ((32768 - 256) / 255));

% print out tables
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

printf("DAC Table: \n")
for m = 1:64
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%6d", dac_table(8 * (m - 1) + n))
    if ((m < 64) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

