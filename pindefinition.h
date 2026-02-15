#if defined(ESP32)
    #if CONFIG_IDF_TARGET_ESP32C6
        #define TRIGGER_PIN 9  // ESP32-C6
        
        #define SCL_PIN 3
        #define SDA_PIN 15
        #define INPUT_INTERRUPT 7

        // for Ethernet W5500
        #define SCK_PIN  22
        #define MISO_PIN 23
        #define MOSI_PIN 18
        #define CS_PIN   21
        #define RST_PIN   1

        #define RGB_LED_PIN   0

        #define HX711_DOUT  10
        #define  HX711_SCK  11

        #define  TX2_PIN    35
        #define  RX2_PIN    17

        #define DWIN_TX_PIN 21
        #define DWIN_RX_PIN 20

    #elif CONFIG_IDF_TARGET_ESP32S3
        #define TRIGGER_PIN 0 // ESP32-S3 
        #define SCL_PIN 2
        #define SDA_PIN 42
        #define INPUT_INTERRUPT 7

        #define SCK_PIN  39
        #define MISO_PIN 40
        #define MOSI_PIN 1
        #define CS_PIN   38
        #define RST_PIN   16
        #define RGB_LED_PIN   15

        #define HX711_DOUT  18 
        #define  HX711_SCK   8

        #define  TX2_PIN    35
        #define  RX2_PIN    17

        #define DWIN_TX_PIN 37
        #define DWIN_RX_PIN 36
        
    #else
        #define TRIGGER_PIN 0  // Default ESP32
        #define SCL_PIN 22
        #define SDA_PIN 21
        #define INPUT_INTERRUPT 35

        #define SCK_PIN  18
        #define MISO_PIN 19
        #define MOSI_PIN 23
        #define CS_PIN   5
        #define RST_PIN   33

        #define RGB_LED_PIN   32

        #define HX711_DOUT  26
        #define  HX711_SCK  27

        #define  TX2_PIN    25
        #define  RX2_PIN    4

        #define DWIN_TX_PIN 17
        #define DWIN_RX_PIN 16

    #endif
#else
    #error "Unsupported board"
#endif