#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

char* get_f_name(char* cmd);

int main(void){
	printf("server mode...\naddress: ");
	fflush(stdout);
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int valread, socknew;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	//char buffer[1024] = {0};
	
	
	if(sockfd == 0){
		printf("socket failed");
		exit(1);
	}
	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(8080);
	
	char address_string[INET_ADDRSTRLEN];
	inet_ntop( AF_INET, &address.sin_addr, address_string, sizeof( address_string ));
	printf("%s\n", address_string);
	
	if (bind(sockfd, (struct sockaddr *)&address,sizeof(address))<0){
		printf("bind failed");
		exit(1);
	}
	
	
	if (listen(sockfd, 3) < 0){
		printf("listen error");
		exit(1);
	}
	
	if ((socknew = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
		printf("accept failed");
		exit(1);
	}
	
	char cmd[64];
	while(1){
		fflush(stdout);
		//fflush(socknew);
		//read(socknew, NULL, 1);
		memset(cmd, 0, sizeof(cmd));
		valread = 0;
		while(valread <= 0){
			valread = read(socknew, cmd, sizeof(cmd));
		}

		printf("recieved: %s\n", cmd);
		
		char filename[64] = {0};
		
		for(int i = 6; i<64; ++i){
			if(cmd[i] != '"'){
				filename[i-6] = cmd[i];
			}else{
				break;
			}
		}
		
		if(strncmp(cmd, "PULL", 4) == 0){ //pull (send file)
			printf("pulling %s\n", filename);
			
			struct stat f_stats;
			char path[64] = {0};
			strcat(path, "./");
			strcat(path, filename); //searching for file in root
			
			char sizeback[20] = {0};
			if(stat(path, &f_stats) == 0){ //if we find the file 
				size_t f_size = f_stats.st_size; //get size in bytes
				sprintf(sizeback, "%zu", f_size); //sprintf to temp string
				
				FILE* file_out = fopen(filename, "r"); //open file
				if(file_out == NULL){
					printf("error opening file");
					send(socknew, "DNE", sizeof("DNE"), 0); //dne when sent to client will indicate no file
					exit(1);
				}else{
					send(socknew, sizeback, sizeof(sizeback), 0); //send filesize to client
					char* f_buffer = (char*) malloc(f_size+1); //alloc buffer for file
					memset(f_buffer, 0, f_size+1); //nullify memory
					fread(f_buffer, f_size+1, 1, file_out); //read file to buffer
					send(socknew, f_buffer, f_size+1, 0); //send buffer
					send(socknew, "OK", sizeof("OK"), 0); //send ok
					free(f_buffer); //free heap
				}
				fclose(file_out); //close file
			}else{
				send(socknew, "DNE", sizeof("DNE"), 0); //did not find the file
			}
		}else{ //push (inbound)
			printf("pushing %s\n", filename);			
			
			const char s[2] = "\""; //token to separate string
			
			char* tok; //strtok
			tok = strtok(cmd, s);
			tok = strtok(NULL, s);
			tok = strtok(NULL, s);
			size_t f_size = atoi(tok);
			
			char* f_buffer = (char*) malloc(f_size+1);
			memset(f_buffer, 0, f_size+1);
			FILE* file_in = fopen(filename, "w");
			valread = 0;
			while(valread <= 0){
				valread = read(socknew, f_buffer, f_size+1);
			}
			
			fwrite(f_buffer, f_size, 1, file_in);
			fclose(file_in);
			free(f_buffer);
			send(socknew, "OK", sizeof("OK"), 0);
		}
		
		printf("done\n");
	}
	
	return 0;
}

