#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h> 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define BUFFER_SIZE 999999
#define PROXYPORTNUM 44102

struct Server
{
    struct addrinfo *info;
    int sock;
    int clientSock;
};

struct Client
{
    int sock;
    struct addrinfo *destInfo;
};

struct Buffer
{
    char *buffer;
    int bytesInBuffer;
};

struct Proxy 
{
    struct Buffer *data;
    struct Server proxyServer;
    struct Client proxyClient;
    int proxyClientSock;
};

struct Node
{
    struct Node* nextM;
    char keyword[100];
};

struct LinkedList
{
    struct Node* headM;
};

#include "main.h"

int main()
{
    // Keeps track of if some telnetted in to change keywords
    int telnet = 0;
    
    // Create keyword list to censor
    struct LinkedList *n;
    n = (struct LinkedList*)malloc(sizeof(struct LinkedList));
    n->headM = newList("Floppy");
    addNode("SpongeBob", n);
    //addNode("curling", n);
    printList(n);
    
    // Create proxy
    
    struct Proxy p;
    // Define port here
    char port[25];
    sprintf(port, "%d", PROXYPORTNUM);
    p.proxyServer.info = addressInit(port);
    p.proxyServer.sock = socketM(*p.proxyServer.info);
    bindM(p.proxyServer.sock, *p.proxyServer.info);
    listenM(p.proxyServer.sock);
    while (1)
    {
        printf("\n\nNEW LOOP\n#################################################\n");
        // If the telnet is closed or is closing wait for another client to join
        if (telnet == 0 || telnet == 2)
        {
            if (telnet == 2)
            {
                close(p.proxyServer.clientSock);
                telnet = 0;
            }
            p.proxyServer.clientSock = acceptM(p.proxyServer.sock, *p.proxyServer.info);
        }
        
        p.data = getRequest(p.proxyServer.clientSock);
        display(p.data->buffer);
        
        // Check if the command is to change a keyword or to get a website
        telnet = changeKeyword(p.data->buffer, n);
        if (!telnet)
        {
            // Check if the page should be censored based on the url
            int c = censorURL(p.data->buffer, p.data->bytesInBuffer, n);
            if (c)
            {
                fprintf(stderr, "\nCENSOR\n");
                //free(p.data->buffer);
                p.data = forbidden();
            }
            
            display(p.data->buffer);
            
            // Set up proxyClient with the web server
            p.proxyClient.destInfo = getHost(p.data->buffer);
            p.proxyClient.sock = socketM(*p.proxyClient.destInfo);
            connectM(p.proxyClient.sock, *p.proxyClient.destInfo);
            
            // Ask for and recieve the page from the web server
            sendRequest(p.proxyClient.sock, p.data->buffer, p.data->bytesInBuffer);
            //free(p.data->buffer);
            p.data = getResponse(p.proxyClient.sock);
            
#if 0 // Block based on body text
            if (censorBODY(p.data->buffer, p.data->bytesInBuffer, n))
            {
                p.data = forbidden();
                close(p.proxyClient.sock);
                p.proxyClient.sock = socketM(*p.proxyClient.destInfo);
                connectM(p.proxyClient.sock, *p.proxyClient.destInfo);
                sendRequest(p.proxyClient.sock, p.data->buffer, p.data->bytesInBuffer);
                p.data = getResponse(p.proxyClient.sock);
            }
#endif
            // fprintf(stderr, "\nContent Type: %s", getContentType(p.data->buffer));
            // Print the response to the console if it is just text/html
            if (!strcmp(getContentType(p.data->buffer), "text/html"))
                display(p.data->buffer);
            
            // Send the webpage to the browser
            sendResponse(p.proxyServer.clientSock, p.data->buffer, p.data->bytesInBuffer);
            
            close(p.proxyServer.clientSock);
        }
        else
        {
            // Print list to show changes to it
            printList(n);
        }
    }
    free(p.proxyServer.info);
    free(p.data->buffer);
    return 0;
}

