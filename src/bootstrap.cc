#include "middleware.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 256
#define BOOTSTRAPPER_CONF_FILE "../conf/bootstrapper.txt"
#define Send_Successor "./send_successor"
#define Send_Successor_2 "./send_successor_2"
// ./send_successor id_suc ip_suc
// ./send_successor_2 id_new  n_suc ip_suc

std::string include_node_call(char *, char *);
std::string get_next_ip();
void set_next_ip(std::string);

int main(int argc, char *argv[])
{
    int sockfd, listener;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    int pid;

    int n = 0; // number of nodes that entered the system

    // Just listens to socket:
    listener = start_listening(PORT_NUMBER);

    while(1){
        // Accepting Connection. Receiving command request:
        log(LOG_LEVEL_INFO, "Bootstrap Server listening to port %d...", PORT_NUMBER);
        sockfd = accept(listener, (struct sockaddr *) &client_addr, &client_len);
        if (sockfd < 0)
            error("ERROR on accept");
        log(LOG_LEVEL_INFO, "Connection stabilished from %s", inet_ntoa(client_addr.sin_addr));
        n++; // add node counter

        pid = fork();
        if(pid>0){
            close(sockfd); // Closes the accepted socket. and resume accepted loop
        }
        else {
            close(listener); // Closes listener socket

            char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
            bzero(buffer, BUFFER_SIZE);
            read(sockfd,buffer,255);
            std::string connected_IP(buffer);

            // start connection to node
            if (n == 1){ // no system yet
                bzero(buffer, BUFFER_SIZE);
                strcpy(buffer,"1 1 ");
                strcat(buffer,connected_IP.c_str());

                log(LOG_LEVEL_INFO, "No Network yet. Sending self ip: %s", buffer);
            	write(sockfd, buffer, strlen(buffer));
            }
            else {
                log(LOG_LEVEL_INFO, "Alredy have Network. Finding next ip...");
                std::string next_IP = get_next_ip();
                log(LOG_LEVEL_INFO, "Next IP: %s", next_IP.c_str());

                // send to node
                bzero(buffer, BUFFER_SIZE);
            	strcpy(buffer, Send_Successor);
                strcat(buffer," ");
                strcpy(buffer,std::to_string(n).c_str());
                strcat(buffer," ");
                strcat(buffer,connected_IP.c_str());
                log(LOG_LEVEL_INFO, "Sending to %s: %s", next_IP.c_str(), buffer);
            	std::string id_and_ip = include_node_call((char*)next_IP.c_str(), (char*)buffer);

            	// send back successor
                bzero(buffer, BUFFER_SIZE);
                strcpy(buffer,std::to_string(n).c_str());
                strcat(buffer," ");
                strcat(buffer,id_and_ip.c_str());
                log(LOG_LEVEL_INFO, "Sending to %s: %s", connected_IP.c_str(), buffer);
                write(sockfd, buffer, strlen(buffer));
            }

            close(sockfd); // When done, closes accepted socket

            set_next_ip(connected_IP);
        }

    }
    return 0;
}


std::string get_next_ip(){
    ifstream fp;
    string line;

    fp.open(BOOTSTRAPPER_CONF_FILE);
    if (!fp.is_open())
        printf("ERRO ao abrir arquivo de pares");

    getline(fp,line); // first ip in bootstrapper file is next ip.
    fp.close();
    return line;
}

void set_next_ip(std::string ip){
    ofstream fp;
    string line;

    fp.open(BOOTSTRAPPER_CONF_FILE, std::ofstream::out | std::ofstream::trunc);
    if (!fp.is_open())
        printf("ERRO ao abrir arquivo de pares");

    fp<<ip;
    fp<<"\n";
    fp.close();
}


std::string include_node_call(char *hostnameOrIp, char *command){
	std::string to_return = "";
    char buffer[FILE_SIZE];
    log(LOG_LEVEL_INFO, "Connecting to server %s on port %d...", hostnameOrIp, PORT_NUMBER);

    int sockfd = connectSocket(hostnameOrIp, PORT_NUMBER);
    if (sockfd == -1) {
        error("Could not connect with server");
    }else {
        // Sending command to server:
        int n = write(sockfd,command,strlen(command));
        if (n < 0){
             error("ERROR writing to socket");
        } else {
            // Receiving response:
            bzero(buffer,FILE_SIZE);
            while (read(sockfd,buffer,255) != 0) {
                to_return += buffer;
                bzero(buffer,FILE_SIZE);
            }
            printf("\n");
            close(sockfd);
        }
    }
    return to_return;
}

