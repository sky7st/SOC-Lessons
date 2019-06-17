#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

int main(void){
    int IP_fd;
    unsigned int data;

    IP_fd = open("/dev/IP-Driver",O_RDWR);
    if(IP_fd == -1){
        perror("open device IP-Driver error!!\n");
        exit(1);
    }
    if(read(IP_fd,&data, sizeof(int))){
    perror("read() error!\n");
    close(IP_fd);
    exit(1);
    }

    printf("Application read>%d",data);
    close(IP_fd);

    return 0;
}