struct addrinfo* addressInit(char* port)
{
    struct addrinfo hints, *server_info;
    server_info = (struct addrinfo*)malloc(sizeof(struct addrinfo));
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, port, &hints, &server_info);
    return server_info;
}

int socketM(struct addrinfo server_info)
{
    int sock = socket(server_info.ai_family,
                      server_info.ai_socktype,
                      server_info.ai_protocol);
    if (sock == -1)
    {
        fprintf(stderr, "socketM(): socket() called failed!\n");
        exit(1);
    }
    
    return sock;
}

void bindM(int sock, struct addrinfo server_info)
{
    if (bind(sock, server_info.ai_addr, server_info.ai_addrlen) == -1)
    {
        fprintf(stderr, "bindM(): bind() call failed!\n");
        exit(1);
    }
}

void listenM(int sock)
{
    if (listen(sock, 5) == -1)
    {
        fprintf(stderr, "socketListen(): listen() call failed!\n");
        exit(1);
    }
}

int acceptM(int sock, struct addrinfo server_info)
{
    int newsock;
    if ((newsock = accept(sock, server_info.ai_addr, &server_info.ai_addrlen)) == -1)
    {
        fprintf(stderr, "acceptM: accept() call failed!\n");
        exit(1);
    }
    return newsock;
}

void display(char *s)
{
    fprintf(stderr, "\nBuffer:\n############\n%s\n############\n", s);
}

struct addrinfo* getHost(char* buffer)
{
    
    struct hostent *hp;
    char hostname[100];
    // Extract hostname from the request
    char copybuffer[strlen(buffer)];
    sprintf(copybuffer, "%s", buffer);
    char *temphostname = strtok(copybuffer, "/");
    temphostname = strtok(NULL, "/");
    
    if (temphostname == NULL)
    {
        fprintf(stderr, "getHost: problem with hostname!\n");
        exit(1);
    }
    
    strcpy(hostname, temphostname);
    //fprintf(stderr, "\nhostname:\n%s\n\n", hostname);
    
    hp = gethostbyname(hostname);
    if (hp == NULL)
    {
        fprintf(stderr, "getHost: gethostbyname() call failed!\n");
        exit(1);
    }
    
    // Switch the hostname to ip and put it in the server_info
    struct addrinfo hints, *server_info;
    server_info = (struct addrinfo*)malloc(sizeof(struct addrinfo));
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname, NULL, &hints, &server_info);
    struct sockaddr_in *address = (struct sockaddr_in *)server_info->ai_addr;
    char ip[200];
    inet_ntop(server_info->ai_family, &(address->sin_addr), ip, 200);
    //fprintf(stderr, "\n%s\n", ip);
    
    getaddrinfo(ip, "80", &hints, &server_info);
    
    return server_info;
}

struct Buffer* getRequest(int tcp_socket)
{
    char message[70];
    struct Buffer *data;
    data = (struct Buffer*)malloc(sizeof(struct Buffer));
    char *cursor;
    data->buffer = malloc(BUFFER_SIZE);
    size_t recv_size;
    data->bytesInBuffer = 0;
    
    cursor = data->buffer;
    while (1)
    {
        // Keep adding what is recieved to the buffer until what is recieved is less than the
        // max. Cursor follows where to put the new data recieved.
        memset(message, 0, sizeof(message));
        recv_size = recv(tcp_socket, &message, sizeof(message), 0);
        
        //fprintf(stderr, "\n%ld\n", recv_size);
        data->bytesInBuffer = data->bytesInBuffer + recv_size;
        memcpy(cursor, message, recv_size);
        cursor = cursor + recv_size;
        
        char* u = strstr(data->buffer, "\r\n\r\n");
        if (u != 0)
            break;
        
        //if (recv_size < 70)
        //break;
        //send(tcp_socket, 0, 0, 0);
    }
    //close(tcp_socket);
    return data;
}

