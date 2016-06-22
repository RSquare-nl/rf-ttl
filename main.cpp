#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>

int set_interface_attribs (int fd, int speed, int parity){
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0){
		printf ("error %d from tcgetattr", errno);
		return -1;
	}

	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
	// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 0;            // read doesn't block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0) {
		printf ("error %d from tcsetattr", errno);
		return -1;
	}
	return 0;
}

void set_blocking (int fd, int should_block) {
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0) {
		printf ("error %d from tggetattr", errno);
		return;
	}

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
		printf ("error %d setting term attributes", errno);
}

/** abort
 * Shows the usage and options of this program
 * @param
 **/
int abort(char **argv){ 
	fprintf(stderr,"Use: %s -<option> \n",argv[0]);
	fprintf(stderr," bv: %s -d /dev/ttyUSB0\n",argv[0]);
	fprintf(stderr,"\n");
	fprintf(stderr," Options:\n");
	fprintf(stderr,"   -d   serial port\n");
	fprintf(stderr,"   -o   Write permanent\n");
	fprintf(stderr,"   -f   <0-1> FEC\n");
	fprintf(stderr,"   -w   Waiting time\n");
	fprintf(stderr,"   -c <channel>  Communication channel 0-31\n");
	fprintf(stderr,"   -p   <0-3> Output power 0-3 (0=30dBm - 3=21dBm)\n");
	fprintf(stderr,"   -a   <0-7> Air speed 0=1Kbps(default)\n\t1=2Kbps\n\t2=5Kbps\n\t3=8Kbps\n\t4=10Kbps\n\t5=15Kbps\n\t6=20Kbps\n\t7=25Kbps\n");
	fprintf(stderr,"   -s   <0-7> uart speed\n\t0=1200bps\n\t1=2400bps\n\t2=4800bps\n\t3=9600bps\n\t4=19200bps\n\t5=38400bps\n\t6=57600bps\n\t7=115200bps\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Serial port programmer for the e32-ttl-100 and the e36-ttl-100\n");
	fprintf(stderr,"\n");
	return(1);
}

/** sendCommand
 * Sends a command thats in the global buffer with the global count 
 * The return is read and placed in the same buffer/count
 * @param int fd must be an open filepointer to a serial device
 **/
void sendCommand(int fd,char *buffer,int *count){
	int n;
	n=write (fd, buffer, *count);
	memset(buffer,0,sizeof(buffer));
	*count=0;
	usleep (1000000);  
	*count = read (fd, buffer, sizeof(buffer)); 
}


/** getVersion
 * Asks for the version of the device and reads the return result
 * The result is placed in the global buffer/count 
 * @param int fd must be an open filepointer to a serial device
 **/
void getVersion(int fd,char *buffer,int *count){
		buffer[0]=0xC3;
		buffer[1]=0xC3;
		buffer[2]=0xC3; //Module returns the present configuration parameters.
		buffer[3]=0x00;
		*count=3;
		sendCommand(fd,buffer,count);
}

/** getConfig
 * Asks for the config of the device and reads the return result
 * The result is placed in the global buffer/count 
 * @param int fd must be an open filepointer to a serial device
 **/
void getConfig(int fd,char *buffer,int *count){
		buffer[0]=0xC1;
		buffer[1]=0xC1;
		buffer[2]=0xC1; //Module returns the present configuration parameters.
		buffer[3]=0x00;
		*count=3;
		sendCommand(fd,buffer,count);
}

/* detectBaudrate
 * Try to find the correct boudrate
 * @int fd serial port
 * @return int -1=Not found; 0-7=The found speed
 */
int detectBaudrate(int fd,char *buffer,int *count){
	int speed[8];
	int i;

	speed[0]=B1200;
	speed[1]=B2400;
	speed[2]=B4800;
	speed[3]=B9600;
	speed[4]=B19200;
	speed[5]=B38400;
	speed[6]=B57600;
	speed[7]=B115200;

	for(i=0;i<8;i++){
		set_interface_attribs (fd, speed[i], 0);  // set speed to 115,200 bps, 8n1 (no parity)
		getVersion(fd,buffer,count);
		if((buffer[0] & 0xff) ==0xC3){//speed found
			return i;
		}
	}
	return -1;
}


