# CS356-Mini-File-System

A Mini File System based on a pseudo disk manager, where the "disk" is simulated by a Linux file.



## Disk Control Protocols

(1) Query about Disk Info:

	Client >	I
	Server >	Disk Info: ...

(2) Read Request:

	Client >	R c s
	Server >	FOLLOW num
    Client >    EXPECT
	Server >	# data buffer 1
    Client >    EXPECT
	Server > 	# data buffer 2
    Client >    EXPECT
		...		...
	Server >	# data buffer N	(all buffers sum up to num bytes)

(3) Write Request:

	Client >	W c s n
	Server >	EXPECT
	Client >	# data buffer 1
	Server >	EXPECT
	Client > 	# data buffer 2
	Server >	EXPECT
		...		...
	Server >	EXPECT
	Client >	# data buffer N
	Server >	OVER

(4) Exit Request:

	Client >	E
	Server >	BYE



## File System Reply Codes

(1) 0: Task has been done.

(2) 1: Waiting for one buffer of data. After client sends data, server replies with 0 code.

(3) 2: Requesting to send data, which should be responded with "ACK". 
Then server would send the total number of bytes that will be sent, 
and every time client sends an "EXPECT" server would respond with one buffer of data.

(4) 3: Service has drawn to a close.

(5) 4: Execution failed.

(6) -1: Command is invalid.


##

BTW, I have a similar socket programming project [here](https://github.com/DevinZ1993/Notebook-of-SJTU/tree/master/courses/FTP), which is an FTP client-server system written in Java.

