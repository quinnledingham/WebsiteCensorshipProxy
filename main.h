#ifndef MAIN_H
#define MAIN_H

struct addrinfo* addressInit(char* port);
// Returns server info for a server with port

int socketM(struct addrinfo server_info);
// Returns a new socket with the correct setup for the server

void bindM(int sock, struct addrinfo server_info);
// Binds the sock to the server

void listenM(int sock);
// Tells the server to listen for connections

int acceptM(int sock, struct addrinfo server_info);
// Returns the socket of the client of the proxyServer

int recvM(int clientSock, char* buffer);
// Get the messsage from the client to the proxyServer

void display(char* s);
// Prints to the console whatever is inside s

struct addrinfo* getHost(char* buffer);
// returns a struct with information about the ip to connect to

struct Buffer* getRequest(int tcp_socket);
// Gets the request from the browser

void connectM(int sock, struct addrinfo server_info);
// Connects sock to the server with server_info

void sendRequest(int sock, char* buffer, int bytes);
// Sends the request recieved from the browser to the server

struct Buffer* getResponse(int sock);
// Returns the response from the webserver to the request that was sent

void sendResponse(int sock, char* buffer, int bytes);
// Sends the response to the browser

int censorURL(char* buffer, int bytes, struct LinkedList *list);
// Returns 1 if the site should be censored, 0 if it should not be

int censorBODY(char* buffer, int bytes, struct LinkedList *list);
// Returns 1 if the site should be censored, 0 if it should not be

struct Node* newList(char* key);
// Creates a new list with the first node containing the key

void addNode(char* key, struct LinkedList *list);
// Adds a node to the list with the keyword from key

void printList(struct LinkedList *list);
// Prints the contents of list to the console

struct Buffer* forbidden();
// Returns the request for the blocked page.

int changeKeyword(char* buffer, struct LinkedList *list);
// Return 0 if it doesnt add a keyword. 1 if it does.

char* getContentType(char* buffer);
// Return the content type

#endif //MAIN_H