#CXXFLAGS = -gstabs 
CXXFLAGS = -O3
LDFLAGS = -lstdc++ -s
BINDIR = /usr/local/bin
MANDIR = /usr/local/share/man/man1

rucnv: rucnv.o

rucnv.o: rucnv.cpp uninames.h trantabs.h subtab.h

clean:
	-rm rucnv *.o

install:
	install -m 755 -s rucnv $(BINDIR)
	gzip rucnv.1
	install -m 644 rucnv.1.gz $(MANDIR)
	gzip -d rucnv.1.gz
