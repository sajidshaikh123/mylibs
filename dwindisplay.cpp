#include "dwindisplay.h"

HardwareSerial HMI_Serial(1);

void dwindisplay::begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin)
{
   HMI_Serial.begin(baud,config,rxPin,txPin);
}

int dwindisplay::available()
{
    return(HMI_Serial.available() );
}

uint8_t dwindisplay::read()
{
    return(HMI_Serial.read());   
}

size_t dwindisplay::read(uint8_t *buffer, size_t size)
{
    uint16_t index = 0;
    while(size >0)
    {
        buffer[index++] = (uint8_t)HMI_Serial.read();
        size--;
    } 
    return(index);  
}

size_t dwindisplay::write(uint8_t c)
{
    HMI_Serial.write(c);
    return 1;
}
void dwindisplay::readRTC() {

    HMI_Serial.write(0x5A);
    HMI_Serial.write(0xA5);
    HMI_Serial.write(0x04);
    HMI_Serial.write(0x83);
    HMI_Serial.write(0x00);
    HMI_Serial.write(0x10 );
    HMI_Serial.write(0x04);
    //delay(1);
}

void dwindisplay::Write_Register(uint16_t _address, uint8_t *_Data) {

    HMI_Serial.write(0x5A);
    HMI_Serial.write(0xA5);
    HMI_Serial.write(0x05);
    HMI_Serial.write(0x82);
    HMI_Serial.write(_address >> 8);
    HMI_Serial.write(_address );
    HMI_Serial.write(_Data[0]);
    HMI_Serial.write(_Data[1]);

    //delay(1);
}
// The function hide() hides a part of the display by sending a series of bytes over a serial interface.

// Write 0x5A, 0xA5, 0x05, 0x82, the high byte of the "_SP_Register" argument, 
//the low byte of the "_SP_Register" argument, 0xFF, and 0x00 over the serial interface.
void dwindisplay::hide(uint16_t _SP_Register)
{
    HMI_Serial.write(0x5A);
    HMI_Serial.write(0xA5);
    HMI_Serial.write(0x05);
    HMI_Serial.write(0x82);
    HMI_Serial.write(_SP_Register >> 8);
    HMI_Serial.write(_SP_Register );
    HMI_Serial.write(0xFF);
    HMI_Serial.write(0x00);

}
// The function show() shows a part of the display by sending a series of bytes over a serial interface.

// Write 0x5A, 0xA5, 0x05, 0x82, the high byte of the "_SP_Register" argument, the low byte of the "_SP_Register" argument,
// the high byte of the "_address" argument, and the low byte of the "_address" argument over the serial interface.
void dwindisplay::show(uint16_t _SP_Register, uint16_t _address)
{
    HMI_Serial.write(0x5A);
    HMI_Serial.write(0xA5);
    HMI_Serial.write(0x05);
    HMI_Serial.write(0x82);
    HMI_Serial.write(_SP_Register >> 8);
    HMI_Serial.write(_SP_Register );
    HMI_Serial.write(_address >> 8);
    HMI_Serial.write(_address);
}

void dwindisplay::showPage(uint16_t _page)
{
    HMI_Serial.write(0x5A);
    HMI_Serial.write(0xA5);
    HMI_Serial.write(0x07);
    HMI_Serial.write(0x82);
    HMI_Serial.write(0x00);
    HMI_Serial.write(0x84);
    HMI_Serial.write(0x5A);
    HMI_Serial.write(0x01);
    HMI_Serial.write(_page >> 8);
    HMI_Serial.write(_page );
}

// The function Write_Number() writes a 16-bit integer value to the specified address on the display module by sending a series of bytes over a serial interface.

// Write 0x5A, 0xA5, 0x05, 0x82, the high byte of the "_address" argument, the low byte of the "_address" argument, the high byte of the "_Data" argument, and the low byte of the "_Data" argument over the serial interface.

// Add a delay of 1 millisecond to ensure that the data is completely sent over the serial interface.

void dwindisplay::Write_Number(uint16_t _address, uint16_t _Data) {

    HMI_Serial.write(0x5A);
    HMI_Serial.write(0xA5);
    HMI_Serial.write(0x05);
    HMI_Serial.write(0x82);
    HMI_Serial.write(_address >> 8);
    HMI_Serial.write(_address );
    HMI_Serial.write(_Data >> 8);
    HMI_Serial.write(_Data );

    //delay(1);
}

void dwindisplay::Write_Register_Long(uint16_t _address, uint8_t *_Data, uint8_t _Size) {

// Declare Length
uint8_t _Pack_Size = 3;

// Set Pack Header
HMI_Serial.write(0x5A);
HMI_Serial.write(0xA5);
HMI_Serial.write(_Size + _Pack_Size);
HMI_Serial.write(0x82);
HMI_Serial.write(_address >> 8);
HMI_Serial.write(_address );

// Send Data Pack
for (size_t i = 0; i < _Size; i++) HMI_Serial.write(_Data[i]);

// Command Delay
delay(1);
}

// Function to set the text color for the display
void dwindisplay::setTextColor(uint16_t _address, uint16_t color) 
{
    // Write data to the specified address
    uint8_t _data[2];
    _data[0] = (color >> 8);
    _data[1] =  color;
    this->Write_Register(_address + 0x03, _data);
}

// The function Write_String() writes a null-terminated string of characters to the specified address on the display module by calling the Write_Register_Long() function.

// Call the Write_Register_Long() function with the "_address" argument, a pointer to the "_Data" character array that is cast to a pointer of uint8_t type, and the length of the "_Data" array obtained using the strlen() function. This writes the string of characters to the specified address on the display module

