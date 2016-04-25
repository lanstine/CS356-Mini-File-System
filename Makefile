all: disk_test

disk_test: src/disk_test.c disk_mgr
	gcc -o bin/disk_test src/disk_test.c src/header.h src/utils.c

disk_mgr: src/disk_mgr.c src/header.h src/utils.c
	gcc -o bin/disk_mgr src/disk_mgr.c src/header.h src/utils.c

clean:
	rm -f bin/*

