
CC      =  cc
CFLAGS  =
COPTIM  = -O 


SRC     = taxsolve_routines.c
LIBS    =


# It would be nice to create a taxsolve_routines.h so the routines.c source only needs to be compiled once. 
# OBJS    =  taxsolve_routines.o
OBJS    =


# Rule to compile objects from source
#.c.o:
#	$(CC)  -c  $(CFLAGS)  $*.c


all: ../bin/taxsolve_US_1040_2018 \
     ../bin/taxsolve_US_1040_Sched_C_2018 \
     ../bin/taxsolve_PA_40_2018 \
     ../bin/taxsolve_NJ_1040_2018 \
     ../bin/taxsolve_VA_760_2018 \
     ../bin/taxsolve_NC_D400_2018 \
     ../bin/taxsolve_OH_IT1040_2018 \
     ../bin/taxsolve_NY_IT201_2018 \
     ../bin/taxsolve_MA_1_2018 \
     ../bin/taxsolve_CA_540_2018 \
     ../bin/universal_pdf_file_modifier \
     ../bin/convert_results2xfdf \
     ../Run_taxsolve_GUI 


../bin/taxsolve_US_1040_2018:         taxsolve_US_1040_2018.c          taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_US_1040_2018         taxsolve_US_1040_2018.c          $(SRCS) $(LIBS)

../bin/taxsolve_US_1040_Sched_C_2018: taxsolve_US_1040_Sched_C_2018.c  taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_US_1040_Sched_C_2018 taxsolve_US_1040_Sched_C_2018.c  $(SRCS) $(LIBS)

../bin/taxsolve_CA_540_2018:          taxsolve_CA_540_2018.c           taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_CA_540_2018          taxsolve_CA_540_2018.c           $(SRCS) $(LIBS)

../bin/taxsolve_NC_D400_2018:            taxsolve_NC_D400_2018.c       taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_NC_D400_2018         taxsolve_NC_D400_2018.c	    $(SRCS) $(LIBS)

../bin/taxsolve_PA_40_2018:          taxsolve_PA_40_2018.c            taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_PA_40_2018           taxsolve_PA_40_2018.c            $(SRCS) $(LIBS)

../bin/taxsolve_NJ_1040_2018:         taxsolve_NJ_1040_2018.c          taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_NJ_1040_2018         taxsolve_NJ_1040_2018.c          $(SRCS) $(LIBS)

../bin/taxsolve_OH_IT1040_2018:       taxsolve_OH_IT1040_2018.c        taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_OH_IT1040_2018       taxsolve_OH_IT1040_2018.c        $(SRCS) $(LIBS)

../bin/taxsolve_VA_760_2018:          taxsolve_VA_760_2018.c           taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_VA_760_2018          taxsolve_VA_760_2018.c           $(SRCS) $(LIBS)

../bin/taxsolve_NY_IT201_2018:       taxsolve_NY_IT201_2018.c          taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_NY_IT201_2018        taxsolve_NY_IT201_2018.c         $(SRCS) $(LIBS)

../bin/taxsolve_MA_1_2018:            taxsolve_MA_1_2018.c             taxsolve_routines.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/taxsolve_MA_1_2018            taxsolve_MA_1_2018.c             $(SRCS) $(LIBS)

../bin/universal_pdf_file_modifier: 	      universal_pdf_file_modifier.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/universal_pdf_file_modifier  universal_pdf_file_modifier.c	$(SRCS) $(LIBS)

../bin/convert_results2xfdf: 	      convert_results2xfdf.c
	$(CC) $(CFLAGS) $(COPTIM) -o  ../bin/convert_results2xfdf  convert_results2xfdf.c	$(SRCS) $(LIBS)

../Run_taxsolve_GUI:		      Run_taxsolve_GUI.c
	$(CC) $(CFLAGS) $(COPTIM) Run_taxsolve_GUI.c -o ../Run_taxsolve_GUI

clean:
	/bin/rm -fv ../bin/taxsolve* ../Run_taxsolve_GUI ../bin/convert_results2xfdf
