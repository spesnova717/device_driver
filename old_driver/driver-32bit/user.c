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

double start_time_read;
double end_time_read;
double start_time_write;
double end_time_write;
double write_time;
double read_time;

double gettimeofday_sec()
{
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec * 1e-6;
}

/*double getrusage_sec()
{
    struct rusage t;
    struct timeval tv;

    getrusage(RUSAGE_SELF,&t);
    tv = t.ru_utime;

    return tv.tv_sec + (double)tv.tv_usec*1e-6;
}*/

int open_device(const char* filename)
{
	int fd;

	fd = open(filename, O_RDWR);
	if(fd == -1){
		printf("can't open device: %s\n", filename);
		printf("mknod /dev/fpga c 246 0\n");
		
		exit(1);
	}

	////printf("success: open device %s\n", filename);
	return fd;
}

void close_device(int fd)
{
	if(close(fd) != 0){
		printf("can't close device");
		exit(1);
	}

	////printf("success: close device\n");
}

//void read_device(int fd, char* buf, int size)
void read_device(int fd, unsigned int* buf, int size) {
	int i=0, j=0, ret; // ssize_t
	FILE *fppap;
	ret = read(fd, buf, size);
}

void write_device(int fd, void *buf, unsigned int size) {
	int ret;
	ret = write(fd, buf, size);
	if(ret < 0) {
		printf("write error");
	}
}

//#define MMM 262144
#define MMM 16384*8

unsigned int buf[MMM];

void mmio_test(int fd)
{
	int i;

	for (i=0; i<MMM; i++){
		buf[i]=0xAAAAAAAC;
	}
	
	//printf("======Write !=======\n");
	lseek(fd,       0, SEEK_SET);
	start_time_write = gettimeofday_sec();
	for (i=0; i<MMM; i++){
		write_device(fd, buf+i, 1);	
	}
	fdatasync(fd);
	end_time_write = gettimeofday_sec();

	//printf("======Read !=======\n");
	lseek(fd,       0, SEEK_SET);
	start_time_read = gettimeofday_sec();	
	for (i=0; i<MMM; i++){
		read_device(fd, buf+i, 1);
	}

	//for (i=0; i<MMM; i++){
	//printf("buf[%d] = %X\n",i,buf[i]);
	//}
	end_time_read = gettimeofday_sec();

	read_time = MMM*4/(end_time_read - start_time_read)/1000000;
	write_time = MMM*4/(end_time_write - start_time_write)/1000000;
	printf("write = %lf\n",write_time);
	printf("read = %lf\n",read_time);
	//printf("read = %lf\n",end_time_read - start_time_read);

}

int main(int argc, char const* argv[])
{
	int fd;

	fd = open_device(DEVFILE);

	mmio_test(fd);

	close_device(fd);
	/*
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
	printf("%lf %lf\n",read_bps,write_bps);
	*/
	return 0;
}
