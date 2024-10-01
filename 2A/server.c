#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <ctype.h>

#define PORT 7000
#define BUFFER_SIZE 1024
#define MAX_PROCESSES 1024
#define STAT_PATH_MAX 300 

struct ProcessInfo {
    int pid;
    char name[256];
    long long user_time;
    long long kernel_time;
    long long total_time;
};

void get_top_cpu_processes(struct ProcessInfo top_processes[2]) {
    DIR *proc_dir = opendir("/proc");
    struct dirent *entry;
    struct ProcessInfo processes[MAX_PROCESSES];
    int process_count = 0;

    if (proc_dir == NULL) {
        perror("Failed to open /proc directory");
        return;
    }

    while ((entry = readdir(proc_dir)) != NULL) {
       
        if (isdigit(entry->d_name[0])) {
            char stat_path[512];
            int ret = snprintf(stat_path, sizeof(stat_path), "/proc/%s/stat", entry->d_name);
            if (ret >= sizeof(stat_path)) {
      
                fprintf(stderr, "Warning: Truncated path for process stat file.\n");
            }

            FILE *stat_file = fopen(stat_path, "r");
            if (stat_file != NULL) {
                struct ProcessInfo p;
                p.pid = atoi(entry->d_name);
                fscanf(stat_file, "%*d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lld %lld",
                       p.name, &p.user_time, &p.kernel_time);
                p.total_time = p.user_time + p.kernel_time;
                fclose(stat_file);

                processes[process_count++] = p;
            }
        }
    }

    for (int i = 0; i < process_count - 1; i++) {
        for (int j = 0; j < process_count - i - 1; j++) {
            if (processes[j].total_time < processes[j + 1].total_time) {
                struct ProcessInfo temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }


    top_processes[0] = processes[0];
    top_processes[1] = processes[1];

    closedir(proc_dir);
}

void* handle_client(void* soc_des) {
    int client_socket = *(int*)soc_des;
    char buffer[BUFFER_SIZE] = {0};
    struct ProcessInfo top_processes[2];

  
    get_top_cpu_processes(top_processes);

    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "Top 2 CPU-consuming processes:\n"
             "1. PID: %d, Name: %s, User CPU Time: %lld, Kernel CPU Time: %lld, Total CPU Time: %lld\n"
             "2. PID: %d, Name: %s, User CPU Time: %lld, Kernel CPU Time: %lld, Total CPU Time: %lld\n",
             top_processes[0].pid, top_processes[0].name, top_processes[0].user_time,top_processes[0].kernel_time,top_processes[0].total_time,
             top_processes[1].pid, top_processes[1].name, top_processes[1].user_time,top_processes[1].kernel_time,top_processes[1].total_time);

    send(client_socket, response, strlen(response), 0);
    printf("Response sent to client\n");

    close(client_socket);
    free(soc_des);
    return NULL;
}

int main() {
    struct sockaddr_in address;
    pthread_t thread_id;
    int backlog = 10;
    int server_fd;


    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket initialization failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);


    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }


    if (listen(server_fd, backlog) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);
    int addrlen = sizeof(address);
    int new_socket;
    int* new_sock;
    while ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) >= 0) {
        printf("Connection accepted\n");
        new_sock = malloc(sizeof(int));
        *new_sock = new_socket;

        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
            return 1;
        }

        pthread_detach(thread_id);
    }

    if (new_socket < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    close(server_fd);
    return 0;
}
