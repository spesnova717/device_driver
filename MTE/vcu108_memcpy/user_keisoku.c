#include <iostream>

#include <random>
#include <chrono>
#include <vector>

//Use Linux file functions for the device
#include <fcntl.h>
#include <unistd.h> // opening flags 
#include <sys/time.h>
#include <sys/types.h>

#define DATA_SIZE 4*1048576/4*2

using std::cout;
using std::endl;
using std::vector;

// test program for pci device
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define DEVFILE "/dev/fpga"
//#include <sys/resource.h>
#include <math.h>
#define DETAIL_LOG

double start_time_read;
double end_time_read;
double start_time_write;
double end_time_write;
double write_time;
double read_time;
int flag = 0;

struct IOCmd_t {
	uint32_t cmd; //command word
	uint8_t barNum; //which bar we are read/writing to
	uint32_t devAddr; // address relative to BAR that we are read/writing to
	void * userAddr; // virtual address in user space to read/write from
};

double gettimeofday_sec()
{
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec * 1e-6;
}

int open_device(const char* filename)
{
	int fd;
	//int FID = open("/dev/piecomm1", O_RDWR | O_NONBLOCK);
	fd = open(filename, O_RDWR | O_NONBLOCK);
	if(fd == -1){
		printf("can't open device: %s\n", filename);
		printf("mknod /dev/fpga c 246 0\n");
		
		exit(1);
	}

	printf("success: open device %s\n", filename);
	return fd;
}

void close_device(int fd)
{
	if(close(fd) != 0){
		printf("can't close device");
		exit(1);
	}
	
	if(flag == 1 || flag == 3)
	printf("success: close device\n");
}

//void read_device(int fd, char* buf, int size)

void mmio_test(int fd)
{
	int i,j;
    int loop;
	double temporary_write;
	double temporary_read;
	FILE *fp;
	unsigned int amount_data_byte;
	int *transfer_data;
	int data;
	unsigned int *receive_data;
	int write_data;
	
    amount_data_byte = 4100;
	
    //Create a random vector of half the length of the block RAM
    vector<uint32_t> testVec;
	  data = 0x11111112;
	
    for (size_t ct=0; ct<amount_data_byte; ct++){
    	testVec.push_back(data);
    }	
for(j=0;j<1;j++){	
	IOCmd_t iocmd = {0,0,0,0};
	iocmd.devAddr = 0;
	iocmd.userAddr = testVec.data();
  printf("test1 size = %d\n",testVec.size());
	start_time_write = gettimeofday_sec();
	write(fd, &iocmd, testVec.size());
	end_time_write = gettimeofday_sec();
	
	vector<uint32_t> testVec2;
    testVec2.resize(amount_data_byte);
    iocmd.userAddr = testVec2.data();	

	start_time_read = gettimeofday_sec();
	read(fd, &iocmd, testVec2.size());
	end_time_read = gettimeofday_sec();	
  printf("test2 size = %d\n",testVec2.size());
    for(i=0;i<amount_data_byte/4; i++){
		//printf("data = receive_data[%d] = %x\n",i,testVec2[i]);
    }
	
    temporary_write = amount_data_byte / (end_time_write - start_time_write) / 1000000;
    temporary_read = amount_data_byte  / (end_time_read - start_time_read) / 1000000;
    
    printf("Write = %8lf[MB/s]\n",temporary_write);
    printf("Read  =  %8lf[MB/s]\n",temporary_read);
}
}

int main(int argc, char const* argv[])
{
	int fd;

	fd = open_device(DEVFILE);
	mmio_test(fd);
	close_device(fd);	
	
	return 0;
}
