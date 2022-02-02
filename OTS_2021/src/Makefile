
CC      =  cc
CFLAGS  =
COPTIM  = -O -Wall


SRC     = taxsolve_routines.c
LIBS    =


# It would be nice to create a taxsolve_routines.h so the routines.c source only needs to be compiled once. 
# OBJS    =  taxsolve_routines.o
OBJS    =


# Rule to compile objects from source
#.c.o:
#	$(CC)  -c  $(CFLAGS)  $*.c


all: ../bin/taxsolve_US_1040_2021 \
     ../bin/taxsolve_US_1040_Sched_C_2021 \
     ../bin/taxsolve_PA_40_2021 \
     ../bin/taxsolve_NJ_1040_2021 \
     ../bin/taxsolve_VA_760_2021 \
     ../bin/taxsolve_NC_D400_2021 \
     ../bin/taxsolve_OH_IT1040_2021 \
     ../bin/taxsolve_NY_IT201_2021 \
     ../bin/taxsolve_MA_1_2021 \
     ../bin/taxsolve_CA_540_2021 \
     ../bin/taxsolve_HSA_f8889 \
     ../bin/taxsolve_f8606 \
     ../bin/taxsolve_US_1040_Sched_SE_2021 \
     ../bin/taxsolve_f8959_2021 \
     ../bin/taxsolve_f8960_2021 \
     ../bin/taxsolve_f2210_2021 \
     ../bin/taxsolve_CA_5805_2021 \
     ../bin/universal_pdf_file_modifier \
     ../bin/convert_results2xfdf \
     ../Run_taxsolve_GUI 


../bin/taxsolve_US_1040_2021:         taxsolve_US_1040_2021.c          taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_US_1040_2021         taxsolve_US_1040_2021.c          $(SRCS) $(LIBS)

../bin/taxsolve_US_1040_Sched_C_2021: taxsolve_US_1040_Sched_C_2021.c  taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_US_1040_Sched_C_2021 taxsolve_US_1040_Sched_C_2021.c  $(SRCS) $(LIBS)

../bin/taxsolve_CA_540_2021:          taxsolve_CA_540_2021.c           taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_CA_540_2021          taxsolve_CA_540_2021.c           $(SRCS) $(LIBS)

../bin/taxsolve_NC_D400_2021:            taxsolve_NC_D400_2021.c       taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_NC_D400_2021         taxsolve_NC_D400_2021.c	    $(SRCS) $(LIBS)

../bin/taxsolve_PA_40_2021:          taxsolve_PA_40_2021.c            taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_PA_40_2021           taxsolve_PA_40_2021.c            $(SRCS) $(LIBS)

../bin/taxsolve_NJ_1040_2021:         taxsolve_NJ_1040_2021.c          taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_NJ_1040_2021         taxsolve_NJ_1040_2021.c          $(SRCS) $(LIBS)

../bin/taxsolve_OH_IT1040_2021:       taxsolve_OH_IT1040_2021.c        taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_OH_IT1040_2021       taxsolve_OH_IT1040_2021.c        $(SRCS) $(LIBS)

../bin/taxsolve_VA_760_2021:          taxsolve_VA_760_2021.c           taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_VA_760_2021          taxsolve_VA_760_2021.c           $(SRCS) $(LIBS)

../bin/taxsolve_NY_IT201_2021:       taxsolve_NY_IT201_2021.c          taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_NY_IT201_2021        taxsolve_NY_IT201_2021.c         $(SRCS) $(LIBS)

../bin/taxsolve_MA_1_2021:            taxsolve_MA_1_2021.c             taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_MA_1_2021            taxsolve_MA_1_2021.c             $(SRCS) $(LIBS)

../bin/taxsolve_HSA_f8889:            taxsolve_HSA_f8889.c             taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_HSA_f8889            taxsolve_HSA_f8889.c		     $(SRCS) $(LIBS)

../bin/taxsolve_f8606:            taxsolve_f8606.c             taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_f8606		taxsolve_f8606.c		     $(SRCS) $(LIBS)

../bin/taxsolve_US_1040_Sched_SE_2021: taxsolve_US_1040_Sched_SE_2021.c taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_US_1040_Sched_SE_2021 taxsolve_US_1040_Sched_SE_2021.c $(SRCS) $(LIBS)

../bin/taxsolve_f8959_2021:            taxsolve_f8959_2021.c  taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_f8959_2021    taxsolve_f8959_2021.c	$(SRCS) $(LIBS)

../bin/taxsolve_f8960_2021:            taxsolve_f8960_2021.c  taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_f8960_2021    taxsolve_f8960_2021.c	$(SRCS) $(LIBS)

../bin/taxsolve_f2210_2021:            taxsolve_f2210_2021.c  taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_f2210_2021    taxsolve_f2210_2021.c	$(SRCS) $(LIBS)

../bin/taxsolve_CA_5805_2021:            taxsolve_CA_5805_2021.c  taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_CA_5805_2021   taxsolve_CA_5805_2021.c  	$(SRCS) $(LIBS)	

../bin/universal_pdf_file_modifier: 	      universal_pdf_file_modifier.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/universal_pdf_file_modifier  universal_pdf_file_modifier.c	$(SRCS) $(LIBS)

../bin/convert_results2xfdf: 	      convert_results2xfdf.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/convert_results2xfdf  convert_results2xfdf.c	$(SRCS) $(LIBS)

../Run_taxsolve_GUI:		      Run_taxsolve_GUI.c
	$(CC) $(CFLAGS) $(COPTIM) Run_taxsolve_GUI.c -o ../Run_taxsolve_GUI

clean:
	/bin/rm -fv ../bin/taxsolve* ../Run_taxsolve_GUI ../bin/convert_results2xfdf
