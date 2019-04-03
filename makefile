
.PHONY: all
all: client server 

.PHONY: client
client: chatClient.cpp
	g++ -o client chatClient.cpp -g
.PHONY: server
server: chatServer.cpp
	g++ -o server chatServer.cpp -g

.PHONY: open
open:
	vim *.cpp *.h README makefile 

.PHONY: backup
backup:
	cp *.cpp *.h ./backup/

	
