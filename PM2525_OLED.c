// *********************************************************************************************************************************************//
//
// This program converts the I2C display data of the Fluke/Philips PM2525 multimeter in case the original unobtainium LCD is gone for good 
// to SPI data for a modern OLED display. 
//
// It's meant to use a 3,2 inch OLED. Those can be found on Aliexpress for about 17 bucks and exist in different colors.
// the bus I use here is SPI so be sure you move the jumper resistor from R5 to R6 to set it up to SPI mode ( it can do 4 different modes parallel
// and serial, 2 of each)
// 
// It runs fine on an Arduino Nano making the multimeter fully usable. the display update is a bit slow though. 
// I Haven't determined if it comes from the processing or the SPI interface.
//
// LCD wiring :
//            
//            
//              D5  ->  pin 16  /CS 
//              D4  ->  pin 15  /RES
//              D3  ->  pin 14  DC
//              D13 ->  pin 4   SCK (CLK)
//              D11 ->  pin 5   Din             
//              GND ->  pin 1   to 13 
//              3.3v ->  pin 2   VCC
//
// as the arduino are 5V and the display is 3.3V please add 330 ohms resistors in series withe the 5 signals.
// the I2C side is straight forward. A4 is SDA and A5 is SCL to the PM2525 bus
// an explanation of the data transfer format can found here : https://www.maximintegrated.com/en/design/technical-documents/app-notes/6/6315.html
// the decoding table sits in the PM2525 or PM2535 service manual.
// 
// modes and symbols are handled (as far as I know) and displayed the same way and layout on the OLED display. All segments are decoded except 
// the battery symbol I don't use on my meter and the Y3 segment as I don't know it's purpose. They should be easy to add anyways.
//
// Hope it helps. I think it can be useful in many applications involving vintage equipment.
// Please note that I'm an electronics engineer not a programmer so my code might not be written in a 'canonical' way :-)


#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

    uint8_t frame[20]= {255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255} ; // buffer for i2c received data  
 
    char value[8]={0,0,0,0,0,0,0,0};  // value to display 

// ************************* glyph definitions ***********************

   // Ohms glyph
    #define OMEGA_width 14
    #define OMEGA_height 16
    static unsigned char OMEGA_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01,
    0xf8, 0x07, 0x1c, 0x0e, 0x0c, 0x0c, 0x06, 0x18,
    0x06, 0x18, 0x06, 0x18, 0x06, 0x18, 0x0c, 0x0c,
    0x18, 0x06, 0x18, 0x06, 0x1e, 0x1e, 0x1e, 0x1e};
    
   // zap glyph
    #define ZAP_width 12
    #define ZAP_height 9
    static unsigned char ZAP_bits[] = {
    0x00, 0x08, 0x00, 0x0c, 0x20, 0x06, 0x70, 0x03,
    0xd8, 0x01, 0x8d, 0x00, 0x07, 0x00, 0x07, 0x00,
    0x0f, 0x00}; 
     
 // S glyph
    #define S_width 5
    #define S_height 7
    static unsigned char S_bits[] = {
    0x0e, 0x01, 0x01, 0x0e, 0x10, 0x10, 0x0e};

 // up arrow 2 ( on the top left corner
    #define UPARROW2_width 7
    #define UPARROW2_height 7
    static unsigned char UPARROW2_bits[] = {
    0x08, 0x1c, 0x2a, 0x49, 0x08, 0x08, 0x08};
     
 // decimal dot  
    #define DP_width 2
    #define DP_height 2
    static unsigned char DP_bits[] = {
    0x03, 0x03};
    
 // loudspeaker glyph 
    #define lsp_width 8
    #define lsp_height 7
    static unsigned char lsp_bits[] = {
    0x60, 0x50, 0x4e, 0x46, 0x4e, 0x50, 0x60};

    
  // diode glyph
    #define diode_width 8
    #define diode_height 7
    static unsigned char diode_bits[] = {
    0x00, 0x24, 0x34, 0xff, 0x34, 0x24, 0x00};

    // DC glyph
    #define DC_width 8
    #define DC_height 6
    static unsigned char DC_bits[] = {
    0xff, 0x00, 0x00, 0xdb, 0x00, 0x00};
    
   // AC glyph
    #define AC_width 7
    #define AC_height 3
    static unsigned char AC_bits[] = {
    0x06, 0x49, 0x30};

   // DC glygh on to of the AC symbol
    #define DCAC_width 7
    #define DCAC_height 1
    static unsigned char DCAC_bits[] = {
    0x7f};

   // DOWN ARROW on upper right
    #define DNARROW_width 5
    #define DNARROW_height 3
    static unsigned char DNARROW_bits[] = {
    0x11, 0x0a, 0x04};

   // UP ARROW on the upper right
    #define UPARROW_width 5
    #define UPARROW_height 3
    static unsigned char UPARROW_bits[] = {
    0x04, 0x0a, 0x11};   

   // Z glyph
    #define Z_width 9
    #define Z_height 11
    static unsigned char Z_bits[] = {
    0x00, 0x00, 0xfc, 0x00, 0x80, 0x00, 0x40, 0x00,
    0x20, 0x00, 0x10, 0x00, 0x08, 0x00, 0x04, 0x00,
    0x02, 0x00, 0x7e, 0x00, 0x00, 0x00};

   // function arrows at the bottom line (won't line up with the bezel though
    #define FCTARROW_width 5
    #define FCTARROW_height 6
    static unsigned char FCTARROW_bits[] = {
    0x04, 0x04, 0x04, 0x1f, 0x0e, 0x04};

