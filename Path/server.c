#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<unistd.h>

#define PORT 5001
#define MAX_CLIENTS 50
#define SIZE 1024
#define NAME_LEN 32

static int client_count=0;
static int uid=10;

typedef struct{
	struct sockaddr_in addr;
	int sockfd;
	int uid;
	char name[NAME_LEN];
}client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite(){
	printf("\r%s","> ");
	fflush(stdout);
}

void str_trim(char* arr,int length){
	for(int i=0;i<length;i++){
		if(arr[i]=='\n'){
			arr[i]='\0';
			break;
		}
	}
}

void add_client(client_t *cl){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0;i<MAX_CLIENTS;i++){
		if(!clients[i]){
			clients[i]=cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int uid){
	 pthread_mutex_lock(&clients_mutex);

	 for(int i=0;i<MAX_CLIENTS;i++){
		 if(clients[i]){
			 if(clients[i]->uid==uid){
				 clients[i]=NULL;
				 break;
			 }
		 }
	 }

	  pthread_mutex_unlock(&clients_mutex);
}

// sending message to all the other clients
void send_msg(char *s,int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			if(clients[i]->uid!=uid){

				if(send(clients[i]->sockfd,s,strlen(s),0)<0){
						printf("ERROR: write to des\n");
						break;
			        }
			}
		}
	}

	 pthread_mutex_unlock(&clients_mutex);
}

void one_to_one(int sockfd1,int sockfd2){
        pthread_mutex_lock(&clients_mutex);

	char buff1[500];
	read(sockfd1,buff1,500);
	str_trim(buff1,500);
	printf("alpha\n");
	send(sockfd2,buff1,sizeof(buff1),0);

	pthread_mutex_unlock(&clients_mutex);
}


void *handle_client(void *arg){
     	char buff[SIZE];
	char name[NAME_LEN];
	int leave_flag=0;
	int flag=0;
	client_count++;

	client_t *cli = (client_t*)arg;
	if(recv(cli->sockfd,name,NAME_LEN,0)<=0){
		printf("enter the name correctly\n");
		leave_flag=1;
	}
	else{
		strcpy(cli->name,name);
		sprintf(buff,"%s has joined\n",cli->name);
		printf("%s",buff);
	//	send_msg(buff,cli->uid);

	}
	bzero(buff,SIZE);

	while(1){
		if(leave_flag==1)
			break;
		int receive = recv(cli->sockfd,buff,SIZE,0);
		if(receive==0||strcmp(buff,"exit")==0){
			sprintf(buff,"%s has left",cli->name);
			printf("%s\n",buff);
			send_msg(buff,cli->uid);
			leave_flag=1;
		}
		else if(receive>0){

				if(strcmp(buff,"1")==0)
				{
					printf("inside\n");
			//		send(cli->sockfd,"enter the msg",sizeof("enter the msg"),0);
					bzero(buff,SIZE);
				int temp=read(cli->sockfd,buff,SIZE);
					printf("%d\n",temp);
					send_msg(buff,cli->uid);

				}
				else{
					read(cli->sockfd,buff,SIZE);     //name reading
					str_trim(buff,SIZE);
					for(int i=0;i<MAX_CLIENTS;i++){
						if(clients[i]){
							if(strcmp(clients[i]->name,buff)==0){
								send(cli->sockfd,"client prresent enter msg",sizeof("client present enter msg"),0);
								one_to_one(cli->sockfd,clients[i]->sockfd);
								flag=1;
								printf("#\n");
								break;
							}
						}
					}
					if(flag==0)
						send(cli->sockfd,"not there",sizeof("not there"),0);
				}
			//	str_trim(buff,strlen(buff));


		}
		else{
			printf("error\n");
			leave_flag=1;
		}
		bzero(buff,SIZE);
	}
	remove_client(cli->uid);
	client_count--;
	close(cli->sockfd);
	free(cli);
	pthread_detach(pthread_self());
	return NULL;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(){
	struct sockaddr_in newaddr;
	socklen_t addr_size;
	 int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    //printf("server: waiting for connections...\n");
	/*char buff[SIZE];

	sockfd = socket(PF_INET,SOCK_STREAM,0);
	if(sockfd>=0)
		printf("Socket created successfully\n");
	memset(&server,'\0',sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = INADDR_ANY;
        int bind_des;
	bind_des=bind(sockfd,(struct sockaddr*)&server,sizeof(server));
	if(bind_des==0)
		printf("Binded successfully\n");
	int listen_desc;
	pthread_t tid;
	listen_desc=listen(sockfd,5);
	if(listen_desc==0)*/
		printf("===WELCOME TO THE GROUP===\n");
	while(1){

  	     addr_size = sizeof(newaddr);
	     newfd = accept(sockfd,(struct sockaddr*)&newaddr,&addr_size);
	     if(client_count+1==MAX_CLIENTS){
		     printf("Limit reached. Closing this connection\n");
		     close(newfd);
		     continue;
	     }
             // create a client variable
	     client_t *cl = (client_t*)malloc(sizeof(client_t));
	     cl->addr = newaddr;
	     cl->sockfd = newfd;
	     cl->uid = uid++;

	     // add client to queue
	     add_client(cl);
             pthread_create(&tid,NULL,&handle_client,(void*)cl);

	     sleep(1);



	}

	return 0;
}
