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
double gettimeofday_sec()
{
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec * 1e-6;
}

int open_device(const char* filename)
{
	int fd;

	fd = open(filename, O_RDWR);
	if(fd == -1){
		printf("can't open device: %s\n", filename);
		printf("mknod /dev/fpga c 246 0\n");
		
		exit(1);
	}

	if(flag == 1 || flag == 3)
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
void read_device(int fd, unsigned int* buf, int size) {
	int i=0, j=0, ret; // ssize_t
	//FILE *fppap;
	ret = read(fd, buf, size);
	if(flag == 2)
	fdatasync(fd);
	//if(flag == 1 || flag == 3) printf("read %d [byte]\n",ret);
}

void write_device(int fd, void *buf, unsigned int size) {
	int ret;
	ret = write(fd, buf, size);
	if(flag == 1 || flag == 2) fdatasync(fd);
	if(ret < 0) printf("write error");
	//if(flag == 1 || flag == 3) printf("write %d [byte]\n",ret);
}

void mmio_test(int fd)
{
	int i;
  int loop;
  double temporary_write;
  double temporary_read;
	FILE *fp;
	unsigned int amount_data;
	int *transfer_data;
	unsigned int *receive_data;
	int write_data;
	printf("flag = 0 : Simple Mode(Default)\n");
	printf("flag = 1 : Debug Mode\n");
	printf("flag = 2 : Write Guarantee Mode\n");
	printf("flag = 3 : Print Value\n");
	printf("flag = 4 : File print Value\n");
  printf("flag = 5 : Random Write Mode\n");
  printf("flag = 6 : Read Mode\n");
	printf("flag = ");
	///scanf("%d", &flag);
  flag = 5;
  if(flag == 5){
    for(loop=0;loop<500;loop++){
    amount_data = 262144;
    transfer_data = (int *)malloc(sizeof(int) * amount_data);
    receive_data = (unsigned int *)malloc(sizeof(unsigned int) * amount_data);
    for (i=0; i<amount_data; i++){
      transfer_data[i]=0x50000000 + i;
    }
    lseek(fd,0,SEEK_SET);
    start_time_write = gettimeofday_sec();
    //for (i=0; i<amount_data; i++){
      //start_time_write = gettimeofday_sec();
      write(fd, transfer_data, amount_data*4);
      //write(fd, transfer_data+i, 4);
      //end_time_write = gettimeofday_sec();
    //}
    end_time_write = gettimeofday_sec();
    printf("---Write Finish---\n");

    start_time_read = gettimeofday_sec();
     read(fd, receive_data, amount_data*4);
    end_time_read = gettimeofday_sec();
    printf("---Read Finish---\n");
      for(i=0;i<amount_data; i++){
        //printf("data = receive_data[%d] = %x\n",i,receive_data[i]);
      }
    temporary_write = amount_data * 4 / (end_time_write - start_time_write) / 1000000;
    temporary_read = amount_data * 4 / (end_time_read - start_time_read) / 1000000;
    
    printf("Write = %8lf[MB/s]\n",temporary_write);
    printf("Read  =  %8lf[MB/s]\n",temporary_read);
    }
    return 0;
  }

	printf("---Amount of Data---\n");
	printf("---1[MByte] = 262144---\n");
	printf("---2[MByte] = 524288---\n");
	printf("Amount of Data = ");
	scanf("%d", &amount_data);
	printf("Write %d [byte]\n",amount_data*4);
	printf("Write Data = 0x");
	scanf("%x",&write_data);
	transfer_data = (int *)malloc(sizeof(int) * amount_data);
	receive_data = (unsigned int *)malloc(sizeof(unsigned int) * amount_data);
		
	for (i=0; i<amount_data; i++){
		transfer_data[i]=write_data;
	}
	
	if(flag == 1 || flag == 3)
	printf("======Write !=======\n");
	lseek(fd,0,SEEK_SET);
	
	start_time_write = gettimeofday_sec();
	for (i=0; i<amount_data; i++){
		write_device(fd, transfer_data+i, 4);
	}
	end_time_write = gettimeofday_sec();

	if(flag == 1 || flag == 3)
	printf("======Read !=======\n");
    	lseek(fd,0,SEEK_SET);
	start_time_read = gettimeofday_sec();	
	for (i=0; i<amount_data; i++){
		read_device(fd, receive_data+i, 4);
	}
	end_time_read = gettimeofday_sec();

	if(flag == 1 || flag == 3){
		for (i=0; i<amount_data; i++){
			printf("receive_data[%d] = %X\n",i,receive_data[i]);
			}
	}

	read_time = amount_data*4/(end_time_read - start_time_read)/1000000;
	write_time = amount_data*4/(end_time_write - start_time_write)/1000000;
	printf("write = %lf\n",write_time);
	printf("read = %lf\n",read_time);
	//printf("read = %lf\n",end_time_read - start_time_read);
	
	if(flag == 4){
		fp = fopen("data.txt","wb");
		for(i=0;i<amount_data;i++){
			fprintf(fp,"receive_data[%d] = %x\n",i,receive_data[i]);
		}
		fclose(fp);
	}

}

int main(int argc, char const* argv[])
{
	int fd;

	fd = open_device(DEVFILE);
	mmio_test(fd);

	close_device(fd);
	
	double read;
	double write;
	double read_bps;
	double write_bps;
	read = (end_time_read - start_time_read);
	write = (end_time_write - start_time_write);

	read_bps = (1000000*8/read/1000000);
	write_bps = (1000000*8/write/1000000);
	//printf("read time = %lf[sec]\n",read);
	//printf("write time = %lf[sec]\n",write);
	//printf("read = %lf[Mbps]\n",read_bps);
	//printf("write = %lf[Mbps]\n",write_bps);	
	//printf("%lf %lf\n",read_bps,write_bps);
	
	return 0;
}