// construtor usage U8G2_SSD1322_NHD_256X64_1_4W_HW_SPI(rotation, cs, dc [, reset]) [page buffer, size = 256 bytes]
    U8G2_SSD1322_NHD_256X64_1_4W_HW_SPI u8g2(U8G2_R2, 5, 3, 4); // OLED init


void setup(void) {
 u8g2.begin();
//Serial.begin(9600);           // start serial interface for debugging purposes only .comment out in real life 
  
  Wire.begin(0x38);                // i2c bus slave address #38 (defaut address of the PCF 8576) A4 is SDA A5 is SCL
  Wire.onReceive(receiveEvent); // register event
 
}

// the host drops a data frame every 30ms triggering an event . the received data are stored in frame[] in the right order
// then decoded to be displayed
// 
void loop(void) {               // the host drops a data frame every 30ms. the received data are stored in frame[] in the right order
  
  u8g2.firstPage();
  do {
    int yfirtsline = 10;
    char StrDisp[10];
    
    u8g2.setFont(u8g2_font_tom_thumb_4x6_tr);   // font for the functions display 

    if (frame[16]&0x02){u8g2.drawStr(0,yfirtsline,"REM" );}
    if (frame[16]&0x20){u8g2.drawStr(16,yfirtsline,"SRQ" );}
    if (frame[17]&0x02){u8g2.drawStr(32,yfirtsline,"LSTN" );}
    if (frame[18]&0x20){u8g2.drawStr(52,yfirtsline,"TLK" );}
    if (frame[18]&0x02){u8g2.drawStr(68,yfirtsline,"ONLY" );}
    
    /// todo
    if (frame[19]&0x20){u8g2.drawStr(88,yfirtsline,"|");} //Z1 bargraph 
    if (frame[8]&0x02){u8g2.drawStr(88+4,yfirtsline,".");} //2
    if (frame[8]&0x01){u8g2.drawStr(88+8,yfirtsline,".");}//3
    if (frame[8]&0x04){u8g2.drawStr(88+12,yfirtsline,".");} //4
    if (frame[8]&0x08){u8g2.drawStr(88+16,yfirtsline,".");} //5
    if (frame[8]&0x80){u8g2.drawStr(88+20,yfirtsline,".");} //6
    if (frame[8]&0x40){u8g2.drawStr(88+24,yfirtsline,".");}//7
    if (frame[8]&0x10){u8g2.drawStr(88+28,yfirtsline,".");}//8
    if (frame[8]&0x20){u8g2.drawStr(88+32,yfirtsline,".");}//9
    if (frame[7]&0x02){u8g2.drawStr(88+36,yfirtsline,".");}//10
    if (frame[7]&0x01){u8g2.drawStr(88+40,yfirtsline,".");}//11
    if (frame[7]&0x04){u8g2.drawStr(88+44,yfirtsline,".");}//12
    if (frame[7]&0x08){u8g2.drawStr(88+48,yfirtsline,".");}//13
    if (frame[7]&0x80){u8g2.drawStr(88+52,yfirtsline,".");}//14
    if (frame[7]&0x40){u8g2.drawStr(88+56,yfirtsline,".");}//15
    if (frame[7]&0x10){u8g2.drawStr(88+60,yfirtsline,".");}//16
    if (frame[7]&0x20){u8g2.drawStr(88+64,yfirtsline,"|");}//17
//    
    if (frame[2]&0x10){u8g2.drawStr(172,yfirtsline+10,"2w" );}
    if (frame[1]&0x04){u8g2.drawStr(180,yfirtsline+10,"4w" );}
    if (frame[2]&0x40){u8g2.drawStr(180,yfirtsline+20,"HF" );}
    
    if (frame[1]&0x01){strcpy (StrDisp,"SHFT");
    DrawInvStr ( 172,yfirtsline,StrDisp, strlen(StrDisp));}   
    if (frame[1]&0x20){strcpy (StrDisp,"LIM");
    DrawInvStr ( 192,yfirtsline,StrDisp, strlen(StrDisp));}
    if (frame[0]&0x02){strcpy (StrDisp,"DELTA%");
    DrawInvStr ( 208,yfirtsline,StrDisp, strlen(StrDisp));}
    if (frame[1]&0x40){strcpy (StrDisp,"CAL");
    DrawInvStr ( 192,yfirtsline+10,StrDisp, strlen(StrDisp));}
    if (frame[0]&0x01){strcpy (StrDisp,"AX+B");
    DrawInvStr ( 212,yfirtsline+10,StrDisp, strlen(StrDisp));}
    if (frame[1]&0x40){strcpy (StrDisp,"MIN");
    DrawInvStr ( 192,yfirtsline+20,StrDisp, strlen(StrDisp));}
    if (frame[0]&0x04){strcpy (StrDisp,"MAX");
    DrawInvStr ( 212,yfirtsline+20,StrDisp, strlen(StrDisp));}
    if (frame[1]&0x80){strcpy (StrDisp,"READ");
    DrawInvStr ( 192,yfirtsline+30,StrDisp, strlen(StrDisp));}
    if (frame[0]&0x08){strcpy (StrDisp,"BURST");
    DrawInvStr ( 212,yfirtsline+30,StrDisp, strlen(StrDisp));}
    if (frame[0]&0x10){strcpy (StrDisp,"SEQU");
    DrawInvStr ( 192,yfirtsline+40,StrDisp, strlen(StrDisp));}
    if (frame[0]&0x20){strcpy (StrDisp,"DELAY");
    DrawInvStr ( 212,yfirtsline+40,StrDisp, strlen(StrDisp));}
    if (frame[17]&0x10){strcpy (StrDisp,"ZERO");
    DrawInvStr ( 192,yfirtsline+50,StrDisp, strlen(StrDisp));}
    if (frame[17]&0x20){strcpy (StrDisp,"SET");
    DrawInvStr ( 212,yfirtsline+50,StrDisp, strlen(StrDisp));}

    if (frame[1]&0x02){u8g2.drawXBM( 160, yfirtsline-6, lsp_width, lsp_height, lsp_bits);}
    if (frame[2]&0x20){u8g2.drawXBM( 160, yfirtsline+4, diode_width, diode_height, diode_bits);}
    if (frame[0]&0x40){u8g2.drawXBM( 180, yfirtsline+29, AC_width, AC_height, AC_bits);}
    if (frame[0]&0x80){u8g2.drawXBM( 180, yfirtsline+36, DC_width, DC_height, DC_bits);}
    if (frame[2]&0x80){u8g2.drawXBM( 180, yfirtsline+26, DCAC_width, DCAC_height, DCAC_bits);}
    if (frame[2]&0x01){u8g2.drawXBM( 172, yfirtsline+18, DNARROW_width, DNARROW_height, DNARROW_bits);}
    if (frame[2]&0x02){u8g2.drawXBM( 172, yfirtsline+13, UPARROW_width, UPARROW_height, UPARROW_bits);}
    if (frame[2]&0x04){u8g2.drawXBM( 168, yfirtsline+26, Z_width, Z_height, Z_bits);}

    // bottom line
    if (frame[17]&0x01){u8g2.drawStr(0,yfirtsline+50,"M RNG" );}
    if (frame[17]&0x04){u8g2.drawStr(24,yfirtsline+50,"S TRG" );}
    if (frame[17]&0x08){u8g2.drawXBM( 45, yfirtsline+44, FCTARROW_width, FCTARROW_height, FCTARROW_bits);}
    if (frame[18]&0x80){u8g2.drawStr(52,yfirtsline+50,"SPEED" );}
    if (frame[18]&0x40){u8g2.drawStr(72,yfirtsline+50,"1" );} 
    if (frame[18]&0x10){u8g2.drawStr(76,yfirtsline+50,"2" );}  
    if (frame[18]&0x01){u8g2.drawStr(80,yfirtsline+50,"3" );}  
    if (frame[18]&0x04){u8g2.drawStr(84,yfirtsline+50,"4" );}  
    if (frame[18]&0x08){u8g2.drawXBM( 90, yfirtsline+44, FCTARROW_width, FCTARROW_height, FCTARROW_bits);}
    if (frame[19]&0x80){u8g2.drawStr(97,yfirtsline+50,"FILT" );} 
    if (frame[19]&0x40){u8g2.drawStr(117,yfirtsline+50,"NULL" );}  
    if (frame[19]&0x10){u8g2.drawXBM( 133, yfirtsline+44, FCTARROW_width, FCTARROW_height, FCTARROW_bits);}
    if (frame[19]&0x01){u8g2.drawStr(140,yfirtsline+50,"HOLD" );}  
    if (frame[19]&0x04){u8g2.drawStr(160,yfirtsline+50,"PROBE" );}  
    if (frame[19]&0x08){u8g2.drawXBM( 182, yfirtsline+44, FCTARROW_width, FCTARROW_height, FCTARROW_bits);}
    if (frame[16]&0x01){u8g2.drawXBM( 0, yfirtsline+6, ZAP_width, ZAP_height, ZAP_bits);}
    if (frame[16]&0x10){u8g2.drawXBM( 18, yfirtsline+6, UPARROW2_width, UPARROW2_height, UPARROW2_bits);}
    if (frame[19]&0x02){u8g2.drawXBM(146, yfirtsline+6, S_width, S_height, S_bits);}

    // dots
    if (frame[16]&0x80){u8g2.drawXBM(20, yfirtsline+34, DP_width, DP_height, DP_bits);}
    if (frame[14]&0x08){u8g2.drawXBM(20+17, yfirtsline+34, DP_width, DP_height, DP_bits);}
    if (frame[13]&0x08){u8g2.drawXBM(20+(2*17), yfirtsline+34, DP_width, DP_height, DP_bits);}
    if (frame[12]&0x08){u8g2.drawXBM(20+(3*17), yfirtsline+34, DP_width, DP_height, DP_bits);}
    if (frame[11]&0x08){u8g2.drawXBM(20+(4*17), yfirtsline+34, DP_width, DP_height, DP_bits);}
    if (frame[10]&0x08){u8g2.drawXBM(20+(5*17), yfirtsline+34, DP_width, DP_height, DP_bits);}
    if (frame[9]&0x08){u8g2.drawXBM(20+(6*17), yfirtsline+34, DP_width, DP_height, DP_bits);}
    // if (frame[2]&0x08){u8g2.drawXBM(20+12+(7*17), yfirtsline+34, DP_width, DP_height, DP_bits); // originally used to draw the legs
    //                   u8g2.drawXBM(20+26+(7*17), yfirtsline+34, DP_width, DP_height, DP_bits);} // of the omega symbol useless here
        
    // second line actual value display + units

    u8g2.setFont(u8g2_font_inr19_mf); // font for  value display 
    if (frame[16]&0x04){u8g2.drawStr(4,yfirtsline+36,"+" );} // polarity sign
    else {u8g2.drawStr(4,yfirtsline+36,"-");}
    
    int j =0;

    for (int i=15; i>8 ; i--){
      value[j++] = ( sevenSeg2char(frame[i]&0xf7));
    }
    u8g2.drawStr(21,yfirtsline+36,value);
    

    // units prefix
    u8g2.setFont(u8g2_font_inr16_mf); // slightly smaller font for the units

        j = 0x00 + frame[5];
        j = j << 8;
        j += frame[6];

        switch(j)
        {
        case 0x2288:
          value[0] = '/'; // falling and raising edge
          break;
        case 0x8282:
          value[0] = 0x5c;  // '\'
          break;
        case 0x0145:
          value[0] = 'V';
          break;
        case 0xc080:
          value[0] = 0xb5; // micro
          break;
        case 0x4480:
          value[0] = 'n';
          break;
        case 0xD480:
          value[0] = 'd';
          break;
        case 0x0E80:
          value[0] = 'k';
          break;
        case 0x5125:
          value[0] = 'M';
          break;
        case 0x4494:
          value[0] = 'm';
          break;
        case 0x3600:
          value[0] = 0xB0; // '°'
          break;
        case 0xC7D3:
          value[0] = '%';
          break;
          default:
          value[0] = ' ';
        }

      // prefix
      
        j = 0x00 + frame[3];
        j = j << 8;
        j += frame[4];

        switch(j)
        {
        case 0x2288:
          value[1] = '/';
          break;
        case 0x8282:
          value[1] = 0x5c;  // '\'
          break;
        case 0x217:
          value[1] = 'P';
          break;
        case 0x5415:
          value[1] = 'H';
          break;
        case 0x2017:
          value[1] = 'F';
          break;
        case 0xF68A:
          value[1] = 'B';
          break;
        case 0x0145:
          value[1] = 'V';
          break;
        case 0x7417:
          value[1] = 'A';
          break;
        case 0x7007:      // The omega symbol doesn't exist in the inconsolata font so let's make our own
          value[1] = ' '; // ohms
          u8g2.drawXBM( 152, yfirtsline+20, OMEGA_width, OMEGA_height, OMEGA_bits);
          break;
        case 0xA00F:
          value[1] = 'C';
          break;
        default:
          value[1] = ' ';
        }

        value[2] = 0; // null terminated
          u8g2.drawStr(138,yfirtsline+36,value );

          
  } while ( u8g2.nextPage() );
}
// draw black on white text

