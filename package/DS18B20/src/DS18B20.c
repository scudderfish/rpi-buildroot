// modified version to read DS18B20 in bit banging
//
// 26 June 2014
// Add fix for no input pull-up or sensor
// Set Start Acquistion waiting delay  using a constant of 750ms

//  25 June 2014
// 
// Updated version
// Search rom  code command added
// More than one sensor  on the same pin
//
// Argument:
//  -s        =>  just scan and display the 64 bits rom code of the probe
//  -gpio n   =>  Specify pin other than GPIO10
//
//  Step process
//  1- Start Conversion using SKIP ROM  (all devices)
//  2- Search for sensor. When one is found, Display the temperature


//  24 May 2014
//  Daniel Perron
//
// Use At your own risk




//
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013


// Access from ARM Running Linux

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
void *gpio_map;

// I/O access
volatile unsigned *gpio;


// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

#define GPIO_READ(g)  (*(gpio + 13) &= (1<<(g)))


#define DS18B20_SKIP_ROM                0xCC
#define DS18B20_CONVERT_T               0x44
#define DS18B20_MATCH_ROM               0x55
#define DS18B20_SEARCH_ROM              0XF0
#define DS18B20_READ_SCRATCHPAD         0xBE
#define DS18B20_WRITE_SCRATCHPAD        0x4E
#define DS18B20_COPY_SCRATCHPAD         0x48
#define DELAY1US  smalldelay();


unsigned char scratchPad[9];
double  temperature;
int   resolution;

void setup_io();



unsigned short DS_PIN=10;
unsigned short argScan=0;
unsigned short ArgFile=0;
char FileName[256];

int  doReset(void)
{
 int loop;

   INP_GPIO(DS_PIN);


   usleep(10);

   INP_GPIO(DS_PIN);
   OUT_GPIO(DS_PIN);

   // pin low for 480 us
   GPIO_CLR=1<<DS_PIN;
   usleep(480);
   INP_GPIO(DS_PIN);
   usleep(60);
   if(GPIO_READ(DS_PIN)==0)
   {
     usleep(420);
     return 1;
   }
  return 0;
}


void  smalldelay(void)
{
  int loop2;
   for(loop2=0;loop2<50;loop2++);
}

void writeByte(unsigned char value)
{
  unsigned char Mask=1;
  int loop;

   for(loop=0;loop<8;loop++)
     {
       INP_GPIO(DS_PIN);
       OUT_GPIO(DS_PIN);
       GPIO_CLR= 1 <<DS_PIN;

       if((value & Mask)!=0)
        {
           DELAY1US
            INP_GPIO(DS_PIN);
           usleep(60);

        }
        else
        {
           usleep(60);
           INP_GPIO(DS_PIN);
           usleep(1);
        }
      Mask*=2;
      usleep(60);
    }


   usleep(100);
}

void WriteBit(unsigned char value)
{
   INP_GPIO(DS_PIN);
   OUT_GPIO(DS_PIN);
   GPIO_CLR=1 <<DS_PIN;
   if(value)
    {
      DELAY1US
      INP_GPIO(DS_PIN);
      usleep(60);
    }
   else
    {
      usleep(60);
      INP_GPIO(DS_PIN);
      usleep(1);
     }
   usleep(60);
}


unsigned char readBit(void)
{
   unsigned char rvalue=0;
   INP_GPIO(DS_PIN);
   OUT_GPIO(DS_PIN);
   // PIN LOW
   GPIO_CLR= 1 << DS_PIN;
   DELAY1US
   // set INPUT
   INP_GPIO(DS_PIN);
   DELAY1US
   DELAY1US
   DELAY1US
   if(GPIO_READ(DS_PIN)!=0)
    rvalue=1;
   usleep(60);
   return rvalue;
}

unsigned char ReadByte(void)
{
  unsigned char Mask=1;
  int loop;
  unsigned  char data=0;

  int loop2;


   for(loop=0;loop<8;loop++)
   {
       //  set output
       INP_GPIO(DS_PIN);
       OUT_GPIO(DS_PIN);
       //  PIN LOW
       GPIO_CLR= 1<<DS_PIN;
       DELAY1US
       //  set input
       INP_GPIO(DS_PIN);
       // Wait  2 us
       DELAY1US
       DELAY1US
       DELAY1US
       if(GPIO_READ(DS_PIN)!=0)
       data |= Mask;
       Mask*=2;
       usleep(60);
    }

    return data;
}



int ReadscratchPad(void)
{
   int loop;

       writeByte(DS18B20_READ_SCRATCHPAD);
       for(loop=0;loop<9;loop++)
         {
          scratchPad[loop]=ReadByte();
        }
}

