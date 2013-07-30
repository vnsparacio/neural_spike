/*
 * neuron_monitor.c
 * STANFORD EE REU 2013
 * Vincent N. Sparacio
 *
 * This program is utilizes the Raspberry pi SPI interface to sample
 * analog data and do subsequent processing. The code implements a
 * peak detection algorithm that searches for peaks within the data
 * and turns on an LED when certain patterns are found.
 */

#include <bcm2835.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define LED_PIN RPI_GPIO_P1_11
#define NEURON_SIGNAL RPI_GPIO_P1_15


static char *device = "/dev/spidev0.0";
static char *device2 = "/dev/spidev0.1";
static const int buf_size = 1000; 
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 32000000;
static uint16_t delay;
static const int PROCESSBLOCK_TIME=500000; //time for each processing block
static const int STARTOFBLOCK_SYM=9999;//value to denote the start of each collection of blocks
static const int NUMBLOCKS=20;
static const int PEAKFLAG_VALUE=500;

int main(int argc, char *argv[])
{
    FILE *file; //input spidev0_0
    FILE *file2; //input spidev1_1
    FILE *peaks; //peaks per block
    FILE *peaks2; //peaks2
    
    file = fopen("input0.txt", "a+"); //append file
    file2 = fopen("input1.txt", "a+");
    peaks = fopen("peaks1.txt", "a+");
    peaks2 = fopen("peaks2.txt", "a+"); 
    //initialize led gpio interface
    if (!bcm2835_init())
	return 1;
    
    bcm2835_gpio_fsel(LED_PIN, BCM2835_GPIO_FSEL_OUTP); //configure LED as an output
	bcm2835_gpio_fsel(NEURON_SIGNAL, BCM2835_GPIO_FSEL_OUTP);//configure signal to neurons as an output
    
    //devices
    int fd;
    int fd2;
    
    fd = open(device, O_RDWR);
    fd2 = open(device2, O_RDWR);
    
    	/*
	 * spi mode
	 */
	ioctl(fd, SPI_IOC_WR_MODE, &mode);
	ioctl(fd, SPI_IOC_RD_MODE, &mode);
    
	ioctl(fd2, SPI_IOC_WR_MODE, &mode);
	ioctl(fd2, SPI_IOC_RD_MODE, &mode);
    
	/*
	 * bits per word
	 */
	ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    
	ioctl(fd2, SPI_IOC_WR_BITS_PER_WORD, &bits);
	ioctl(fd2, SPI_IOC_RD_BITS_PER_WORD, &bits);
    
	/*
	 * max speed hz
	 */
	ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    
	ioctl(fd2, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	ioctl(fd2, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    
	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
    
    //Begin Transfer Function
    int counter = 0; //index of buffers
    uint16_t rx_buffer[buf_size]; //buffer of data for input0
    uint16_t rx_buffer2[buf_size]; //buffer of data for input1

    int peak_analysis[NUMBLOCKS]; //buffer for peak analysis of input0
    int peak_analysis2[NUMBLOCKS]; //buffer for peak analysis of input1 
    
    //transmit two bytes of data to received two bytes of data
    uint8_t tx[] = {
        0xFF, 0xFF,
    };
    
    //initialize receive arrays
    uint8_t rx[ARRAY_SIZE(tx)] = {0, };
    uint8_t rx2[ARRAY_SIZE(tx)] = {0, };
    
    //Congifgure transfer structure and variables 
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ARRAY_SIZE(tx),
        .delay_usecs = delay,
        .speed_hz = speed,
        .bits_per_word = bits,
    };
    
    struct spi_ioc_transfer tr2 = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx2,
        .len = ARRAY_SIZE(tx),
        .delay_usecs = delay,
        .speed_hz = speed,
        .bits_per_word = bits,
    };
    
    //tells whether or not a peak has been detected on one of the channels
    int peak_flag1 = 0;
    int peak_flag2 = 0;
    //counts peaks when both flags are not high
    int peak_count1 = 0;
    int peak_count2 = 0;
    int block_counter = 0; //counts the number of blocks

    int led_on = 0; //tells whether or not the LED is on
    
    struct timeval tv;
    struct timeval tv2;
    
    //initialize first block
    rx_buffer[counter] = STARTOFBLOCK_SYM; //start character
    rx_buffer2[counter] = STARTOFBLOCK_SYM; //start character
    
    bcm2835_gpio_write(NEURON_SIGNAL, HIGH);
    int nSig = 1;
    
    counter++;
    
    gettimeofday(&tv, NULL); //start time
    gettimeofday(&tv2, NULL); //continuously sampling time
    
    while(1){
        if(block_counter == 2 && led_on == 1){
            bcm2835_gpio_write(LED_PIN, LOW);
            led_on = 0;
        }
        
        //reset block counter
        if(block_counter == NUMBLOCKS){
            peak_count1 = 0; 
            peak_count2 = 0;
            block_counter = 0;
            bcm2835_gpio_write(NEURON_SIGNAL, HIGH);
            nSig = 1;
            //dump the peak counter
            for(int i = 0; i < NUMBLOCKS; i++){
                fprintf(peaks, "%d\n", peak_analysis[i]);
                fprintf(peaks2, "%d\n", peak_analysis2[i]);
            }
	    fflush(peaks); 
	    fflush(peaks2); 
        }
        
        int second_time = tv2.tv_usec;
        if(tv2.tv_sec > tv.tv_sec){
            second_time = 1000000 + tv2.tv_usec;
        }
        
        if((second_time - tv.tv_usec) > PROCESSBLOCK_TIME){
            if(nSig == 1){
                bcm2835_gpio_write(NEURON_SIGNAL, LOW);
                nSig = 0;
            }
            peak_analysis[block_counter] = peak_count1;
            peak_analysis2[block_counter] = peak_count2;
        
            gettimeofday(&tv, NULL);
            peak_count1 = 0;
            peak_count2 = 0;
            rx_buffer[counter] = STARTOFBLOCK_SYM; //start character
            rx_buffer2[counter] = STARTOFBLOCK_SYM; //start character
            counter++;
            block_counter++;
        }

        
        gettimeofday(&tv2, NULL); //continuously sampling time
           
        //acquire data from first input 0_0
        ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        /*incoming stream consists of 14 bits, MSB first. 2 setup bits followed by the 12 bit digital input. Following bits are garbage.*/
        uint16_t received = ((uint16_t)rx[0] << 8) | (uint16_t)rx[1];
        //received = received & 0011111111111111;
        received = received >> 3;
        rx_buffer[counter] = received;
           
        //acquire data from second input 0_1
        ioctl(fd2, SPI_IOC_MESSAGE(1), &tr2);
        /*incoming stream consists of 14 bits, MSB first. 2 setup bits followed by the 12 bit digital input. Following bits are garbage.*/
        uint16_t received2 = ((uint16_t)rx2[0] << 8) | (uint16_t)rx2[1];
        //received2 = received2 & 0011111111111111;
        received2 = received2 >> 3;
        rx_buffer2[counter] = received2;
        
        //Analyze data for peaks
        if((rx_buffer[counter] > PEAKFLAG_VALUE) && (peak_flag1 == 0))
            peak_flag1 = 1;
        else if((rx_buffer[counter] < PEAKFLAG_VALUE) && (peak_flag1 == 1)){
            peak_count1++;
            peak_flag1 = 0;
        }
        if((rx_buffer2[counter] > PEAKFLAG_VALUE) && (peak_flag2 == 0))
            peak_flag2 = 1;
        else if(rx_buffer2[counter] < PEAKFLAG_VALUE && peak_flag2 == 1){
            peak_count2++;
            peak_flag2 = 0;
        }
        
        if(block_counter == 1 && ((peak_analysis[block_counter-1] > 5) || (peak_analysis2[block_counter-1] > 5))){
            bcm2835_gpio_write(LED_PIN, HIGH);
            led_on = 1;
        }

        counter++; //increment to the next index of the arrays

        //dump data from buffer into file
        if (counter > 1000){
		int i=0;
			
		while(i < 1000){
                	//print 0_0 data into the first file
			fprintf(file, "%d\n", rx_buffer[i]);
                	//print 1_1 data into the second file
                	fprintf(file2, "%d\n", rx_buffer2[i]);
			i++;
		}
		fflush(file);
		fflush(file2);
		counter = 0;
	}
    }
    fclose(file);
    fclose(file2);
    fclose(peaks);
    fclose(peaks2); 
    
    return 1; 
}
