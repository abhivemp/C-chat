# C-chat
An implementation of chat rooms using POSIX and C
NOTE: UPDATE ON README AND CODE CLEAN UP ON NEXT COMMIT

## Usage

After entering directory:
```
$ make
```

Run the `main_server` executable. 
```
.\main_server
```

For a client to connect:
Enter a username
```
./main_client localhost <username> <roomOption>
```

`roomOptions` can be the following:


&nbsp;&nbsp;&nbsp;&nbsp;1.`new` as a roomOption, the client connects to the next empty available room

&nbsp;&nbsp;&nbsp;&nbsp;2. An integer from 0 to 5 to connect to those rooms

&nbsp;&nbsp;&nbsp;&nbsp;3. The client can enter nothing and they'll receive a status of all the rooms in the server,

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;in which they can enter their number OR enter `new` again

