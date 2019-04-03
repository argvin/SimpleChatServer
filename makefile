
.PHONY: all
all: client server 

.PHONY: client
client: ./src/chatClient.cpp
	g++ -o client ./src/chatClient.cpp -g
	mv client ./bin/
.PHONY: server
server: ./src/chatServer.cpp
	g++ -o server ./src/chatServer.cpp -g
	mv server ./bin/

.PHONY: clean
clean:
	rm ./bin/*

.PHONY: open
open:
	vim ./src/*.cpp ./src/*.h README makefile 

.PHONY: backup
backup:
	cp ./src/*.cpp ./src/*.h ./backup/


	