unsigned char  CalcCRC(unsigned char * data, unsigned char  byteSize)
{
   unsigned char  shift_register = 0;
   unsigned char  loop,loop2;
   char  DataByte;
 
   for(loop = 0; loop < byteSize; loop++)
   {
      DataByte = *(data + loop);
      for(loop2 = 0; loop2 < 8; loop2++)
      {
         if((shift_register ^ DataByte)& 1)
         {
            shift_register = shift_register >> 1;
            shift_register ^=  0x8C;
         }
         else
            shift_register = shift_register >> 1;
         DataByte = DataByte >> 1;
      }
   }
   return shift_register;
}

char IDGetBit(unsigned long long *llvalue, char bit)
{
  unsigned long long Mask = 1ULL << bit;

  return ((*llvalue & Mask) ? 1 : 0);
}


unsigned long long IDSetBit(unsigned long long *llvalue, char bit, unsigned char newValue)
{
  unsigned long long Mask = 1ULL << bit;

  if((bit >= 0) && (bit < 64))
  {
  if(newValue==0)
   *llvalue &= ~Mask;
  else
   *llvalue |= Mask;
   }
  return *llvalue;
}


void SelectSensor(unsigned  long long ID)
{
  int BitIndex;
  char Bit;


  writeByte(DS18B20_MATCH_ROM);

  for(BitIndex=0;BitIndex<64;BitIndex++)
  {
    WriteBit(IDGetBit(&ID,BitIndex));
  }

}

int  searchSensor(unsigned long long * ID, int * LastBitChange)
{
 int BitIndex;
  char Bit , NoBit;


  if(*LastBitChange <0) 
  {
	return 0;
  }

// Set bit at LastBitChange Position to 1
// Every bit after LastbitChange will be 0

  if(*LastBitChange <64)
  {

     IDSetBit(ID,*LastBitChange,1);
     for(BitIndex=*LastBitChange+1;BitIndex<64;BitIndex++)
     {
	   IDSetBit(ID,BitIndex,0);
	 }
  }

  *LastBitChange=-1;

  if(!doReset()) 
  {
	return -1;
  }


  writeByte(DS18B20_SEARCH_ROM);

  for(BitIndex=0;BitIndex<64;BitIndex++)
  {

      NoBit = readBit();
      Bit = readBit();

     if(Bit && NoBit)
        return -2;

     if(!Bit && !NoBit)
        {
          // ok 2 possibilities
//          printf("B");
          if(IDGetBit(ID,BitIndex))
            {
               // Bit High already set 
                WriteBit(1);
             }
          else
             {
               // ok let's try LOW value first
               *LastBitChange=BitIndex;
                WriteBit(0);
             }
         }
      else if(!Bit)
        { 
//   printf("1");
         WriteBit(1);
         IDSetBit(ID,BitIndex,1);
        }
      else
        {
        //printf("0");
        WriteBit(0);
        IDSetBit(ID,BitIndex,0);
        }
//   if((BitIndex % 4)==3)printf(" ");
    }
  return 1;
}

int readSensor(unsigned long long ID)
{
  int RetryCount;
  int loop;
  unsigned char  CRCByte;
  union {
   short SHORT;
   unsigned char CHAR[2];
  }IntTemp;


  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  temperature=-9999.9;

  for(RetryCount=0;RetryCount<10;RetryCount++)
  {

   if(!doReset()) continue;

   // start a conversion
   SelectSensor(ID);

  if(!ReadscratchPad()) continue;

  // OK Check sum Check;
  CRCByte= CalcCRC(scratchPad,8);

  if(CRCByte!=scratchPad[8]) continue;;

  //Check Resolution
   resolution=0;
   switch(scratchPad[4])
   {

     case  0x1f: resolution=9;break;
     case  0x3f: resolution=10;break;
     case  0x5f: resolution=11;break;
     case  0x7f: resolution=12;break;
   }

   if(resolution==0) continue;
    // Read Temperature

    IntTemp.CHAR[0]=scratchPad[0];
    IntTemp.CHAR[1]=scratchPad[1];


    temperature =  0.0625 * (double) IntTemp.SHORT;

    ID &= 0x00FFFFFFFFFFFFFFULL;
    printf("%02llX-%012llX : ",ID & 0xFFULL, ID >>8);

    printf("%02d bits  Temperature: %6.2f +/- %f Celsius\n", resolution ,temperature, 0.0625 * (double)  (1<<(12 - resolution)));

    return 1;
    }

  return 0;

}



