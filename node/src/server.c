#include "server.h"

#define MAX_BUFFER_SIZE 1024

void handle_req_task_submit(struct command_req *req, struct command_rsp *rsp) {
    struct subtask_params task_submit = req->task_submit;
    printf("task_submit: %s\n", task_submit.name);
    sche_enqueue_subtask_params(task_submit);
    rsp->status = 0;
}

void handle_command_req(int client_socket) {
    struct command_req *req = (struct command_req *)malloc(sizeof(struct command_req));
    struct command_rsp *rsp = (struct command_rsp *)malloc(sizeof(struct command_rsp));
    
    // Receive command_req from client
    
    if (recv(client_socket, req, sizeof(struct command_req), 0) < 0) {
        perror("Error receiving command_req");
        return;
    }
    
    // Process command_req
    if(req->type == TASK_SUBMIT) {
        printf("TASK_SUBMIT\n");
        handle_req_task_submit(req, rsp);
    }  else { 
        rsp->status = -1;
    }

    // Send command_rsp to client
    rsp->status = 0;
    if (send(client_socket, rsp, sizeof(struct command_rsp), 0) < 0) {
        perror("Error sending command_rsp");
        return;
    }
}

int init_server(void *arg) {
    char msgq_name[NAME_MAX];
    /* Resource create */
    // p->msgq_re
    struct rte_ring *command_queue = rte_ring_create("COMMAND-QUEUE",
                                    PIPELINE_MSGQ_SIZE,
                                    SOCKET_ID_ANY,
                                    RING_F_SP_ENQ | RING_F_SC_DEQ);
    if (command_queue == NULL)
        return -1;
    
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length;
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        return -1;
    }
    
    // Set server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(NODE_PORT);
    
    int recv_buffer_size = 1024*1024; // 设置为1MB
    int flag = setsockopt(server_socket,
                        SOL_SOCKET,
                        SO_RCVBUF,
                        &recv_buffer_size,
                        sizeof(recv_buffer_size));
    if (flag < 0) {
        perror("setsockopt SO_RCVBUF");
        // 错误处理
    }
    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Error binding socket");
        return -1;
    }
    
    // Listen for connections
    if (listen(server_socket, 5) < 0) {
        perror("Error listening for connections");
        return -1;
    }
    
    printf("Server listening on port %d\n", NODE_PORT);
    
    while (1) {
        // Accept client connection
        client_address_length = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length);
        if (client_socket < 0) {
            perror("Error accepting client connection");
            return -1;
        }
        
        printf("Client connected\n");
        
        // Handle command_req
        handle_command_req(client_socket);
        
        // Close client socket
        close(client_socket);
        
        printf("Client disconnected\n");
    }
    
    // Close server socket
    close(server_socket);
    return 0;
}

