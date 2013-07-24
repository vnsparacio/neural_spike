#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
  char timebuffer[30];
	char timebuffer2[30];

  struct timeval tv;
	struct timeval tv2; 

	time_t curtime; 
  time_t endtime;

	gettimeofday(&tv, NULL); 
	curtime=tv.tv_sec;
	strftime(timebuffer,30,"%m-%d-%Y  %T.",localtime(&curtime));

	while(1)
	{
		gettimeofday(&tv2, NULL);  	
		
	 	endtime = tv2.tv_sec; 
		
		strftime(timebuffer2,30,"%m-%d-%Y  %T.",localtime(&endtime));
	
	 	printf("%s%ld\n",timebuffer,tv.tv_usec);
		printf("%s%ld\n",timebuffer2,tv2.tv_usec);
		
		if(tv2.tv_usec > tv.tv_usec){
			if((tv2.tv_usec - tv.tv_usec) > 500000)
			{
				printf("500 MILLISECONDS HAS ELAPSED\n"); 
				sleep(3); 
				gettimeofday(&tv, NULL); 
				curtime=tv.tv_sec;
				strftime(timebuffer,30,"%m-%d-%Y  %T.",localtime(&curtime));
			}	
		} else{
			if((tv.tv_usec - tv2.tv_usec) > 500000)
			{
				printf("500 MILLISECONDS HAS ELAPSED\n"); 
				sleep(3); 
				gettimeofday(&tv, NULL); 
				curtime=tv.tv_sec;
				strftime(timebuffer,30,"%m-%d-%Y  %T.",localtime(&curtime));
			}		
		}
	}	

	return 0; 
}