void dwindisplay::Write_String(uint16_t _address, String  _Data) {    
    this->Write_Register_Long(_address,(uint8_t *)_Data.c_str(),_Data.length()+1);
}

void dwindisplay::message(uint16_t _address, String  _Data) {

    String temp_str="";
    for(int i = 0;i < last_len ; i++){
            temp_str +=" ";
    }
    this->Write_Register_Long(_address,(uint8_t *)temp_str.c_str(),temp_str.length()+1);
    this->Write_Register_Long(_address,(uint8_t *)_Data.c_str(),_Data.length()+1);
    last_len = _Data.length();
}


void dwindisplay::Write_UString(uint16_t _address, String  _Data) {

    uint8_t _Pack_Size = 3;
	uint16_t _Size = _Data.length() + 1;

	// Set Pack Header
	HMI_Serial.write(0x5A);
	HMI_Serial.write(0xA5);
	HMI_Serial.write((_Size * 2) + _Pack_Size);
	HMI_Serial.write(0x82);
	HMI_Serial.write(_address >> 8);
	HMI_Serial.write(_address );

	// Send Data Pack
	for (size_t i = 0; i < _Size; i++) {
		HMI_Serial.write(0x00);
		HMI_Serial.write(_Data[i]);
	}
	// Command Delay
	delay(1);

}
void dwindisplay::touchevent(uint16_t _page_no, uint8_t _control_no, uint8_t _control_type,uint16_t _mode)
{
    delay(20);
    HMI_Serial.write(0x5A);
    HMI_Serial.write(0xA5);
    HMI_Serial.write(0x0B);
    HMI_Serial.write(0x82);
    HMI_Serial.write(0x00);
    HMI_Serial.write(0xB0);
    HMI_Serial.write(0x5A);
    HMI_Serial.write(0xA5);
    HMI_Serial.write(_page_no >> 8);
    HMI_Serial.write(_page_no );
    HMI_Serial.write(_control_no );
    HMI_Serial.write(_control_type );
    HMI_Serial.write(_mode >> 8);
    HMI_Serial.write(_mode );

    delay(20);
}
// This function performs a reset of the display module.

// Create a Register object "Reset_Register" with the address 0x00 and 0x04.
// Declare an array "reset_data" with four elements and initialize it with the values { 0x55, 0xAA, 0x5A, 0xA5 }.
// Call the member function "Write_Register_Long" and pass the "Reset_Register" object, "reset_data" array, and the size of "reset_data" as arguments.

void dwindisplay::reset()
{
        uint16_t Reset_Register = { 0x0004 };
        uint8_t reset_data[4] = { 0x55, 0xAA, 0x5A, 0xA5 };
        this->Write_Register_Long(Reset_Register, reset_data, sizeof(reset_data)); 
}

// This function sets the date and time on the display module.

// Create a Register object "Time_Stamp_Register" with the address 0x00 and 0x9C.
// Declare an array "Data" with eight elements to store the date and time information.
// Set the first two elements of "Data" to 0x5A and 0xA5 respectively.
// Store the year, month, day, hour, minute, and second in Data[2], Data[3], Data[4], Data[5], Data[6], and Data[7] respectively.
// Call the member function "Write_Register_Long" and pass the "Time_Stamp_Register" object, "Data" array, and the size of "Data" as arguments.

void dwindisplay::Time_Stamp(uint8_t _Day, uint8_t _Month, uint8_t _Year, uint8_t _Hour, uint8_t _Minute, uint8_t _Second) {

    // Declare Default Data Array
    uint16_t Time_Stamp_Register = {0x009C}; // RTC Setting Address
    uint8_t Data[8];

    // Set Array Values
    Data[0] = 0x5A;     //Start RTC setting once.
    Data[1] = 0xA5;     // 
    Data[2] = _Year;
    Data[3] = _Month;
    Data[4] = _Day;
    Data[5] = _Hour;
    Data[6] = _Minute;
    Data[7] = _Second;

    // Write Data
    this->Write_Register_Long(Time_Stamp_Register, Data, sizeof(Data) );
}

void dwindisplay::Sleep(bool _Status,uint8_t low_brightness , uint16_t sleep_after ) {

    uint16_t Sleep_Register = {0x0082};
    // Declare Variables
    uint8_t Data[4];

    // Data[0] - ON State Brightnes
    Data[0] = 0x64;

    // Data[1] - Sleep State Brightnes
    if (_Status) Data[1] = low_brightness;
    if (!_Status) Data[1] = 0x64;
    
    // Data[2] - Sleep Time
    Data[2] = (sleep_after >> 8);

    // Data[3] - Sleep Time
    Data[3] = (sleep_after );

    // Write Data
    this->Write_Register_Long(Sleep_Register, Data, 4);
}

// This function turns on the buzzer for the specified amount of time.
// The input parameter "onTime" represents the duration in milliseconds for which the buzzer should be on.

// Create a Register object "buzzer" with the address 0x00 and 0xA0.
// Declare an array "Data" with two elements to store the high and low bytes of "onTime".
// Store the high byte of "onTime" in Data[0] and the low byte in Data[1].
// Call the member function "Write_Register" and pass the "buzzer" object and "Data" array as arguments.

void dwindisplay::buzzerOn(uint16_t onTime)
{
    uint16_t buzzer = { 0x00A0 };
    uint8_t Data[2];
    Data[0] = (onTime >> 8);
    Data[1] = (onTime );

    this->Write_Register(buzzer, Data);  
}