int globalStartConversion(void)
{
   int retry=0;
   int maxloop;

   while(retry<10)
   {
     if(!doReset())
      usleep(10000);
     else
      {
       writeByte(DS18B20_SKIP_ROM);
       writeByte(DS18B20_CONVERT_T);
       maxloop=0;

#define USE_CONSTANT_DELAY
#ifdef USE_CONSTANT_DELAY
       usleep(750000);
       return 1;
#else
      // wait until ready
      while(!readBit())
      {
       maxloop++;
       if(maxloop>100000) break;
      }

      if(maxloop<=100000)  return 1;
#endif
     }
    retry++;
   }
   return 0;


}


void writeScratchPad(unsigned char TH, unsigned char TL, unsigned char config)
{
int loop;

    // First reset device

    doReset();

    usleep(10);
    // Skip ROM command
     writeByte(DS18B20_SKIP_ROM);


     // Write Scratch pad

    writeByte(DS18B20_WRITE_SCRATCHPAD);

    // Write TH

    writeByte(TH);

    // Write TL

    writeByte(TL);

    // Write config

    writeByte(config);
}

void  copyscratchPad(void)
{

   // Reset device
    doReset();
    usleep(1000);

   // Skip ROM Command

    writeByte(DS18B20_SKIP_ROM);

   //  copy scratch pad

    writeByte(DS18B20_COPY_SCRATCHPAD);
    usleep(100000);
}



void scanForSensor(void)
{
  unsigned long long  ID=0ULL;
  int  NextBit=64;
  int  _NextBit;
  int  rcode;
  int retry=0;
  unsigned long long  _ID;
  unsigned char  _ID_CRC;
  unsigned char _ID_Calc_CRC;
  unsigned char  _ID_Family;

  while(retry<10)
  {
   _ID=ID;
   _NextBit=NextBit;
   rcode=searchSensor(&_ID,&_NextBit);
    if(rcode==1)
     {
        _ID_CRC =  (unsigned char)  (_ID>>56);
        _ID_Calc_CRC =  CalcCRC((unsigned char *) &_ID,7);
        if(_ID_CRC == _ID_Calc_CRC)
        {
         if(argScan==0)
          {
           if(readSensor(_ID))
            {
              ID=_ID;
              NextBit=_NextBit;
              retry=0;
            }
           else
             retry=0;
          }
          else
           {
            ID=_ID;
            NextBit=_NextBit;
            printf("%016llX\n",ID);
           }
        }
        else retry++;
     }
    else if(rcode==0 )
     break;
    else
     retry++;
  }
}

void decodeArg(int argc, char ** argv)
{

   int idx=1;

   while(idx<argc)
    {
       if(strcmp(argv[idx],"-gpio")==0)
            DS_PIN = atoi(argv[++idx]);
       else if(strcmp(argv[idx],"-s")==0)
         argScan=1;
//       else if(strcmp(argv[idx],"-f")==0)
//        {
//          ArgFile=1;
//          strcpy(FileName,argv[++idx]);
//        }
       else
        {
         printf("Unknown argument %s! ",argv[idx]);
         exit(0);
        }
       idx++;
     }
}


int main(int argc, char **argv)
{
  int loop;
  int config;
  int flag=0;
  // Set up gpi pointer for direct register access

  decodeArg(argc,argv);

  setup_io();

  // Check for pull up resistor
  // Signal  input should be high

  // Set PIN to INPUT MODe
  INP_GPIO(DS_PIN);

  flag=0;
  for(loop=0;loop<100;loop++)
   {
     usleep(1000);
     if(GPIO_READ(DS_PIN)!=0)
        {
          flag=1;
          break;
        }
   }

   if(flag==0)
    {
      printf("*** Error Unable to detect Logic level 1. No pull-up ?\n");
      exit(-1);
    }

    if(globalStartConversion()==0)
    {
      printf("*** Error Unable to detect any DS18B20 sensor\n");
      exit(-2);
    }

     scanForSensor();

  return 0;

} // main


//
// Set up a memory regions to access GPIO
//
void setup_io()
{
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit(-1);
   }

   /* mmap GPIO */
   gpio_map = mmap(
      NULL,             //Any adddress in our space will do
      BLOCK_SIZE,       //Map length
      PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
      MAP_SHARED,       //Shared with other processes
      mem_fd,           //File to map
      GPIO_BASE         //Offset to GPIO peripheral
   );

   close(mem_fd); //No need to keep mem_fd open after mmap

   if (gpio_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)gpio_map);//errno also set!
      exit(-1);
   }

   // Always use volatile pointer!
   gpio = (volatile unsigned *)gpio_map;


} // setup_io