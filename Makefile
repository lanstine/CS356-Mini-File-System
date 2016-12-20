all: disk_test disk_mgr client server format

utils_o: src/utils.c src/utils.h
	gcc -o obj/utils.o -c src/utils.c

disk_test_o: src/disk_test.c src/utils.h
	gcc -o obj/disk_test.o -c src/disk_test.c

disk_test: utils_o disk_test_o
	gcc -o bin/disk_test obj/disk_test.o obj/utils.o

disk_mgr_o: src/disk_mgr.c src/utils.h
	gcc -o obj/disk_mgr.o -c src/disk_mgr.c

disk_mgr: utils_o disk_mgr_o
	gcc -o bin/disk_mgr obj/disk_mgr.o obj/utils.o

client_o: src/client.c src/utils.h
	gcc -o obj/client.o -c src/client.c

client: utils_o client_o
	gcc -o bin/client obj/client.o obj/utils.o

fs_o: src/fs_lib.c src/fs_header.h
	gcc -o obj/fs.o -c src/fs_lib.c

path_o: src/path.c src/path.h
	gcc -o obj/path.o -c src/path.c

alloc_o: src/alloc.c src/alloc.h
	gcc -o obj/alloc.o -c src/alloc.c

server_o: src/server.c src/fs_header.h
	gcc -o obj/server.o -c src/server.c

server: server_o fs_o alloc_o path_o utils_o
	gcc -o bin/server obj/server.o obj/fs.o obj/alloc.o obj/path.o obj/utils.o

format_o: src/format.c src/alloc.h
	gcc -o obj/format.o -c src/format.c

format: format_o alloc_o utils_o
	gcc -o bin/format obj/format.o obj/alloc.o obj/utils.o

clean:
	rm -f bin/* obj/*