void printConfig(char *buffer){
	printf("\nConfig:\n");
	printf("\nAddress:\t\thigh=0x%X\n\t\t\tLow=0x%X  ",buffer[1],buffer[2]);
	printf("\nUART mode: ");
	switch(buffer[3] & 0xC0){
		case 0x00:
			printf("\t\t8N1");
			break;
		case 0x40:
			printf("\\tt8O1");
			break;
		case 0x80:
			printf("\t\t8E1");
			break;
		case 0xC0:
			printf("\t\t8N1");
			break;
	}
	printf("\nUART baud rate: ");
	switch(buffer[3] & 0x38){
		case 0x00:
			printf("\t1200bps");
			break;
		case 0x08:
			printf("\t2400bps");
			break;
		case 0x10:
			printf("\t4800bps");
			break;
		case 0x18:
			printf("\t9600bps");
			break;
		case 0x20:
			printf("\t19200bps");
			break;
		case 0x28:
			printf("\t38400bps");
			break;
		case 0x30:
			printf("\t57600bps");
			break;
		case 0x38:
			printf("\t115200bps");
			break;
	}
	printf("\nAir date rate: ");
	switch(buffer[3] & 0x07){
		case 0x00:
			printf("\t\t1kbps");
			break;
		case 0x01:
			printf("\t\t2kbps");
			break;
		case 0x02:
			printf("\t\t5kbps");
			break;
		case 0x03:
			printf("\t\t8kbps");
			break;
		case 0x04:
			printf("\t\t10kbps");
			break;
		case 0x05:
			printf("\t\t15kbps");
			break;
		case 0x06:
			printf("\t\t20kbps");
			break;
		case 0x07:
			printf("\t\t25kbps");
			break;
	}
	printf("\nChannel: \t\t0x%X (%d)",buffer[4] & 0x1F,buffer[4] & 0x1F);
	printf("\nTransmission mode: ");
	if(buffer[5] & 0x80)
		printf("\tFixed transmission mode");
	else
		printf("\tTransparent transmission mode");
	printf("\nIO drive mode: ");
	if(buffer[5] & 0x40)
		printf("\t\tTXD and AUX push-pull outputs, RXD pull-up inputs");
	else
		printf("\t\tTXD、AUX open-collector outputs, RXD open-collector inputs");
	printf("\nWireless wake-up time:");
	switch(buffer[5] & 0x38){
		case 0x00:
			printf("\t250ms");
			break;
		case 0x01:
			printf("\t500ms");
			break;
		case 0x02:
			printf("\t750ms");
			break;
		case 0x03:
			printf("\t1000ms");
			break;
		case 0x04:
			printf("\t1250ms");
			break;
		case 0x05:
			printf("\t1500ms");
			break;
		case 0x06:
			printf("\t1750ms");
			break;
		case 0x07:
			printf("\t2000ms");
			break;
	}
	printf("\nFEC switch: ");
	if(buffer[5] & 0x04)
		printf("\t\tFEC ON");
	else
		printf("\t\tFEC OFF");
	printf("\nTransmission power:");
	switch(buffer[5] & 0x03){
		case 0x00:
			printf("\t30dbm");
			break;
		case 0x01:
			printf("\t27dbm");
			break;
		case 0x02:
			printf("\t24dbm");
			break;
		case 0x03:
			printf("\t21dbm");
			break;
	}
}




