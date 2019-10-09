##########################################################################
## SimMips: Simple Computer Simulator of MIPS    Arch Lab. TOKYO TECH   ##
##########################################################################
CC      = g++
OFLAG   = -O3 -Wall
LFLAG   = -lncurses
DEBUG   = -g

TARGET  = SimMips
HEADER  = define.h
SOURCE  = main.cc board.cc memory.cc simloader.cc mips.cc mipsinst.cc cp0.cc device.cc
OBJECT  = $(SOURCE:.cc=.o)
LIBOBJ  = board.o memory.o simloader.o mips.o mipsinst.o cp0.o device.o
LIB	= libmips.a
##########################################################################
all:
	$(MAKE) $(TARGET)
##########################################################################
$(TARGET): $(SOURCE) $(HEADER) Makefile
	$(CC) $(OFLAG) -o $@ $(SOURCE) $(LFLAG)
##########################################################################
debug: 
	$(CC) $(DEBUG) -o $(TARGET) $(SOURCE) $(LFLAG)
##########################################################################
.SUFFIXES :
.SUFFIXES : .o .cc

.cc.o: 
	$(CC) $(OFLAG) -c $<

$(OBJECT) : $(HEADER) Makefile
##########################################################################
$(LIB): $(LIBOBJ)
	ar -qv $(LIB) $(LIBOBJ)

lib:
	make $(LIB)

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
	rm -f *.o *.*~ *.exe $(TARGET) $(LIB) code.cc code.ps code.pdf
##########################################################################
run:
	./$(TARGET) test/qsort
runlinux:
	./$(TARGET) -M test/mem_qemu.txt test/vmlinux-2.6.18-3-qemu
runmieru:
	./$(TARGET) -M test/mem_mieru.txt test/tokei
##########################################################################