void connectM(int sock, struct addrinfo server_info)
{
    if ((connect(sock, server_info.ai_addr, server_info.ai_addrlen)) == -1)
    {
        fprintf(stderr, "connectM: connect() call failed!\n");
    }
}

void sendRequest(int sock, char* buffer, int bytes)
{
    //fprintf(stderr, "\nsendRequest: bytes: %d\n", bytes);
    if (send(sock, buffer, bytes, 0) < 0)
    {
        fprintf(stderr, "sendRequest: send() call failed!\n");
    }
    //fprintf(stderr, "\n%d\n", bytes);
}

struct Buffer* getResponse(int sock)
{
    // Same as getRequest
    char message[80];
    struct Buffer *data;
    data = (struct Buffer*)malloc(sizeof(struct Buffer));
    char *cursor;
    data->buffer = malloc(BUFFER_SIZE);
    size_t recv_size;
    data->bytesInBuffer = 0;
    cursor = data->buffer;
    while (1)
    {
        //fprintf(stderr, "\n%ld\n", recv_size);
        memset(message, 0, sizeof(message));
        recv_size = recv(sock, &message, sizeof(message), 0);
        if (recv_size == 0)
            break;
        data->bytesInBuffer = data->bytesInBuffer + recv_size;
        memcpy(cursor, message, recv_size);
        cursor = cursor + recv_size;
    }
    
    close(sock);
    return data;
}

void sendResponse(int sock, char* buffer, int bytes)
{
    //fprintf(stderr, "\n%d\n", bytes);
    if (send(sock, buffer, bytes, 0) < 0)
    {
        fprintf(stderr, "%d", errno);
        fprintf(stderr, "sendResponse: send() call failed!\n");
    }
}

int censorURL(char* buffer, int bytes, struct LinkedList *list)
{
    int censor = 0;
    
    char copy[1000];
    // copys the buffer up to 1000 char to the copy char*
    if ((snprintf(copy, 1000, "%s", buffer)) < 0) {
        fprintf(stderr, "censor(): snprintf did not work");
    }
    char*u = strtok(copy, " ");
    u = strtok(NULL, " ");
    //fprintf(stderr, "\nu: %s\n", u);
    
    struct Node *cursor;
    cursor = list->headM;
    
    // Go through each word in the list to see if any of them can be found
    // in the buffer.
    while(cursor != 0)
    {
        char* k = strstr(u, cursor->keyword);
        
        if (k != 0) {
            censor = 1;
        }
        
        if (cursor->nextM != 0)
            cursor = cursor->nextM;
        else
            break;
    }
    return censor;
}

int censorBODY(char* buffer, int bytes, struct LinkedList *list)
{
    int censor = 0;
    
    char copy[1000];
    // copys the buffer up to 1000 char to the copy char*
    if ((snprintf(copy, 1000, "%s", buffer)) < 0) {
        fprintf(stderr, "censor(): snprintf did not work");
    }
    /*
    char*u = strtok(copy, "\r\n\r\n");
    for (int i = 0; i < 9; i++)
        u = strtok(NULL, "\r\n\r\n");
    fprintf(stderr, "\nbodyu: %s\n", u);
    */
    char* u = strstr(copy, "\r\n\r\n");
    u = u + 2;
    //fprintf(stderr, "\nbodyu: %s\n", u);
    
    struct Node *cursor;
    cursor = list->headM;
    
    // Go through each word in the list to see if any of them can be found
    // in the buffer.
    while(cursor != 0)
    {
        char* k = strstr(u, cursor->keyword);
        
        if (k != 0) {
            censor = 1;
        }
        
        if (cursor->nextM != 0)
            cursor = cursor->nextM;
        else
            break;
    }
    return censor;
}

