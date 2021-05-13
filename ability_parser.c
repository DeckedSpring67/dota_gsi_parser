#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

int tmp;

struct header{
char * n;
char * v;
}h[100];

int main(){
	struct sockaddr_in addr,remote_addr;
	int n_abilities = 0;
	int last_n = -1;
	int i,j,k,s,t,s2,len;
	int yes=1;
	int entity_length;
	FILE *f;
	char *entity, *statusline, *tempptr, *strability;
	char request[5000],response[10000],temp[1000];
	s =  socket(AF_INET, SOCK_STREAM, 0);
	if ( s == -1 ){
		perror("Socket fallita"); 
		return 1;
	}
	addr.sin_family = AF_INET;
	//Server Port
	addr.sin_port = htons(3000);
	addr.sin_addr.s_addr = 0;
	t= setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
	if (t==-1){
		perror("setsockopt failed");
		return 1;
	}
	if ( bind(s,(struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
		perror("bind failed");
		return 1;
	}
	if ( listen(s,5) == -1 ) {
		perror("Listen failed");
		return 1;
	}
	len = sizeof(struct sockaddr_in);
	while(1){
		printf("Waiting...\n");
		s2 =  accept(s, (struct sockaddr *)&remote_addr,&len);
		if ( s2 == -1 ) {
			perror("Accept failed");
			return 1;
		}
		//Reset header
                bzero(h,sizeof(struct header)*100);

		statusline = h[0].n=response;
		//Get the 8 headers
                for( j=0,k=0; read(s2,response+j,1);j++){
                                if(response[j]==':' && (h[k].v==0) ){
                                        response[j]=0;
                                        h[k].v=response+j+2;
                                }
                                else if((response[j]=='\n') && (response[j-1]=='\r') ){
					//Check if we're done with the headers, in case exit the cycle
					if(response[j-2] == '\n')
						break;
                                        response[j-1]=0;
                                        h[++k].n=response+j+1;
                                }
                }

		//Get entity length
		entity_length = -1;
                for(i=1;i<k;i++){
                        if(strcmp(h[i].n,"Content-Length")==0){
                                entity_length=atoi(h[i].v);
//                                printf("* (%d) ",entity_length);
                        }
//                        printf("%s ----> %s\n",h[i].n, h[i].v);
                }
		//Fill the char array containing the JSON
		entity = (char*) malloc(entity_length);
		i = 0;
		k = 0;
		while(i < entity_length){
			t = read(s2,entity+k,(entity_length-i));
			i+=t;
			k+=t;
		}
//		printf("%s\n",entity);
		tempptr = entity;
		//Reset n_abilities for when we get packets with only the game 
		n_abilities = 0;
		for(i = 0; i < entity_length; i++){
			if(entity[i] == '\n'){
				//JSON doesn't have carriage return for some reason, fucking dogs
				entity[i] = 0;
				strcpy(temp,tempptr);
				//DEBUG: shows the line we're parsing
				//printf("%s\n",temp);
				tempptr = entity+(i+1);
				if(((strability = strstr(temp,"ability")) != NULL) && ((strstr(temp, "active") == NULL ))){
					//printf("%s\n",strability);
					//Get the ability number (when we exit the cycle it will always be the last one)
					//+1 because, you know, that's the count not the position
					sscanf(strability,"ability%d",&n_abilities);
					n_abilities++;
				}
				//If we parse a "player1" it means we're on a replay or something of sorts so we can't really tell which player we're looking at, set the number of abilities to 0 instead and break the loop
				if(strstr(temp,"player1") != NULL){
					n_abilities = 0;
					break;
				}
				//Ignore previously scenario (always after abilities)
				if(strstr(temp,"previously") != NULL){
					break;
				}
			}
		}
		//Open file
		if(last_n != n_abilities){
			last_n = n_abilities;
			//Check if file exists
			sprintf(temp,"mask%d.png",n_abilities);
			if(f = fopen(temp,"r")){
				//Close the file we don't need it
				fclose(f);
				//Copy the file
				sprintf(temp,"cp -f mask%d.png mask.png",n_abilities);
				if(system(temp) < 0){
					perror("copy failed");
					return 1;
				}
			}
		}
		printf("Number of abilities: %d\n",n_abilities);
		//Write success response
                sprintf(response,"HTTP/1.1 200 OK\r\n\r\n");
                write(s2,response,strlen(response));
		close(s2);
		
		free(entity);	
	}
}
	 
