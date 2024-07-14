main: write_server read_server

read_server: read_server_1.c
	gcc read_server_1.c -D READ_SERVER -o read_server
write_server:write_server_1.c
	gcc write_server_1.c -D WRITE_SERVER -o write_server

clean:
	rm write_server read_server