struct Node* newList(char* key)
{
    // Creates a new list. First node has key as its keyword.
    // Gets NextM set to end which is to be always the last node.
    struct Node *n;
    struct Node *end;
    n = (struct Node*)malloc(sizeof(struct Node));
    end = (struct Node*)malloc(sizeof(struct Node));
    strcpy(n->keyword, key);
    strcpy(end->keyword, "END");
    n->nextM = end;
    return n;
}

void addNode(char* key, struct LinkedList *list)
{
    // Add a node with key as the keyword right before the node with
    // the keyword "END".
    struct Node *cursor;
    cursor = list->headM;
    //fprintf(stderr, "\nheadM: %s\n", list->headM->keyword);
    while(strcmp(cursor->nextM->keyword, "END"))
    {
        //fprintf(stderr, "\nCursor: %s\n", cursor->nextM->keyword);
        cursor = cursor->nextM;
    }
    
    struct Node *newN = (struct Node*)malloc(sizeof(struct Node));
    newN->nextM = cursor->nextM;
    cursor->nextM = newN;
    strcpy(cursor->nextM->keyword, key);
}

void printList(struct LinkedList *list)
{
    struct Node *cursor;
    cursor = list->headM;
    printf("\nKeyword List:\n");
    while(strcmp(cursor->keyword, "END"))
    {
        if (strcmp(cursor->keyword, "Skip"))
            printf("\nKeyword: %s \n", cursor->keyword);
        
        cursor = cursor->nextM;
    }
    
}

struct Buffer* forbidden()
{
    // Changes the request to a request for the error page.
    struct Buffer *data;
    data = (struct Buffer*)malloc(sizeof(struct Buffer));
    memset(&data->buffer, 0, sizeof(data->buffer));
    
    // Request for error page
    data->buffer = 
        "GET http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/error.html HTTP/1.1\r\n"
        "Host: pages.cpsc.ucalgary.ca\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Upgrade-Insecure-Requests: 1\r\n"
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.0 Safari/605.1.15\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Accept-Language: en-US,en;q=0.90\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n";
    
    data->bytesInBuffer = 415;
    
    return data;
}

int changeKeyword(char* buffer, struct LinkedList *list)
{
    int change = 0;
    // Add a keyword to the list of blocked words
    char* a = strstr(buffer, "KEYWORDA");
    if (a != 0) 
    {
        change = 1;
        char copybuffer[strlen(buffer)];
        int size = strcspn(buffer, "\n");
        snprintf(copybuffer, size, "%s", buffer);
        char* temp = strtok(copybuffer, "+\n");
        temp = strtok(NULL, "+");
        addNode(temp, list);
    }
    else
    {
        // Delete a keyword from the list of blocked words
        char* d = strstr(buffer, "KEYWORDD");
        if (d != 0)
        {
            change = 1;
            
            struct Node *cursor;
            cursor = list->headM;
            
            while(strcmp(cursor->keyword, "END"))
            {
                char* k = strstr(buffer, cursor->keyword);
                
                if (k != 0) {
                    strcpy(cursor->keyword, "Skip");
                }
                
                cursor = cursor->nextM;
            }
            
        }
        else
        {
            // Exit from the telnet to let another user join
            char* e = strstr(buffer, "EXIT");
            if (e != 0)
            {
                change = 2;
            }
        }
    }
    
    // Otherwise it was not a telnet request
    return change;
}

char* getContentType(char* buffer) 
{
    char copy[1000];
    // copys the buffer up to 1000 char to the copy char*
    if ((snprintf(copy, 1000, "%s", buffer)) < 0) {
        fprintf(stderr, "getContentType(): snprintf did not work");
    }
    //fprintf(stderr, "\ncopy of header:\n\%sn", copy);
    // Extracts the contenttype if it exists
    char *contentType = strstr(copy, "Content-Type");
    if (contentType != 0)
    {
        contentType = strtok(contentType, " ");
        contentType = strtok(NULL, ";\r");
    }
    
    return contentType;
}