#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// ------------------ WiFi Credentials ------------------
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ------------------ Telegram Bot ------------------
String BOTtoken = "YOUR_BOT_TOKEN";  
String CHAT_ID = "YOUR_CHAT_ID";

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(token, clientTCP);


// ------------------ PIR Pin ------------------
#define PIR_PIN 13

// ------------------ AI Thinker ESP32-CAM Pins ------------------
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

bool motionDetected = false;

// ------------------ Camera Init Function ------------------
void configInitCamera() {
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

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
}

// ------------------ Send Photo to Telegram ------------------
void sendPhotoTelegram() {
  Serial.println("📸 Capturing photo...");

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  Serial.println("📤 Sending photo to Telegram...");

  clientTCP.setInsecure();

  String boundary = "RandomNerdTutorials";
  String head = "--" + boundary + "\r\n"
                "Content-Disposition: form-data; name=\"chat_id\";\r\n\r\n" + CHAT_ID + "\r\n"
                "--" + boundary + "\r\n"
                "Content-Disposition: form-data; name=\"photo\"; filename=\"motion.jpg\"\r\n"
                "Content-Type: image/jpeg\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";

  uint32_t imageLen = fb->len;
  uint32_t totalLen = head.length() + imageLen + tail.length();

  if (clientTCP.connect("api.telegram.org", 443)) {
    clientTCP.println("POST /bot" + token + "/sendPhoto HTTP/1.1");
    clientTCP.println("Host: api.telegram.org");
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=" + boundary);
    clientTCP.println();
    clientTCP.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;

    for (size_t n = 0; n < fbLen; n = n + 1024) {
      if (n + 1024 < fbLen) {
        clientTCP.write(fbBuf + n, 1024);
      } else {
        clientTCP.write(fbBuf + n, fbLen - n);
      }
    }

    clientTCP.print(tail);

    esp_camera_fb_return(fb);

    Serial.println("✅ Photo sent to Telegram!");

  } else {
    Serial.println("❌ Connection to Telegram failed");
    esp_camera_fb_return(fb);
  }
}

// ------------------ Setup ------------------
void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);

  Serial.println("Initializing Camera...");
  configInitCamera();

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  bot.sendMessage(CHAT_ID, "🚀 ESP32-CAM Motion Detector Started!", "");
}

// ------------------ Loop ------------------
void loop() {
  int pirValue = digitalRead(PIR_PIN);

  if (pirValue == HIGH) {
    Serial.println("🚨 Motion Detected!");
    sendPhotoTelegram();
    delay(10000);  // 10 sec cooldown
  }

  delay(200);
}
