##########################################################################
## SimPipe: MIPS Pipeline Simulator based on SimMips  by Keiji Kimura   ##
##########################################################################
CC      = g++
OFLAG   = -O3 -Wall
LFLAG   = -lncurses
DEBUG   = -g

TARGET  = SimPipe
HEADER  = pipe.h
SOURCE  = main.cc pipe.cc cache.cc
OBJECT  = $(SOURCE:.cc=.o)
MIPSDIR = SimMips
DIRS = $(MIPSDIR)
INCFLAG   = -I$(MIPSDIR)
LIB  = $(MIPSDIR)/libmips.a
LMIPSFLAG = -L$(MIPSDIR) -lmips

##########################################################################
all:
	$(MAKE) $(TARGET)

main.cc: pipe.h cache.h $(MIPSDIR)/define.h
pipe.cc: pipe.h cache.h $(MIPSDIR)/define.h
cache.cc: cache.h $(MIPSDIR)/define.h

##########################################################################
$(TARGET): $(OBJECT) $(HEADER) $(LIB) Makefile
	$(CC) $(OFLAG) -o $@ $(OBJECT)  $(LMIPSFLAG) $(LFLAG)

$(LIB):
	cd $(DIRS); make lib

##########################################################################
debug: 
	$(CC) $(DEBUG) -o $(TARGET) $(SOURCE) $(LFLAG) $(LMIPSFLAG)
##########################################################################
.SUFFIXES :
.SUFFIXES : .o .cc

.cc.o: 
	$(CC) $(OFLAG) $(DEBUG) $(INCFLAG) -c $<

$(OBJECT) : $(HEADER) Makefile
##########################################################################

wc:
	wc -l $(HEADER) $(SOURCE)

indent:
	indent -kr -ts4 -nut main.cc

text:
	cats Makefile > code.txt
	echo -e "\nFile Organization by [wc *.h *.cc]" >> code.txt
	echo -e "  lines words bytes" >> code.txt
	wc $(HEADER) $(SOURCE) >> code.txt      
	echo -e "\f" >> code.txt
	cats -f $(HEADER) $(SOURCE) >> code.txt
	a2ps --medium=a4 -f 6.5 code.txt -o  code.ps
	ps2pdf13 -sPAPERSIZE=a4 code.ps
	rm -f code.txt code.ps

cflow:
	cflow *.cc

clean:
	rm -f *.o *.*~ *.exe $(TARGET) code.cc code.ps code.pdf
	cd $(DIRS); make clean
##########################################################################
run:
	./$(TARGET) $(MIPSDIR)/test/qsort
runlinux:
	./$(TARGET) -M $(MIPSDIR)/test/mem_qemu.txt $(MIPSDIR)/test/vmlinux-2.6.18-3-qemu
runmieru:
	./$(TARGET) -M $(MIPSDIR)/test/mem_mieru.txt $(MIPSDIR)/test/tokei
##########################################################################