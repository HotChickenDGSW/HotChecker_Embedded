#include "stm32f10x.h"


volatile unsigned int TimingDelay;
volatile unsigned int count_micros;


short values[8];
char serialprint[100] = "asd";
double res2;

unsigned long micros_10us(){
   return count_micros;
}

void Delay(unsigned int nTime){
   TimingDelay = nTime;
   
   while(TimingDelay != 0);
}

void TimingDelay_Decrement(void){
   if(TimingDelay != 0x00){
      TimingDelay--;
   }
}

void SysTick_Handler(void)
{
  TimingDelay_Decrement();
    count_micros++;
    
}

void BuildMessage(unsigned char address, unsigned char type, unsigned short start, unsigned short registers, unsigned char message[], int message_size) ;
int CheckResponse(unsigned char response[], int response_size);
int sendfc3(unsigned char address, unsigned short start, unsigned short registers, short values[]);
void GetCRC(unsigned char message[], unsigned char crc[], int message_size) ;

int SerialAvailable() {
  return 1;
}

void SerialWrite(unsigned char *data, int sz) {
  for (int i = 0; i < sz; i++) {
		USART1->DR = data[i];
		while(!(USART1->SR & (0x01<<6)));
		USART2->DR = data[i];
		while(!(USART2->SR & (0x01<<6)));
  }
}

void SerialReadBytes(unsigned char *data, int sz) {
  for (int i = 0; i < sz; i++) {
		while(!(USART1->SR & (0x01<<5)));
    data[i] = USART1->DR;
		USART2->DR = data[i];
		while(!(USART2->SR & (0x01<<6)));
  }
}

void SerialPrintln(char *data) {
  int idx = 0;
  while (data[idx] != '\0') {
		USART2->DR = data[idx];
		while(!(USART2->SR & (0x01<<6)));
    idx++;
  }
}

int sendfc3(unsigned char address, unsigned short start, unsigned short registers, short values[]) {
  if (SerialAvailable()) {
    unsigned char message[8];
    unsigned char response[5 + 2 * registers];

    BuildMessage(address, (unsigned char)3, start, registers, message, (sizeof(message) / sizeof(unsigned char)));

    SerialWrite(message, sizeof(message) / sizeof(unsigned char));
    SerialReadBytes(response, sizeof(response) / sizeof(unsigned char));

    if (CheckResponse(response, (sizeof(response) / sizeof(unsigned char)))) {
      for (int i = 0; i < (sizeof(response) / sizeof(unsigned char) - 5) / 2; i++) {
        values[i] = response[2 * i + 3];
        values[i] <<= 8;
        values[i] += response[2 * i + 4];
      }
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

double GetTemp() {
  while (!sendfc3(1, 0, 8, values)) ;
  short data1 = (short)(values[2] >> 8);
  short data2 = ((short)(values[2] & 0x00FF));
  int res = data1 * 256 + data2;
  return ((double)((res * 0.02) - 273.15));
}

void GetCRC(unsigned char message[], unsigned char crc[], int message_size) {
  unsigned short CRCFull = 0xFFFF;
  unsigned char CRCHigh = 0xFF, CRCLow = 0xFF;
  char CRCLSB;
  for (int i = 0; i < message_size - 2; i++) {
    CRCFull = (unsigned short)(CRCFull ^ message[i]);
    for (int j = 0; j < 8; j++) {
      CRCLSB = (char)(CRCFull & 0x0001);
      CRCFull = (unsigned short)((CRCFull >> 1) & 0x7FFF);
      if (CRCLSB == 1)
        CRCFull = (unsigned short)(CRCFull ^ 0xA001);
    }
  }
  crc[1] = CRCHigh = (unsigned char)((CRCFull >> 8) & 0xFF);
  crc[0] = CRCLow = (unsigned char)(CRCFull & 0xFF);
}


void BuildMessage(unsigned char address, unsigned char type, unsigned short start, unsigned short registers, unsigned char message[], int message_size) {
  unsigned char crc[2];
  message[0] = address;
  message[1] = type;
  message[2] = (unsigned char)(start >> 8);
  message[3] = (unsigned char)start;
  message[4] = (unsigned char)(registers >> 8);
  message[5] = (unsigned char)registers;
  GetCRC(message, crc, message_size);
  message[message_size - 2] = crc[0];
  message[message_size - 1] = crc[1];
}



int CheckResponse(unsigned char response[], int response_size) {
  unsigned char crc[2];
  GetCRC(response, crc, response_size);
  if (crc[0] == response[response_size - 2] && crc[1] == response[response_size - 1])
    return 1;
  else
    return 0;
}

unsigned char te;

int main(void){
	
	
	SystemInit();
	
	
	SysTick->LOAD = 720;
	SysTick->CTRL = 0X07;
	
	/*USART2 SETTING*/
	RCC->APB2ENR |= (0x01<<2);
	RCC->APB1ENR |= (0x01<<17);
	
	GPIOA->CRL &= ~(0xf<<(4*2));
	GPIOA->CRL |= 0xB<<(4*2);
	
	USART2->CR1 |= (0x01<<2) | (0x01<<3) | (0x01<<13) ;
	USART2->BRR = 0xEA6;
	
	/*USART1 SETTING*/
	RCC->APB2ENR |= (0x01<<14);
	
	GPIOA->CRH &= ~(0xB<<(4*1));
	GPIOA->CRH |= 0xB<<(4*1);
	
	USART1->BRR = 0xEA6;
	USART1->CR1 |= (0x01<<2) | (0x01<<3) | (0x01<<13) ;
	
	
	while(1){
   res2 = GetTemp();
		Delay(1000);
	//	while(!(USART1->SR & (0x01<<5)));
 //   te = USART1->DR;
//		USART2->DR = te;
	//	while(!(USART2->SR & (0x01<<6)));
	}
	
}