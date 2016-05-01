all: disk_test disk_mgr client server format

disk_test: src/disk_test.c src/utils.h src/utils.c
	gcc -o bin/disk_test src/disk_test.c src/utils.c

disk_mgr: src/disk_mgr.c src/utils.h src/utils.c
	gcc -o bin/disk_mgr src/disk_mgr.c src/utils.c

client: src/client.c src/utils.h src/utils.c
	gcc -o bin/client src/client.c src/utils.c

server: src/server.c src/fs_header.h src/fs_lib.c src/path.h src/path.c src/alloc.h src/utils.h src/alloc.c src/utils.c
	gcc -o bin/server src/server.c src/fs_lib.c src/path.c src/alloc.c src/utils.c

format: src/format.c src/alloc.h src/utils.h src/alloc.c src/utils.c
	gcc -o bin/format src/format.c src/alloc.c src/utils.c

clean:
	rm -f bin/*

