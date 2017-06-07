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
//#define MMIO_DATASIZE 1048576/4
#define MMIO_DATASIZE 262140
double start_time_read;
double end_time_read;
double start_time_write;
double end_time_write;

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

void read_device(int fd, char* buf, int size)
//void read_device(int fd, int* buf, int size)
{
	int i=0, ret; // ssize_t
	start_time_read = gettimeofday_sec();
	ret = read(fd, buf, size);
	////printf("ret=%d,fd=%d, buf=%d, size=%d\n",ret,fd, buf, size);
	//printf("%2d ", buf[1048576]);
	end_time_read = gettimeofday_sec();
	if(ret > 0) {
		////printf("read device : %d bytes read\n", ret);
		//while(ret--) printf("%2d ", buf[i++]);
	} else {
		printf("read error");
	}

	////printf("\n");
}

void write_device(int fd, void *buf, unsigned int size)
//void write_device(int fd, void *buf, unsigned int size)
{
	int ret;
	start_time_write = gettimeofday_sec();
	ret = write(fd, buf, size);
	end_time_write = gettimeofday_sec();
	if(ret < 0) {
		printf("write error");
	}
	////printf("write device : %d bytes write\n", ret);
}

void mmio_test(int fd)
{
	char buf[MMIO_DATASIZE];
	//int buf[MMIO_DATASIZE];
	int i;

	////printf("\n---- start mmio test ----\n");
	for (i = 0; i < MMIO_DATASIZE; i++) {
		buf[i] = 0;
	}

	read_device(fd, buf, MMIO_DATASIZE);
	lseek(fd,	0, SEEK_SET);

	for (i = 0; i < MMIO_DATASIZE; i++) {
		buf[i] = 17;
	}
	
	write_device(fd, buf, sizeof(buf));
	lseek(fd,	0, SEEK_SET);

	for (i = 0; i < MMIO_DATASIZE; i++) {
		buf[i] = 0;
	}
	read_device(fd, buf, MMIO_DATASIZE);

	////printf("\n---- end mmio test ----\n");
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

	read_bps = (262140*4*8/read/1000000);
	write_bps = (262140*4*8/write/1000000);
	//printf("read time = %lf[sec]\n",read);
	//printf("write time = %lf[sec]\n",write);
	//printf("read = %lf[Mbps]\n",read_bps);
	//printf("write = %lf[Mbps]\n",write_bps);	
	printf("%lf %lf\n",read_bps,write_bps);
	return 0;
}
