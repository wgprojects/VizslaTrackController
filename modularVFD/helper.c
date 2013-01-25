
const char* byte_to_binary( short int x )
{
   static char b[sizeof(short int)*8+1] = {0};
   int y;
   long long z;
   for (z=(1LL<<((sizeof(short int)*8)-1)),y=0; z>0; z>>=1,y++)
   {
       b[y] = ( ((x & z) == z) ? '1' : '0');
   }
   b[y] = 0;
   return b;
}

double ms_uptime(void)
{
    FILE *in=fopen("/proc/uptime", "r");
    double retval=0;
    char tmp[256]={0x0};
    if(in!=NULL)
    {
    	fgets(tmp, sizeof(tmp), in);
    	retval=atof(tmp);
    	fclose(in);
    }
    return retval*1000;
			        
}

int file_exists_delete(const char * filename)
{
   FILE *file;
   if ((file = fopen(filename, "r"))) 
   {
	  fclose(file);
	  remove(filename);
	  return 0;
   }
   return -1;
}
