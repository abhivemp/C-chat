all:
	gcc -o chat_server chat_server.c -lpthread
	gcc -o chat_client chat_client.c
	gcc -o chat_server_full chat_server_full.c -lpthread
	gcc -o chat_client_full chat_client_full.c -lpthread
	gcc -o main_client main_client.c -lpthread
	gcc -o main_server main_server.c -lpthread
clean:
	rm chat_server chat_client
	rm chat_server_full chat_client_full
	rm main_server main_client
