
#ifndef RTC_OPERATION_H
#define RTC_OPERATION_H

#include "pindefinition.h"
#include <DS3231.h>
#include <ESP32Time.h>


static DS3231 RTC;

static ESP32Time internalrtc(0); 
static String result = "";

static bool Century = false;
static bool h12;
static bool PM;
static bool RTC_OK = 0;


inline bool RTCInit(uint8_t scl = SCL_PIN, uint8_t sda = SDA_PIN){

      Wire.begin(sda, scl);

      // if (!RTC.begin()) {
      //   Serial.println("Couldn't find RTC");
      // }
      if(RTC.getSecond() < 60){
            RTC_OK = true;
            Serial.println("RTC OK");
      }else{
            RTC_OK = false;
            Serial.println("RTC Error");
      }
      return RTC_OK;
}

inline const char * getDateTime(){
    result = "";
    if(RTC_OK){
        uint16_t year = RTC.getYear() + 2000;
        uint8_t month = RTC.getMonth(Century);
        uint8_t date = RTC.getDate();

        uint8_t hours = RTC.getHour(h12, PM);
        uint8_t minutes = RTC.getMinute();
        uint8_t seconds = RTC.getSecond();

        result = String(year) + "-";

        if(month < 10){
          result += "0";  
        }
        result +=  String(month) + "-";  

        if(date < 10){
          result += "0";  
        }
        result +=  String(date) + " ";  

        if(hours < 10){
          result += "0";  
        }
        result +=  String(hours) + ":";  

        if(minutes < 10){
          result += "0";  
        }
        result +=  String(minutes) + ":";

        // if(seconds > 10){
        //     seconds = seconds - 10;
        // }else{
        //     seconds = 0;
        // }
        
        if(seconds < 10){
          result += "0";  
        }
        result +=  String(seconds);

    }else{
        uint16_t year = internalrtc.getYear() ;
        uint8_t month = internalrtc.getMonth() + 1;
        uint8_t date = internalrtc.getDay();

        uint8_t hours = internalrtc.getHour(true);
        uint8_t minutes = internalrtc.getMinute();
        uint8_t seconds = internalrtc.getSecond();

       

        result = String(year) + "-";

        if(month < 10){
          result += "0";  
        }
        result +=  String(month) + "-";  

        if(date < 10){
          result += "0";  
        }
        result +=  String(date) + " ";  

        if(hours < 10){
          result += "0";  
        }
        result +=  String(hours) + ":";  

        if(minutes < 10){
          result += "0";  
        }
        result +=  String(minutes) + ":";

        // if(seconds > 10){
        //     seconds = seconds - 10;
        // }else{
        //     seconds = 0;
        // }
        
        if(seconds < 10){
          result += "0";  
        }
        result +=  String(seconds);
    }
    // Serial.println(result);
    return(  result.c_str() );
}

inline void setRTC(uint8_t date,uint8_t month,uint16_t year,uint8_t hours,uint8_t min, uint8_t seconds){
    if(RTC_OK){
        RTC.setClockMode(false); 
        if(year > 2000){
            year = year - 2000;
        }
        RTC.setYear(year );
        RTC.setMonth(month);
        RTC.setDate(date);

        RTC.setHour(hours);
        RTC.setMinute(min);
        RTC.setSecond(seconds);
        RTC.setClockMode(false);

        internalrtc.setTime(seconds,min,hours,date,month,year);
    }else{
        internalrtc.setTime(seconds,min,hours,date,month,year);
    }
}

inline uint8_t getSeconds(){
    if(RTC_OK){
        return(RTC.getSecond());
    }else{
      return(internalrtc.getSecond());
    }
}

inline uint8_t getDate(){
    if(RTC_OK){
        return(RTC.getDate());
    }else{
      return(internalrtc.getDay());
    }
}

inline uint8_t getMonth(){
    if(RTC_OK){
        return(RTC.getMonth(Century));
    }else{
      return(internalrtc.getMonth()+1);
    }
}

inline uint8_t getMinute(){
    if(RTC_OK){
        return(RTC.getMinute());
    }else{
      return(internalrtc.getMinute());
    }
}

inline uint8_t getHour(){
    if(RTC_OK){
        return(RTC.getHour(h12, PM));
    }else{
      return(internalrtc.getHour(true));
    }
}

inline uint16_t getyear(){
    if(RTC_OK){
        return(RTC.getYear());
    }else{
      return(internalrtc.getYear());
    }
}

inline uint16_t getYear(){
    if(RTC_OK){
        return(RTC.getYear());
    }else{
      return(internalrtc.getYear());
    }
}

#endif