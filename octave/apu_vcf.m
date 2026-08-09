clear;
clc;

% vcf clock: 3 khz

% vcf saw breakpoints (max/min values)
vcf_bend_bp_1 = round(256 * exp(-log(2) * ((0:255)/64)));
vcf_bend_bp_2 = round(1024 - 256 * exp(-log(2) * ((0:255)/64)));

% vcf square breakpoints (max/min on falling edge)
vcf_bend_bp_3 = round(512 - 256 * exp(-log(2) * ((0:255)/64)));
vcf_bend_bp_4 = round(512 + 256 * exp(-log(2) * ((0:255)/64)));

% vcf double sine breakpoint (min of 1st sine wave)
vcf_bend_bp_5 = round(768 * exp(-log(2) * ((0:255)/64)));

% vcf slopes (8 bit mantissa)
vcf_bend_P_slope = round(256 * exp(log(2) * ((0:255)/64)));
vcf_bend_Q_slope = round(256 * exp(log(2) * ((0:255)/64)));

% print out tables
printf("VCF Bend Breakpoint 1 Table: \n");
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", vcf_bend_bp_1(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCF Bend Breakpoint 2 Table: \n");
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", vcf_bend_bp_2(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCF Bend Breakpoint 3 Table: \n");
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", vcf_bend_bp_3(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCF Bend Breakpoint 4 Table: \n");
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", vcf_bend_bp_4(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("VCF Bend Breakpoint 5 Table: \n");
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", vcf_bend_bp_5(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")
