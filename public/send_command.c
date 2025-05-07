#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "command.h"

bool send_command(char *host_ip, int port, struct command_req* req, struct command_rsp* rsp) {
    char ip[32];
    if(!host_ip) {
        strcpy(ip, "127.0.0.1");
    }
    if(port <= 0) {
        port = NODE_PORT;
    }

    int client_socket;
    struct sockaddr_in server_address;
    
    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Error creating socket");
        return false;
    }
    
    // Set server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    
    // Convert IP address from string to binary form
    
    if (inet_pton(AF_INET, ip, &(server_address.sin_addr.s_addr)) <= 0) {
        perror("Invalid address/ Address not supported");
        return false;
    }
    int max_buffer_size = 1024 * 1024;
    int flag = setsockopt(client_socket,
                       SOL_SOCKET,
                       SO_SNDBUF,
                       &max_buffer_size,
                       sizeof(max_buffer_size));
    if (flag < 0) {
        perror("setsockopt SO_SNDBUF");
        // 错误处理
    }
    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Error connecting to server");
        return false;
    }
    
    printf("Connected to server\n");
    // Send command_req to server
    if (send(client_socket, req, sizeof(struct command_req), 0) < 0) {
        perror("Error sending command_req");
        return false;
    }
    
    // Receive command_rsp from server
    if (recv(client_socket, rsp, sizeof(struct command_rsp), 0) < 0) {
        printf("Error receiving command_rsp %s\n", ip);
        perror("Error receiving command_rsp");
        return false;
    }
    
    return true;
}
