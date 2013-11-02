#include <stdio.h>                                                                                                                                                 
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include "../libmodbus-3.0.3/src/modbus.h"
#include "helper.c"

#define cll printf("%c[K", 27)
//////#define SIMULATE

int main(void)
{
#ifdef SIMULATE

#else	
   modbus_t *ctx = modbus_new_rtu("/dev/ttyUSB0", 19200, 'N', 8, 2); 
   if (ctx == NULL) 
   {   
	  fprintf(stderr, "Unable to create the libmodbus context\n");
	  modbus_free(ctx);
	  return -1; 
   }   
   modbus_set_debug(ctx, FALSE);
   modbus_set_slave(ctx, 1); 

   if (modbus_connect(ctx) < 0)
   {   
	  fprintf(stderr, "Connection failed: %s\n",
		 modbus_strerror(errno));
	  modbus_free(ctx);
	  return -1; 
   }    
   int addr = 0x091B; //RUN/STOP 09.26                                       
   int addr2 = 0x091A; //Speed 09.25
   int count = 1;
   uint16_t data[12];
#endif

   int currentCmdSpeed, lastCmdSpeed;
   int currentCmdBoost, lastCmdBoost;
   int currentBoostVal, lastBoostVal;
   int currentSlowVal, lastSlowVal;
   int currentFastVal, lastFastVal;
   int currentRunVal, lastRunVal;

   currentCmdSpeed = 0;
   currentCmdBoost = 0;
   currentBoostVal = 0;
   currentSlowVal = 20;
   currentFastVal = 200;
   currentRunVal = 100;
   
   
   //int currentCommanded= '?';
   //unsigned int currentCmdSpeed = 0;
   int firstRunWrite = 0;

   int dirty = 1; //Needs reading
   int writeDirty = 1; //Needs writing 
   time_t lastReadTime = time(NULL);
   
   double lastSuccessfulRead = 0;
   int lastRead_validSpeed = 0;

   unsigned short status1 = 0;
   unsigned short status2 = 0;
   int actualFreq = 0;
   int outputAmps = 0; 
   int dcvoltage = 0;
   int outputvoltage = 0; 
   int motorrpm = 0;
   double kmh = 0;
   
#ifdef SIMULATE
   unsigned short SIMstatus1 = 0;
   unsigned short SIMstatus2 = 0;
   int SIMactualFreq = 0;
   int SIMoutputAmps = 0; 
   int SIMdcvoltage = 0;
   int SIMoutputvoltage = 0; 
   int SIMmotorrpm = 0;
   double SIMkmh = 0;
#endif

   int timeouts = 0;
   int maxTimeouts = 100;

   int rs485Error = 1;
   int error = 0;
   double totaldistance = 0;			
   time_t lastAttemptTime = time(NULL);
   time_t stopTime = time(NULL);
   int stopped = 1;

//cmdSpeed = {stopped, slow, fast, run}
//cmdBoost    = {-1, 0, 1}
//boostVal = (int)
//slowVal  = (int)
//fastVal  = (int)
//runVal   = (int)

   char readBuff[20];
   int readPSettingsAttempts = 0;
   int maxReadSettingsAttempts = 10;
   int numReads = 0;
   int numWrites = 0;

   printf("%c[2J", 27);
   while(1)
   {
	  printf("%c[10;0H", 27);
	  FILE *fp;

	  if((fp = fopen("/home/www/VTsettings", "r")) == NULL)
	  {
		 cll;
		 printf("Missing /home/www/VTsettings file\n");
 		 printf("X\nX\nX\nX\n");

		 readPSettingsAttempts++;
		 if(readPSettingsAttempts > maxReadSettingsAttempts)
		 {
			currentSlowVal = 10;
			currentFastVal = 600;
			currentRunVal = 300;
			currentBoostVal = 50;
			error = 1;
		 }
	  }
	  else
	  {
	  	 cll;
  		 printf("Read /home/www/VTsettings\n");
		 readPSettingsAttempts = 0;
		 
		 
		 if(fgets(readBuff, 20, fp) != NULL)
		 {
			int latestBoostVal = atoi(readBuff);	
			cll;
		 	printf("Read commanded boostVal: %d\n", latestBoostVal);
			
			if(currentBoostVal != latestBoostVal)
			{
			   currentBoostVal = latestBoostVal;
			   dirty = 1;
			   writeDirty = 1;
			}
		 }
		 
		 if(fgets(readBuff, 20, fp) != NULL)
		 {
			int latestSlowVal = atoi(readBuff);	
			cll;
		 	printf("Read commanded slowVal: %d\n", latestSlowVal);
			
			if(currentSlowVal != latestSlowVal)
			{
			   currentSlowVal = latestSlowVal;
			   dirty = 1;
			   writeDirty = 1;
			}
		 }
		 
		 if(fgets(readBuff, 20, fp) != NULL)
		 {
			int latestFastVal = atoi(readBuff);	
			cll;
		 	printf("Read commanded fastVal: %d\n", latestFastVal);
			
			if(currentFastVal != latestFastVal)
			{
			   currentFastVal = latestFastVal;
			   dirty = 1;
			   writeDirty = 1;
			}
		 }

		 if(fgets(readBuff, 20, fp) != NULL)
		 {
			int latestRunVal = atoi(readBuff);	
			cll;
		 	printf("Read commanded runVal: %d\n", latestRunVal);
			
			if(currentRunVal != latestRunVal)
			{
			   currentRunVal = latestRunVal;
			   dirty = 1;
			   writeDirty = 1;
			}
		 }

		 fclose(fp);
	  }
	  
	  if((fp = fopen("/tmp/VTsettings", "r")) == NULL)
	  {
		 //Do nothing. If the file doesn't exist, continue with last settings!
		 //If we start with setting of "off" this works well.
		 //If website re-writes "start" the user wants it to go!
		 cll;
  		 printf("Missing /tmp/VTsettings file\n");
 		 printf("X\nX\n");
	  }
	  else
	  {
		 if(fgets(readBuff, 20, fp) != NULL)
		 {
			int latestCmdSpeed = atoi(readBuff);	
			cll;
		 	printf("Read commanded cmdSpeed: %d\n", latestCmdSpeed);
			
			if(latestCmdSpeed >= 0 && currentCmdSpeed != latestCmdSpeed)
			{
			   dirty = 1;
			   writeDirty = 1;
			   currentCmdSpeed = latestCmdSpeed;

			   if(currentCmdSpeed)
				  firstRunWrite = 1;
			}
		 }
		 
		 if(fgets(readBuff, 20, fp) != NULL)
		 {
			int latestCmdBoost = atoi(readBuff);	
			cll;
		 	printf("Read commanded cmdBoost: %d\n", latestCmdBoost);
			
			if(currentCmdBoost != latestCmdBoost)
			{
			   currentCmdBoost = latestCmdBoost;
			   dirty = 1;
			   writeDirty = 1;
			}
		 }
		 fclose(fp);
		 remove("/tmp/VTsettings");
	  }

	  if(readPSettingsAttempts == 0)
		 error = 0;


	int currentFreqSetpoint; 
	switch(currentCmdSpeed)
	{
		case 0: //Stopped
			currentFreqSetpoint = 0; 
			break;
		case 1: //Slow 
			currentFreqSetpoint = currentSlowVal; 
			break;
		case 2: //Fast
			currentFreqSetpoint = currentFastVal; 
			break;
		case 3: //Run
			currentFreqSetpoint = currentRunVal; 
			currentFreqSetpoint += currentCmdBoost * currentBoostVal;
			if(currentFreqSetpoint<18)
				currentFreqSetpoint = 18;
			break;
		default: //error
			currentFreqSetpoint = 0; 
			currentCmdSpeed = 0;
			break;
	}
	if(currentFreqSetpoint < 0)
	 currentFreqSetpoint = 0; //Minimum of zero

	cll;
	printf("Current setpoint: %f\n", currentFreqSetpoint/10.0);
	cll;
	printf("Error status: %d\n", error);
		
	  
	if(writeDirty)
	{
		writeDirty = 0;
	  	numWrites++;
	

#ifdef SIMULATE	 
	
		SIMstatus1 = 0;
		SIMstatus2 = (1<<5) | (1<<7);


		if(currentCmdSpeed != 0)
		{
			SIMstatus2 |=  0x03;
			SIMoutputAmps = 14;
		}
		else
		{
			SIMoutputAmps = 0;
		}
		SIMactualFreq = currentFreqSetpoint;
		SIMmotorrpm = SIMactualFreq * 3;
		SIMoutputvoltage = 230 * SIMactualFreq / 60;
		SIMdcvoltage = 3350;


		SIMkmh = 0.02743 * (double)SIMmotorrpm;
		 

#else
	  
	  
		//VFD write starts here


		uint16_t desiredRunState = (currentFreqSetpoint >= 18);
		if(firstRunWrite || desiredRunState == 0)
		{
			if(modbus_write_register(ctx, addr, desiredRunState) < 0)         
			{
				fprintf(stderr, "Write failed: %s\n",
				modbus_strerror(errno));
		 	}
			else
		 	{
				cll;
				printf("Successfully wrote Run state %d\n", desiredRunState);
				firstRunWrite = 0;
			}
		}
		else
		{
			printf("\n");
		}

		if(modbus_write_register(ctx, addr2, currentFreqSetpoint) < 0)
		{
		 	fprintf(stderr, "Write failed: %s\n",
				modbus_strerror(errno));
		}
		else
		{
		 	cll;
		 	printf("Successfully wrote speed %u\n", currentFreqSetpoint);
		}   

		//VFD write stops here

#endif
	}


int forceRetry = file_exists_delete("/tmp/VTforceretry");
	time_t currTime = time(NULL);

	if(actualFreq == 0 && currentFreqSetpoint == 0)
	{
          if(stopped == 0)
	  {
	    stopTime = currTime;
	    stopped = 1;
	  }
	}
	else
	{
	  stopped = 0;
	  //stopTime = currTime; //Lie! So we don't read unnecessarily, in the case where we're stopped and currTime wraps around but stopTime doesn't.
	}


	//cll;
        //printf("stopped=%d   stopDiff=%d\n", stopped, currTime - stopTime);
	if(( (!stopped
	|| (stopped && stopTime != time(NULL) && (currTime - stopTime) < 5) 
	|| dirty != 0) && rs485Error == 0) 
	|| currTime > lastAttemptTime + 20 || forceRetry == 0)
	{
	 	 dirty = 0;
		 lastAttemptTime = currTime;
		 numReads++;


#ifdef SIMULATE
		//Simulation starts here

		 rs485Error = 0;

		 status1 = SIMstatus1; 
		 status2 = SIMstatus2;
		 outputAmps = SIMoutputAmps;
		 actualFreq = SIMactualFreq; 
		 motorrpm = SIMmotorrpm;
		 outputvoltage = SIMoutputvoltage;
		 dcvoltage = SIMdcvoltage;
		 kmh = SIMkmh;
		 
		  //Simulation ends here
	
#else	  

		  //VFD read starts here
  
		 if(modbus_read_registers(ctx, 0x2100, 12, data) < 0)
		 {
			lastRead_validSpeed = 0;
			fprintf(stderr, "Read failed: %s\n",
			   modbus_strerror(errno));
			dirty = 1;
			
			rs485Error++;
			if(rs485Error > maxTimeouts)
			   sleep(10);
		 }
		 else
		 { 
			rs485Error=0;
			status1 = data[0];
			status2 = data[1];
			actualFreq = data[2];
			outputAmps = data[4];
			dcvoltage = data[5];
			outputvoltage = data[6];
			motorrpm = data[7];
			//loadPct = data[10];
			
			kmh = 0.02743 * (double)motorrpm;

			double mm_per_km = 1000*100*10;
			double uptime = ms_uptime();
			double sec = (uptime - lastSuccessfulRead)/1000;
			double dist = kmh * (mm_per_km * sec/3600);	
			lastSuccessfulRead = uptime;

			cll;
			printf("Seconds elapsed this sample: %f\n", sec);
			cll;
			printf("mm travelled this sample: %f\n", dist); 

			if(lastRead_validSpeed)
			{
				totaldistance += dist;
			}
			lastRead_validSpeed=1;
		 }



		  //VFD read ends here
#endif
	}	
	else
	{
		 cll;
		 printf("rs485Error=%d   secondsLeft=%ld\n", rs485Error, 20+lastAttemptTime-currTime);
	}
	
	  cll;
	  printf("Writes: %d\n", numWrites);

	  cll;
	  printf("Reads: %d\n", numReads);



	  fp = fopen("/tmp/statusTmp", "w");
	  if(fp != NULL)
	  {
		 fprintf(fp, "%d\n", rs485Error);	//Communication error //0=OK 1=notcommunicating
		 fprintf(fp, "%d\n", error);	//frequency value file couldn't be read
	     	 fprintf(fp, "%d\n", status1);	//VFD error code

		 if((status2 & 0x03) == 0x03)
			fprintf(fp, "%d\n", currentCmdSpeed);
		 else
			fprintf(fp, "0\n");

		 int rs485 = (1<<5) | (1<<7);
		 if((status2 & rs485) == rs485)
		   fprintf(fp, "1\n");
		 else
		   fprintf(fp, "0\n");
	     
		 fprintf(fp, "%f\n", actualFreq/10.0);
	     	 fprintf(fp, "%f\n", outputAmps/10.0);
	         fprintf(fp, "%f\n", outputvoltage/10.0);
	     	 fprintf(fp, "%f\n", dcvoltage/10.0);
	     	 fprintf(fp, "%d\n", motorrpm);
	     	 fprintf(fp, "%f\n", kmh);
	     	 fprintf(fp, "%f\n", totaldistance/1000000);
		 fprintf(fp, "%d\n", currentBoostVal);
		 fprintf(fp, "%d\n", currentSlowVal);
		 fprintf(fp, "%d\n", currentFastVal);
		 fprintf(fp, "%d\n", currentRunVal);
	     fclose(fp);
	  }
	  rename("/tmp/statusTmp", "/tmp/status");

    	  cll;
	  printf("%.1f freq\n", actualFreq/10.0);
	  cll;
	  printf("%.1f output amps\n", outputAmps/10.0);
	  cll;
	  printf("%.1f output volts\n", outputvoltage/10.0);
	  cll;
	  printf("%.1f DC bus volts\n", dcvoltage/10.0);
	  cll;
	  printf("%d RPM\n", motorrpm);
	  cll;
	  printf("%.3f km/h\n", kmh);
	  cll;
	  printf("%.6f km\n", totaldistance/1000000);
	  cll;
	  printf("\n");

	  usleep(50000);
	  continue; 


  }
   return 0;
}
