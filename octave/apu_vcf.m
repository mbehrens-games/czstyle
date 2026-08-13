clear;
clc;

% vcf clock: 3 khz

% vcf bend tables
vcf_bend_P = round(256 * exp(-log(2) * ((0:255)/64)));

vcf_bend_mults = 256 ./ [16 * ones(1,16), (16:495), 495 * ones(1, 16)];
vcf_bend_mults = round(256 * vcf_bend_mults);

% print out tables
printf("VCF Bend Periods Table: \n");
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", vcf_bend_P(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCF Bend Multipliers Table: \n");
for m = 1:64
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", vcf_bend_mults(8 * (m - 1) + n))
    if ((m < 64) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