int main(int argc, char **argv){
	int pid=0;
	char port[20];
	int fd;
	int i,baudrate;
	int timeout=0;
	int ret=0;
	char *result_filename;
	FILE * pFile;
	int txt=0;

	char *cvalue = NULL;
	int index;
	int c;
	int version=0;
	int channel=-1;
	int power=-1;
	int airspeed=-1;
	int uartspeed=-1;
	int fec=-1;
	char buffer [100];
	int count;
	char mask;
	char header=0xC2 & 0xFF;

	memset(port,0,sizeof(port));

	opterr = 0;
	while ((c = getopt (argc, argv, ":hof:wc:p:a:s:d:")) != -1){
		switch (c)
		{
			case 'f'://FEC
				fec = atoi(optarg);
				break;
			case 'o'://Write permanent
				header=0xC0 & 0xFF;
				break;
			case 'w'://Waittime
				break;
			case 'c':
				channel = atoi(optarg);
				break;
			case 'p'://power
				power = atoi(optarg);
				break;
			case 'a'://air speed
				airspeed = atoi(optarg);
				break;
			case 's'://uart speed
				uartspeed = atoi(optarg);
				break;
			case 'd':
				strcpy(port,optarg);
				break;
			case '?':
				if (optopt == 'c')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr,
							"Unknown option character `\\x%x'.\n",
							optopt);
				return 1;
			default:
				abort (argv);
				return 1;
				break;
		}
	}
	if(argc<2){
		abort(argv);
		return 2;
	}
	if(port[0]==0x00){
		printf("\nPort is required\n");
		abort(argv);
		return 3;
	}

	//Open the serial device
	fd = open (port, O_RDWR | O_NOCTTY );
	if (fd < 0){
		printf ("\nerror %d opening %s: %s\n", errno, port, strerror (errno));
		return 1;
	}
	set_blocking (fd, 1);                // set no blocking

	baudrate=detectBaudrate(fd,buffer,&count);
	switch(baudrate){
		case 0:
			printf("\nDetected Baudrate 1200");
			break;
		case 1:
			printf("\nDetected Baudrate 2400");
			break;
		case 2:
			printf("\nDetected Baudrate 4800");
			break;
		case 3:
			printf("\nDetected Baudrate 9600");
			break;
		case 4:
			printf("\nDetected Baudrate 19200");
			break;
		case 5:
			printf("\nDetected Baudrate 38400");
			break;
		case 6:
			printf("\nDetected Baudrate 57600");
			break;
		case 7:
			printf("\nDetected Baudrate 115200");
			break;
		default:
			printf("\nDetected Baudrate notfoud\n");
			abort(argv);
			return 3;
			break;
	}

	getVersion(fd,buffer,&count);
	printf("\nModule: E%X version: 0x%X  features: 0x%X",buffer[1],buffer[2],buffer[3]);
	version=buffer[1];

	getConfig(fd,buffer,&count);
	printf("\nCurrent config");
	printConfig(buffer);

	if(airspeed>=0){
		//printf("\nAirspeed: %d",airspeed);
		mask=0x07;
		buffer[0]=header;
		buffer[3]&= ~mask;
		buffer[3]|= airspeed & mask;
	}

	if(uartspeed>=0){
		mask=0x38;
		buffer[0]=header;
		buffer[3]&= ~mask;
		buffer[3]|= (uartspeed<<3) & mask;
	}

	if(power>=0){
		mask=0x03;
		buffer[0]=header;
		buffer[5]&= ~mask;
		buffer[5]|= power & mask;
	}

	if(channel>=0){
		switch(version){
			case 0x36:
				mask=0xff;
				break;
			case 0x32:
			default:
				mask=0x1f;
				break;
		}
		buffer[0]=header;
		buffer[4]&= ~mask;
		buffer[4]|= channel & mask;
	}

	if(fec>=0){
		mask=0x04;
		buffer[0]=header;
		buffer[5]&= ~mask;
		buffer[5]|= (fec<<2) & mask;
	}

	if((buffer[0] & 0xff)==(header & 0xff)){
		count=6;
		sendCommand(fd,buffer,&count);
		getConfig(fd,buffer,&count);
		printf("\n\nNew config");
		printConfig(buffer);
	}

	printf("\n\n");
	for(i=0;i<count;i++){
		printf("%2X ", buffer[i] & 0xff);
	}
	printf("\n");

	close(fd);
}
