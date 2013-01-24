#include <stdio.h>                                                                                                                                                 
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "../libmodbus-3.0.3/src/modbus.h"

int main(int argc, char* argv[])
{
   if(argc != 2)
   {
	  printf("Usage\n");
	  printf("vfdstate 1 or vfdstate 0 (485 or Front panel)\n");
	  return -1;
   }

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

   int count = 1;

   
   if(argv[1][0]=='0')
   {
	  printf("Setting inputs to front panel");
	  if(modbus_write_register(ctx, 0x0300, 0x00) < 0)
	  {
		 fprintf(stderr, "Write failed: %s\n",
			modbus_strerror(errno));
	  }
	  if(modbus_write_register(ctx, 0x0400, 0x00) < 0)
	  {
		 fprintf(stderr, "Write failed: %s\n",
			modbus_strerror(errno));
	  }
   }
   else
   {
	  printf("Setting inputs to RS-485");
	  if(modbus_write_register(ctx, 0x0300, 0x03) < 0)
	  {
		 fprintf(stderr, "Write failed: %s\n",
			modbus_strerror(errno));
	  }
	  if(modbus_write_register(ctx, 0x0400, 0x05) < 0)
	  {
		 fprintf(stderr, "Write failed: %s\n",
			modbus_strerror(errno));
	  }	  
   }
   modbus_close(ctx);
   modbus_free(ctx);
   return 0;
}
