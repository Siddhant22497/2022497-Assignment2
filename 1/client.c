#include <stdio.h>
#include<string.h>
#include<arpa/inet.h>
#include <pthread.h>
#include<stdlib.h>
#include<unistd.h>


#define PORT 7000
#define BUFFER_SIZE 1024

void* client_thread(void* arg)
{ 
    int sock=0;
    struct sockaddr_in serv_addr;
    char* messagefromclient="Hello this is from client";
    char buffer[BUFFER_SIZE]={0};
    int client_id=*(int*)arg;

    if((sock=socket(AF_INET,SOCK_STREAM,0))<0)
    {
        printf("Client with %d id fails to socket creation\n",sock);
        return NULL;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET,"172.26.100.18",&serv_addr.sin_addr)<=0)
    {
        printf("Client %d not supported error\n",client_id);
        return NULL;
    }

    if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
    {
        printf("Client %d not failed to  connect\n",client_id);
        return NULL;   
    }
    
    send(sock,messagefromclient,strlen(messagefromclient),0);

    printf("Client %d message send to server\n",client_id);

    read(sock,buffer,BUFFER_SIZE);
    printf("Client %d Response form server: %s\n",client_id,buffer);

    close(sock);

    return NULL;


}
int main(int argc,char* argv[])
{
   if(argc!=2)
   {
        printf("Error\n");
        return 1;
   }

   int n=atoi(argv[1]);
   pthread_t thread[n];        
   int clientid[n];
   for(int i=0;i<n;++i)
   {
        clientid[i]=i+1;
        // putting each client into a new thread
        if(pthread_create(&thread[i],NULL,client_thread,(void*)&clientid[i])!=0)
        {
            perror("Failed to create thead\n");
            return 1;
        }
   }

   for(int i=0;i<n;++i)
   {
        pthread_join(thread[i],NULL);
   }
   return 0;

}