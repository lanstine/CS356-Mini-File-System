# CS356-Mini-File-System

A Mini File System based on a client-server disk management system, where the "disk" is simulated by a Linux file.


1. Disk Management Protocols

(1) Query about Disk Info:

	Client >	I
	Server >	Disk Info: ...

(2) Read Request:

	Client >	R c s
	Server >	FOLLOW num
    Client >    EXPECT
	Server >	# data round 1
    Client >    EXPECT
	Server > 	# data round 2
    Client >    EXPECT
		...		...
	Server >	# data round N	(all round sum up to num bytes)

(3) Write Request:

	Client >	W c s n
	Server >	EXPECT
	Client >	# data round 1
	Server >	EXPECT
	Client > 	# data round 2
	Server >	EXPECT
		...		...
	Server >	EXPECT
	Client >	# data round N
	Server >	OVER

(4) Exit Request:

	Client >	E
	Server >	BYE
