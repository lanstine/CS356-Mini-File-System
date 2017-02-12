# Mini File System

A simplified file system based on a pseudo disk manager, where the "disk" is simulated by a 20 MB ext4 file.

## Big Picture

![Architecture](doc/arch.jpg)

## Build and Run

(0) To build, type in the top directory:

	make

(1) To start a disk manager:

	./bin/disk_mgr 10356

(2) To start a file system server:

	./bin/server 10356 10357

(3) To start a client-side shell:

	./bin/client 10357

![test](doc/test_script.png)

## Data Structures

Each disk block (512 bytes) is divided into eight 64-byte zones, and there are five types of zones.

(1) A dentry is a zone storing meta data of a directory, which forms a tree structure.

(2) A dir\_zone is the extension of a dentry, storing pointers to sub-dentries or inodes.

(3) An inode is the core of a file, through which we can get access to its data blocks.

(4) A data\_zone stores 64 bytes of data, namely the content of a file.

(5) An idx\_zone stores 32 pointers that point to other idx\_zones or data\_zones.

![Structures](doc/struct.jpg)

The first 4 zones of the first disk block is liable to keep track of the usage of all the disk zones, which constitute a "bitmap". 
The usage of any zone on the remaining part of the disk is indicated by two bits in bitmap, and hence a zone may have four possible states.

## Some Limits

(0) This FS design is AWFUL!!! Same i-node format should have been used for both regular files and directories in that a directory is only a special file whose data blocks are dentries. (I should have referred to S5FS, but I didn't... Orz)

(1) A free list like the one in S5FS should have been maintained, rather than the bitmap that I'm currently using. Now the disk size is restricted to 20 MB and could hardly be scaled up.

(2) The name of a file or directory could not be longer than 9 characters, and cannot include any whitespaces.

(3) The maxium size of a file is 136447 characters, which may not live up to real-world requirements.

(4) Whitespaces could not be written into a file, in that the interpreter of the server would separate it into tokens.

(5) No index has been provided to accelerate look-ups in a directory.

