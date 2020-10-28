
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"               // SD Card ESP32
#include "SD_MMC.h"           // SD Card ESP32
#include "soc/soc.h"          // Disable brownout problems
#include "soc/rtc_cntl_reg.h" // Disable brownout problems
#include "driver/rtc_io.h"
#include <EEPROM.h> // read and write from flash memory
#include "camera_pins.h"
//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

#define CAMERA_MODEL_AI_THINKER
#define EEPROM_SIZE 1
#define REDLED 15
#define GREENLED 13

WiFiClient client;
int pictureNumber = 0;
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  pinMode(4, OUTPUT);
  pinMode(14, INPUT);
  pinMode(REDLED, OUTPUT);
  pinMode(GREENLED, OUTPUT);
  digitalWrite(REDLED, LOW);
  digitalWrite(GREENLED, LOW);
  int z = 0;
  while (z < 20)
  {
    digitalWrite(GREENLED, HIGH);
    delay(250);
    digitalWrite(GREENLED, LOW);
    digitalWrite(REDLED, HIGH);
    delay(250);
    digitalWrite(REDLED, LOW);
    z++;
  }
  //digitalWrite(lampledPin, HIGH);
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound())
  {
    //  Serial.println("Getting XGA Images");
    config.frame_size = FRAMESIZE_XGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    Serial.println("Stuck with SVGA");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    flashError();
  }
  if (!SD_MMC.begin())
  {
    Serial.println("SD Card Mount Failed");
    flashError();
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No SD Card attached");
    flashError();
  }

  sensor_t *s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID)
  {
    s->set_vflip(s, 1);       //flip it back
    s->set_brightness(s, 1);  //up the blightness just a bit
    s->set_saturation(s, -2); //lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_XGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin("YOURSSID", "YOUR_WIFI_PASSWD");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;

  Serial.print("Camera Ready!");
  digitalWrite(GREENLED, HIGH);
}

void flashError()
{
  while (1)
  {
    digitalWrite(REDLED, HIGH);
    delay(500);
    digitalWrite(REDLED, LOW);
  }
}
void loop()
{
  if (digitalRead(14) == HIGH)
  {

    digitalWrite(4, HIGH);
    camera_fb_t *fb = NULL;
    delay(1000);
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      digitalWrite(GREENLED, LOW);
      int x = 10;
      while (x > 0)
      {
        digitalWrite(REDLED, HIGH);
        delay(500);
        digitalWrite(REDLED, LOW);
        x--;
      }
      digitalWrite(GREENLED, HIGH);
      return;
    }
    String path = "/picture" + String(pictureNumber) + ".jpg";
    fs::FS &fs = SD_MMC;
    // Serial.printf("Picture file name: %s\n", path.c_str());
    while (fs.exists(path.c_str()))
    {
      Serial.printf("File %s exists, incrementing...", path.c_str());
      pictureNumber++;
      path = "/picture" + String(pictureNumber) + ".jpg";
    }
    File file = fs.open(path.c_str(), FILE_WRITE);
    if (!file)
    {
      Serial.println("Failed to open file in writing mode");
    }
    else
    {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.printf("Saved file to path: %s\n", path.c_str());
      EEPROM.write(0, pictureNumber);
      EEPROM.commit();
    }
    file.close();
    digitalWrite(4, LOW);

    file = fs.open(path.c_str(), FILE_READ);
    String fileName = file.name();
    String fileSize = String(file.size());

    if (file)
    {
      byte server[] = {192, 168, 1, 213};
      if (client.connect(server, 9090))
      {
        Serial.println(F("connected to server"));

        //      // Make a HTTP request:
        //
        String start_request = "";
        String end_request = "";
        start_request = start_request +
                        "\n--AaB03x\n" +
                        "Content-Disposition: form-data; name=\"uploadfile\"; filename=\"" + file.name() + "\"\n" +
                        "Content-Transfer-Encoding: binary\n\n";
        String midRequest = "";
        midRequest += "\n--AaB03x\nContent-Disposition: form-data; foo=\"bar\"; \nContent-Transfer-Encoding: text\n\n";
        end_request = end_request + "\n--AaB03x--\n";
        uint16_t full_length;
        full_length = start_request.length() + file.size() + midRequest.length() + end_request.length();
        Serial.println("Connected ok!");
        client.println("POST /photo HTTP/1.1");
        client.println("Host: example.com");
        client.println("User-Agent: ESP32");
        client.println("Content-Type: multipart/form-data; boundary=AaB03x");
        client.print("Content-Length: ");
        client.println(full_length);
        client.print(start_request);
        const int bufSize = 2048;
        byte clientBuf[bufSize];
        int clientCount = 0;
        while (file.available())
        {
          clientBuf[clientCount] = file.read();
          clientCount++;
          if (clientCount > (bufSize - 1))
          {
            client.write((const uint8_t *)clientBuf, bufSize);
            // Serial.print((char *)clientBuf);
            clientCount = 0;
          }
        }
        if (clientCount > 0)
        {
          client.write((const uint8_t *)clientBuf, clientCount);
          // Serial.print((char *)clientBuf);
        }
        client.print(midRequest);
        client.print(end_request);
        client.stop();
        Serial.println("Done!");

        esp_camera_fb_return(fb);
      }
    }
  }
}
