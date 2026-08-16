clear;
clc;

% chip clock: 48 khz
%  out clock: 24 khz

% filters
fs = 48000;
hp_fc = 27.5 / (fs / 2);
lp_fc =  10500 / (fs / 2);

[hp_b, hp_a] = butter(1, hp_fc, "high");
lp_b = fir1(64, lp_fc);

%{
[h, w] = freqz(hp_b, hp_a, 2048, fs);
plot(w, abs(h))
%}

%{
[h, w] = freqz(lp_b, 1, 2048, fs);
plot(w, abs(h))
%}

hp_b = round(32768 * hp_b);
hp_a = round(32768 * hp_a);

lp_b = round(32768 * lp_b);

% print out highpass filter coefficents
printf("Highpass Coefficents: \n");
printf("#define APU_HP_MULT_A0 %d\n", hp_a(1))
printf("#define APU_HP_MULT_A1 %d\n", hp_a(2))
printf("#define APU_HP_MULT_B0 %d\n", hp_b(1))
printf("#define APU_HP_MULT_B1 %d\n", hp_b(2))
printf("\n")

% print out lowpass filter kernel
printf("Lowpass Filter Kernel: \n");
for m = 1:4
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", lp_b(8 * (m - 1) + n))
    if ((m < 48) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("    %5d\n", lp_b(33))
printf("  };")
printf("\n\n")

