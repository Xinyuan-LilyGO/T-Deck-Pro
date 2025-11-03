#include <SPI.h>
//EPD
#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"
#include "Ap_29demo.h"  

#define BOARD_LORA_CS   3
#define BOARD_LORA_RST  4
#define BOARD_SD_CS   48
#define BOARD_EPD_CS   34

#define BOARD_EPD_SCK  36
#define BOARD_EPD_MOSI 33

void setup() {

  // LORA、SD、EPD use the same SPI, in order to avoid mutual influence;
  // before powering on, all CS signals should be pulled high and in an unselected state;
  pinMode(BOARD_LORA_CS, OUTPUT); 
  digitalWrite(BOARD_LORA_CS, HIGH);
  pinMode(BOARD_LORA_RST, OUTPUT); 
  digitalWrite(BOARD_LORA_RST, HIGH);
  pinMode(BOARD_SD_CS, OUTPUT); 
  digitalWrite(BOARD_SD_CS, HIGH);
  pinMode(BOARD_EPD_CS, OUTPUT); 
  digitalWrite(BOARD_EPD_CS, HIGH);


#ifdef ESP8266
   pinMode(D0, INPUT);  //BUSY
   pinMode(D1, OUTPUT); //RES 
   pinMode(D2, OUTPUT); //DC   
   pinMode(D4, OUTPUT); //CS     
#endif 
#ifdef Arduino_UNO
   pinMode(BOARD_EPD_BUSY, INPUT);  //BUSY
   pinMode(BOARD_EPD_RST, OUTPUT); //RES 
   pinMode(BOARD_EPD_DC, OUTPUT); //DC   
   pinMode(BOARD_EPD_CS, OUTPUT); //CS   
#endif 
   //SPI
   SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0)); 
   SPI.begin(BOARD_EPD_SCK, -1, BOARD_EPD_MOSI, BOARD_EPD_CS);
}

//Tips//
/*
1.Flickering is normal when EPD is performing a full screen update to clear ghosting from the previous image so to ensure better clarity and legibility for the new image.
2.There will be no flicker when EPD performs a partial refresh.
3.Please make sue that EPD enters sleep mode when refresh is completed and always leave the sleep mode command. Otherwise, this may result in a reduced lifespan of EPD.
4.Please refrain from inserting EPD to the FPC socket or unplugging it when the MCU is being powered to prevent potential damage.)
5.Re-initialization is required for every full screen update.
6.When porting the program, set the BUSY pin to input mode and other pins to output mode.
*/
void loop() {
   unsigned char i;
#if 1 //Full screen refresh, fast refresh, and partial refresh demostration.

      EPD_Init(); //Full screen refresh initialization.
      EPD_WhiteScreen_White(); //Clear screen function.
      EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
      delay(2000); //Delay for 2s. 
     /************Full display(2s)*******************/
      EPD_Init(); //Full screen refresh initialization.
      EPD_WhiteScreen_ALL(gImage_1); //To Display one image using full screen refresh.
      EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
      delay(2000); //Delay for 2s.
            
      /************Fast refresh mode(1.5s)*******************/
      EPD_Init_Fast(); //Fast refresh initialization.
      EPD_WhiteScreen_ALL(gImage_1); //To display one image using fast refresh.
      EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
      delay(2000); //Delay for 2s.

  #if 1 //Partial refresh demostration.
  //Partial refresh demo support displaying a clock at 5 locations with 00:00.  If you need to perform partial refresh more than 5 locations, please use the feature of using partial refresh at the full screen demo.
  //After 5 partial refreshes, implement a full screen refresh to clear the ghosting caused by partial refreshes.
  //////////////////////Partial refresh time demo/////////////////////////////////////
      EPD_Init(); //Electronic paper initialization.  
      EPD_SetRAMValue_BaseMap(gImage_1); //Please do not delete the background color function, otherwise it will cause unstable display during partial refresh. 
      for(i=0;i<6;i++)
      {
        EPD_Dis_Part_Time(40,150,Num[1],Num[0],gImage_numdot,Num[0],Num[i],5,64,32); //x,y,DATA-A~E,number,Resolution 32*64         
      }        
      EPD_DeepSleep();  //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
      delay(2000); //Delay for 2s.
        
      EPD_Init(); //Full screen refresh initialization.
      EPD_WhiteScreen_White(); //Clear screen function.
      EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
      delay(2000); //Delay for 2s.
  #endif  
  
  #if 0    //Demo of using partial refresh to update the full screen, to enable this feature, please change 0 to 1.
  //After 5 partial refreshes, implement a full screen refresh to clear the ghosting caused by partial refreshes.
  //////////////////////Partial refresh time demo/////////////////////////////////////
      EPD_Init(); //Full screen refresh initialization.
      EPD_WhiteScreen_White(); //Clear screen function.   
      EPD_Dis_PartAll(gImage_1);
      EPD_DeepSleep();//Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
      delay(2000); //Delay for 2s. 
      
      EPD_Init(); //Full screen refresh initialization.
      EPD_WhiteScreen_White(); //Clear screen function.
      EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
      delay(2000); //Delay for 2s.
  #endif
  
  #if 0 //Demonstration of full screen refresh with 180-degree rotation, to enable this feature, please change 0 to 1.
      /************Full display(2s)*******************/
      EPD_Init_180(); //Full screen refresh initialization.
      EPD_WhiteScreen_ALL(gImage_1); //To Display one image using full screen refresh.
      EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
      delay(2000); //Delay for 2s.
  #endif        
  
#endif

#ifdef ESP8266
  while(1) 
    {
     Sys_run();//System run
     LED_run();//Breathing lamp
    }
#endif
#ifdef Arduino_UNO
 while(1);  // The program stops here   
#endif
}




//////////////////////////////////END//////////////////////////////////////////////////
