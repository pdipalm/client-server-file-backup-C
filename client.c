//icsi333
//remote backup 1, client.
//peter dipalma
//partner: proshanto dabnath
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

int main(int argc, char *argv[]){
	if((argc!=3)){
        printf("improper arguments, requires: IPv4 PORT");
		exit(1);
    }
    char ipv4[16] = {0};
    strcpy(ipv4, argv[1]);
    unsigned short port = atoi(argv[2]); //argv is stored as string we need unsigned short

	printf("client mode...\n");

	int sock = 0, valread;
	struct sockaddr_in server_addr;
	
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("client socket error");
        exit(1);
    }
	
	//preparing address for socket
	server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(ipv4);
	
	int status = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)); //connecting socket to address
	if(status < 0){
        printf("client connection failed, status: %d", status);
        exit(1);
    }
	char cmd[64]; //raw command from user
	char filename[64]; 
	char cmd_tosocket[64]; //formatted command to send
	while(1){
		//nullifying old memory from last loop
		memset(cmd, 0, sizeof(cmd));
		memset(filename, 0, sizeof(filename));
		memset(cmd_tosocket, 0, sizeof(cmd_tosocket));
		
		printf(">> ");
		fgets(cmd, 64, stdin);
		
		if(strncmp(cmd, "QUIT", 4)==0){
			close(sock);
			exit(0);
		}else{
			strncpy(filename, cmd+5, 59); 
			filename[strcspn(filename, "\n")] = 0; //fetch file name and remove \n from user pressing enter
			
			strncpy(cmd_tosocket, cmd, 4); //all commands are 4 letters, so we can copy the first 4 to the formatted command
			strcat(cmd_tosocket, " \""); 
			strcat(cmd_tosocket, filename); //inserting file name between quotes to formatted command
			strcat(cmd_tosocket, "\"");
			
			if(strncmp(cmd, "PULL", 4)==0){ //pull file from server
				printf("pulling %s\n", filename);
				
				send(sock, cmd_tosocket, sizeof(cmd_tosocket), 0); //formatted command is done so send it
				
				valread = 0;
				char size_back[20] = {0};
				while(valread <= 0){
					valread = read(sock, size_back, sizeof(size_back)); //get filesize from server as char*
				}
				
				
				
				if(strcmp(size_back, "DNE") == 0){
					printf("no such file exists");
					exit(1);
				}
				
				size_t sb_as_num = atoi(size_back); //convert char* from server to size_t

					
				char* file_buffer = (char*) malloc(sb_as_num+1); //allocate buffer for file coming in
				memset(file_buffer, 0, sb_as_num+1);
				FILE* file_in = fopen(filename, "w"); //create inbound file
				valread = 0;
				while(valread <= 0){ //wait to get the data
					valread = read(sock, file_buffer, sb_as_num+1); //read socket and copy to buffer
				}
				
				
				fwrite(file_buffer, sb_as_num+1, 1, file_in); //write buffer to file 
				
				fclose(file_in); 
				free(file_buffer); //garbage collection
				int ok = 0;
				char temp[3];
				while(ok <= 0){ //get ok from server
					//printf("waiting for ok");
					ok = read(sock, temp, sizeof(temp));
				}
				printf("%s pulled\n", filename);
			}else if(strncmp(cmd, "PUSH", 4)==0){ //push file to server 
				printf("pushing %s\n", filename);
				FILE* file_out = fopen(filename, "r"); //open file for reading
				if(file_out == NULL){
					printf("file does not exist");
					exit(1);
				}
				struct stat f_stats;
				char path[64] = {0};
				strcat(path, "./");
				strcat(path, filename); //searching for file in root
				
				if(stat(path, &f_stats) == 0){ //if we find the file 
					size_t f_size = f_stats.st_size; //get size in bytes
					char tempstring[20] = {0};
					sprintf(tempstring, " %zu", f_size); //sprintf to temp string
					strcat(cmd_tosocket, tempstring); //append to formatted command
					
					send(sock, cmd_tosocket, sizeof(cmd_tosocket), 0); //send formatted command
					
					
					
					char* f_buffer = (char*) malloc(f_size+1); //allocate file buffer
					memset(f_buffer, 0, f_size+1);
					fread(f_buffer, f_size, 1, file_out); //copy file to buffer
					fclose(file_out); //close file
					
					send(sock, f_buffer, f_size, 0); //sending file buffer to server
					
					free(f_buffer); //free heap mem
					
					char temp[3];
					int ok = 0;
					while(ok <= 0){ //get ok from server
						ok = read(sock, temp, sizeof(temp));
					}
					printf("%s pushed\n", filename);
					
				}else{
					printf("could not retrieve file");
					exit(1);
				}
			}else{
				printf("command not recognized, exiting...");
				exit(1);
			}
		}
	}
	return 0;
}