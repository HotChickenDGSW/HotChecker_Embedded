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

void BuildMessage(uint8_t  address, uint8_t  type, unsigned short start, unsigned short registers, uint8_t  message[], int message_size) ;
int CheckResponse(uint8_t  response[], int response_size);
int sendfc3(uint8_t  address, unsigned short start, unsigned short registers, short values[]);
void GetCRC(uint8_t  message[], uint8_t  crc[], int message_size) ;

int SerialAvailable() {
  return 1;
}

void SerialWrite(uint8_t  *data, int sz) {
  for (int i = 0; i < sz; i++) {
		while(!(USART1->SR & (0x01<<7)));
		USART1->DR = data[i];
		while(!(USART1->SR & (0x01<<6)));
  }
}

void SerialReadBytes(uint8_t  *data, int sz) {
  for (int i = 0; i < sz; i++) {
		while(!(USART1->SR & (0x01<<5)));
    data[i] = USART1->DR;
  }
}

void SerialPrintln(char *data) {
  int idx = 0;
  while (data[idx] != '\0') {
		while(!(USART2->SR & (0x01<<7)));
		USART2->DR = data[idx];
		while(!(USART2->SR & (0x01<<6)));
    idx++;
  }
}

int sendfc3(uint8_t  address, unsigned short start, unsigned short registers, short values[]) {
  if (SerialAvailable()) {
    uint8_t  message[8];
    uint8_t  response[5 + 2 * registers];

    BuildMessage(address, (uint8_t )3, start, registers, message, (sizeof(message) / sizeof(uint8_t )));

    SerialWrite(message, sizeof(message) / sizeof(uint8_t ));
    SerialReadBytes(response, sizeof(response) / sizeof(uint8_t ));

    if (CheckResponse(response, (sizeof(response) / sizeof(uint8_t )))) {
      for (int i = 0; i < (sizeof(response) / sizeof(uint8_t ) - 5) / 2; i++) {
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

void GetCRC(uint8_t  message[], uint8_t  crc[], int message_size) {
  unsigned short CRCFull = 0xFFFF;
  uint8_t  CRCHigh = 0xFF, CRCLow = 0xFF;
  uint8_t CRCLSB;
  for (int i = 0; i < message_size - 2; i++) {
    CRCFull = (unsigned short)(CRCFull ^ message[i]);
    for (int j = 0; j < 8; j++) {
      CRCLSB = (uint8_t)(CRCFull & 0x0001);
      CRCFull = (unsigned short)((CRCFull >> 1) & 0x7FFF);
      if (CRCLSB == 1)
        CRCFull = (unsigned short)(CRCFull ^ 0xA001);
    }
  }
  crc[1] = CRCHigh = (uint8_t )((CRCFull >> 8) & 0xFF);
  crc[0] = CRCLow = (uint8_t )(CRCFull & 0xFF);
}


void BuildMessage(uint8_t  address, uint8_t  type, unsigned short start, unsigned short registers, uint8_t  message[], int message_size) {
  uint8_t  crc[2];
  message[0] = address;
  message[1] = type;
  message[2] = (uint8_t )(start >> 8);
  message[3] = (uint8_t )start;
  message[4] = (uint8_t )(registers >> 8);
  message[5] = (uint8_t )registers;
  GetCRC(message, crc, message_size);
  message[message_size - 2] = crc[0];
  message[message_size - 1] = crc[1];
}



int CheckResponse(uint8_t  response[], int response_size) {
  uint8_t  crc[2];
  GetCRC(response, crc, response_size);
  if (crc[0] == response[response_size - 2] && crc[1] == response[response_size - 1])
    return 1;
  else
    return 0;
}

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
		char te[10];
   res2 = GetTemp();
		sprintf(te,"%lf\r\n",res2);
		SerialPrintln(te);
		Delay(100000);
	//	while(!(USART2->SR & (0x01<<5)));
    //uint8_t  da = USART2->DR;
		//USART2->DR = da;
		//while(!(USART2->SR & (0x01<<6)));
	//	while(!(USART1->SR & (0x01<<5)));
 //   te = USART1->DR;
//		USART2->DR = te;
	//	while(!(USART2->SR & (0x01<<6)));
	}
	
}