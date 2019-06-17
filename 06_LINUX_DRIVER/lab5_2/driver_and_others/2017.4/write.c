#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>


int main(void){
    int IP_fd;
    int i;
    //unsigned int data = "GSLAB";
    unsigned long data = 0xaa;
    IP_fd = open("/dev/IP-Driver",O_RDWR);
    if(IP_fd == -1){
        perror("open device IP-Driver error!!\n");
        exit(1);
    }


        if(write(IP_fd,&data, sizeof(int))){
        	perror("write() error!\n");
        close(IP_fd);
	exit(1);
	}


/*
    if(write(IP_fd,&data, sizeof(int))){
        perror("write() error!\n");
        close(IP_fd);
        exit(1);
    }
*/
    close(IP_fd);

    return 0;
}
