
#ifndef dwindisplay_h
#define dwindisplay_h

#ifndef __Arduino__
	#include <Arduino.h>
#endif

#include <HardwareSerial.h>

//HardwareSerial HMI_Serial(1);

#define VERIABLE_DATA_INPUT     0x00
#define POPUP_MENU              0x01
#define INCREMENT_ADJUSTMENT    0x02
#define DRAG_ADJUSTMENT         0x03
#define RETURN_KEY_CODE         0x05
#define ASCII_TEXT_INPUT        0x06
#define SYNCHRODATA_RETURN      0x08
#define ROTATION_ADJUSTMENT     0x09
#define SLIDING_ADGUSTMENT      0x0A
#define PAGE_SLIDING            0x0B
#define SLIDE_ICON_SET          0x0C
#define BIT_BUTTON              0x0D

#define ENABLE                  0x0001
#define DISABLE                 0x0000

struct Register {
    const uint8_t High_Address;
    const uint8_t Low_Address;
};

class dwindisplay
{
    private:  /* Private data / methods*/
    //Stream* HMI_Serial; 
    bool hmi_enable = false;
        uint8_t last_len =0;
    public:
        void begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin);
        void Write_Register(uint16_t _Register, uint8_t *_Data);
        void Write_Number(uint16_t _Register, uint16_t _Data);
        void Write_Register_Long(uint16_t _Register, uint8_t *_Data, uint8_t _Size);  
        void touchevent(uint16_t _page_no, uint8_t _control_no, uint8_t _control_type,uint16_t _mode); 
        void Write_String(uint16_t _address, String  _Data);
        void message(uint16_t _address, String  _Data);
		void Write_UString(uint16_t _address, String  _Data);
        void reset();
        void Sleep(bool _Status,uint8_t low_brightness = 0x05, uint16_t sleep_after = 60000);
        void buzzerOn(uint16_t onTime);
        void Time_Stamp(uint8_t _Day, uint8_t _Month, uint8_t _Year, uint8_t _Hour, uint8_t _Minute, uint8_t _Second);
        void hide(uint16_t _SP_Register);
        void show(uint16_t _SP_Register, uint16_t _address);
        int available();
		void readRTC();
        uint8_t read();
        void showPage(uint16_t _page);
        size_t read(uint8_t *buffer, size_t size);
        inline size_t read(char * buffer, size_t size)
        {
            return read((uint8_t*) buffer, size);
        }
        size_t write(uint8_t c);
        void setTextColor(uint16_t _Register,uint16_t color);

};

#endif