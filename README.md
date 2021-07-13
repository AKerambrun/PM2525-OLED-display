# PM2525-OLED-display
unobtainium PM2525 LCD OLED replacement
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

//              D5  ->  pin 16  /CS 

//              D4  ->  pin 15  /RES

//              D3  ->  pin 14  DC

//              D13 ->  pin 4   SCK (CLK)

//              D11 ->  pin 5   Din             

//              GND ->  pin 1   to 13 

//              3.3v ->  pin 2   VCC

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
