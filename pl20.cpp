/**
 * @file pl20.cpp
 *
 * https://github.com/helioz2000/pl20
 *
 * Author: Erwin Bejsta
 * July 2020
 */

/*********************
 *      INCLUDES
 *********************/

#include "pl20.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/select.h>

using namespace std;

/* PL20 comms */
#define PL20_CMD_RD_RAM 20		// Read from processor RAM
#define PL20_CMD_RD_EEPROM 72	// Read from EEPROM
#define PL20_CMD_WR_RAM 152		// Write to processor RAM
#define PL20_CMD_WR_EEPROM 202	// Write to EEPROM
#define PL20_CMD_PUSH 87		// Short push or long push


/*********************
 * MEMBER FUNCTIONS
 *********************/
 
Pl20::Pl20() {
	printf("%s\n", __func__);
	throw runtime_error("Class Pl20 - forbidden constructor");
}

Pl20::Pl20(const char* ttyDeviceStr, int baud) {
	if (ttyDeviceStr == NULL) {
		throw invalid_argument("Class Pl20 - ttyDeviceStr is NULL");
	}
	this->_ttyDevice = ttyDeviceStr;
	this->_ttyBaud = baud

}

Pl20::~Pl20() {
	
}

int Pl20::read_RAM(unsigned char address, unsigned char *readValue) {
	unsigned char value;
	
	if (_open_tty() < 0) 
		return -1;

	if (_tty_write(address, PL20_CMD_RD_RAM) < 0)
		goto return_fail;

	if (_tty_read(&value) < 0)
		goto return_fail;

	*readValue = value;
	return 0;
	
return_fail:
	_tty_close();
	return -1;
}

int Pl20::_tty_open() {

	this->_tty_fd = open(this->_ttyDevice.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
	if (_tty_fd < 0) {
		printf("Error opening %s: %s\n", this->_ttyDevice.c_str(), strerror(errno));
		return -1;
	}
	/*baudrate 8 bits, no parity, 1 stop bit */
	if (_tty_set_attribs(this->_tty_fd, this->baud) < 0){
		this->_tty_close();
		return -1;
	}
	//set_mincount(tty_fd, 0);                /* set to pure timed read */
	return 0;
}

void Pl20::_tty_close() {
	close(this->_tty_fd);
	this->_tty_fd = -1;
}


int Pl20::_tty_set_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 2;			// wait for 2 bytes (std reply)
    tty.c_cc[VTIME] = 10;		// wait for 1 second

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int Pl20::_tty_write(unsigned char address, unsigned char cmd) {
	int wrLen;
	unsigned char txbuf[10];
	txbuf[0] = cmd;
	txbuf[1] = address;
	txbuf[2] = 0;
	txbuf[3] = 255 - cmd;		// One's complement

	wrLen = write(this->_tty_fd, txbuf, 4);
	if (wrLen != 4) {
		printf("Error from write: %d, %d\n", wrLen, errno);
		return -1;
	}
	tcdrain(this->_tty_fd);    /* delay for output */
	return 0;
}

/* read PL20 reply */
int Pl20::_tty_read(unsigned char *value) {
	int rxlen = 0;
	int rdlen;
	unsigned char buf[80];

	fd_set rfds;
	struct timeval tv;
	int select_result;

	FD_ZERO(&rfds);
	FD_SET(this->_tty_fd, &rfds);
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	select_result = select(this->_tty_fd + 1, &rfds, NULL, NULL, &tv);

	if (select_result == -1) {
		perror("select()");
		return -1;
	}
	if (select_result) {
		printf("Data is available now.\n");
	} else {
		printf("No data within timeout.\n");
		return -1;
	}

	do {
		rdlen = read(this->_tty_fd, buf, sizeof(buf) - 1);
		if (rdlen > 0) {
			rxlen += rdlen;
			if  (buf[0] != 200) {
			printf("Error response expected:%d received:%d\n", 200, buf[0]);
			return -1;
			}
			*value = buf[1];
		} else if (rdlen < 0) {
			printf("Error from read: %d: %s\n", rdlen, strerror(errno));
			return -1;
		} else {  /* rdlen == 0 */
			printf("Timeout from read\n");
			return -1;
		}
	} while (rxlen < 2);
	return 0;
}

