clear;
clc;

% chip clock: 48 khz
%  out clock: 24 khz

% dac (9 bits to 16 bits)
% the sega genesis dac has a "ladder effect"
% there is a larger step between -1 and 0 than between the other values.
% the base step is ~128 (16 - 9 = 7 bits), so the larger step is ~256
% the multipliers have a 6 bit mantissa
dac_pos_mult = round(64 * (32767 / 255));
dac_neg_mult = round(64 * ((32768 - 256) / 255));

% filters
fs = 48000;
hp_fc =    32 / (fs / 2); % c-1
lp_fc =  2840 / (fs / 2); % sega genesis lowpass cutoff
ds_fc = 10500 / (fs / 2); % middle of transition band (~3 khz width)

[hp_b, hp_a] = butter(1, hp_fc, "high");
[lp_b, lp_a] = butter(1, lp_fc, "low");
ds_b = fir1(64, ds_fc);

%{
[h, w] = freqz(hp_b, hp_a, 2048, fs);
plot(w, abs(h))
%}

%{
[h, w] = freqz(lp_b, lp_a, 2048, fs);
plot(w, abs(h))
%}

%{
[h, w] = freqz(ds_b, 1, 2048, fs);
plot(w, abs(h))
%}

hp_b = round(32768 * hp_b);
hp_a = round(32768 * hp_a);

lp_b = round(32768 * lp_b);
lp_a = round(32768 * lp_a);

ds_b = round(32768 * ds_b);

% print out dac multipliers
printf("DAC Multipliers: \n");
printf("#define APU_DAC_POS_MULT %4d\n", dac_pos_mult)
printf("#define APU_DAC_NEG_MULT %4d\n", dac_neg_mult)
printf("\n")

% print out highpass (1st order) filter coefficents
printf("Highpass Coefficents: \n");
printf("#define APU_HP_MULT_A0 %6d\n", hp_a(1))
printf("#define APU_HP_MULT_A1 %6d\n", hp_a(2))
printf("#define APU_HP_MULT_B0 %6d\n", hp_b(1))
printf("#define APU_HP_MULT_B1 %6d\n", hp_b(2))
printf("\n")

% print out lowpass (1st order) filter coefficents
printf("Lowpass Coefficents: \n");
printf("#define APU_LP_MULT_A0 %6d\n", lp_a(1))
printf("#define APU_LP_MULT_A1 %6d\n", lp_a(2))
printf("#define APU_LP_MULT_B0 %6d\n", lp_b(1))
printf("#define APU_LP_MULT_B1 %6d\n", lp_b(2))
printf("\n")

% print out downsampler filter kernel
printf("Downsampler Filter Kernel: \n");
for m = 1:4
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", ds_b(8 * (m - 1) + n))
    if ((m < 48) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("    %5d\n", ds_b(33))
printf("  };")
printf("\n\n")