void DrawInvStr ( int xpos , int ypos , char DispStr[], int lgth ) {
    //Serial.print( lgth);
    u8g2.setFontMode(1);  /* activate transparent font mode */// (sizeof (DispStr)*4)+2
    u8g2.setDrawColor(1); /* color 1 for the box */
    u8g2.drawBox( xpos-2, ypos-6,((lgth*4)+2), 7);
    u8g2.setFont( u8g2_font_tom_thumb_4x6_tr);
    u8g2.setDrawColor(0);
    u8g2.drawStr( xpos,ypos,DispStr );
    u8g2.setDrawColor(1);
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
// the total of data bytes is 20 ( 40 nibbles) are stored in the right order in the frame[] array the 

void receiveEvent(int count)  // count should be 22 bytes for the first chunk and 8 for the second. total transmission time 3.5 ms
{
  uint8_t idx = 0;                    // index for storing frames 
  uint8_t data;

  while( Wire.available()) // store all the bytes into the buffer 
  {
    data = Wire.read();
 //   Serial.print(data,HEX);
 
    if (idx > 4)                          // let's drop the 5 first commands
    {
       if ( count == 22)               // we onlty need 17 bytes (34 nibbles)  of data 
       {
             frame[idx-2] = data;          // get byte in buffer and store in an array from the index 3 
       }

       
       if ( count == 8)                // same here we need 3 bytes ( 6 nibbles)
       {
             frame[idx-5] = data;         // get byte in buffer and store in an array 
        } 
    }
       idx++;        
   }
}


// seven segments to char conversion
char sevenSeg2char(uint8_t input)
{
  switch(input)
  {
  case 0x00:
    return ' ';
  case 0x01:
    return '-';
  case 0x02:
    return '\'';
  case 0x05:
    return 'R';
  case 0x08:
    return '.';
  case 0x12:
    return '\"';
  case 0x27:
    return 'F';
  case 0x35:
    return '?';
  case 0x37:
    return 'P';
  case 0x50:
    return '1';
  case 0x53:
    return '4';
  case 0x57:
    return 'H';
  case 0x70:
    return '7';
  case 0x73:
    return 'Q';
  case 0x76:
    return 'N';
  case 0x77:
    return 'A';
  case 0x80:
    return '_';
  case 0x81:
    return '=';
  case 0x86:
    return 'L';
  case 0x87:
    return 'T';
  case 0xA6:
    return 'C';
  case 0xA7:
    return 'E';
  case 0xB5:
    return '2';
  case 0xC7:
    return 'B';
  case 0xD3:
    return 'Y';
  case 0xD5:
    return 'D';
  case 0xD6:
    return 'U';
  case 0xE3:
    return '5';
  case 0xE6:
    return 'G';
  case 0xE7:
    return '6';
  case 0xF0:
    return ']';
  case 0xF1:
    return '3';
  case 0xF3:
    return '9';
  case 0xF4:
    return 'J';
  case 0xF5:
    return '@';
  case 0xF6:
    return '0';
  case 0xF7:
    return '8';
  default:
    return '?';
  }
}

