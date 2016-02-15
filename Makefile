all: BDS BDC_command BDC_random IDS FS FC

clean:
	rm -f bin/*

BDS: src/BDS.c src/cse356header.h
	g++ src/BDS.c src/cse356header.h -o bin/BDS

BDC_command: src/BDC_command.c src/cse356header.h
	g++ src/BDC_command.c src/cse356header.h -o bin/BDC_command

BDC_random: src/BDC_random.c src/cse356header.h
	g++ src/BDC_random.c src/cse356header.h -o bin/BDC_command

IDS: src/IDS.c src/cse356header.h
	g++ src/IDS.c src/cse356header.h -o bin/IDS

FS: src/FS.c src/cse356header.h
	g++ src/FS.c src/cse356header.h -o bin/FS

FC: src/FC.c src/cse356header.h
	g++ src/FC.c src/cse356header.h -o bin/FC

