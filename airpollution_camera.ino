#include "WiFi.h"
#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <SPIFFS.h>
#include <FS.h>
#include "Firebase_ESP_Client.h"
//Provide the token generation process info.
#include <addons/TokenHelper.h>




#include "addons/RTDBHelper.h" //Provide the real-time database payload printing info and other
//Replace with your network credentials

FirebaseData firebaseData;
//Replace with your network credentials
const char* ssid = "Lohith";
const char* password = "9945584409";

const String FIREBASE_PATH  = "/";

// Insert Firebase project API Key
#define API_KEY "AIzaSyBILhhjgyZfzrG85-3y5UVtyoxD5QINQWc"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "lohithsrinivas.14@gmail.com"
#define USER_PASSWORD "lohith$2002"

// Insert Firebase storage bucket ID e.g bucket-name.appspot.com
#define STORAGE_BUCKET_ID "airpollution-bb9c1.appspot.com"
#define DATABASE_URL "airpollution-bb9c1-default-rtdb.firebaseio.com"
// Photo File Name to save in SPIFFS
#define FILE_PHOTO "/photo.jpeg"

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
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

boolean takeNewPhoto = false;

//Define Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;
String uid;
String databasePath;
FirebaseJson json;
bool taskCompleted = false;

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly
  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }
    // Photo file name
    
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
}
int count=0;
void initWiFi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
    Serial.println(WiFi.localIP());
      Serial.println();
}

void initSPIFFS(){
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
}

void initCamera(){
 // OV2640 camera module
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
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  } 
}

String parentPath;
void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  
  initWiFi();

  initSPIFFS();
  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  initCamera();
 Serial.println("Camera Inititalization is completed");
  //Firebase
  // Assign the api key
  configF.api_key = API_KEY;
  //Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  //Assign the callback function for the long running token generation task
  configF.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
configF.database_url = DATABASE_URL;
Firebase.reconnectWiFi(true);
Firebase.begin(&configF, &auth);

    delay(100);
 while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);
  
   databasePath = "/UsersData/" ;

}

void loop() {
  count++;

  
  if (takeNewPhoto) {
    capturePhotoSaveSpiffs();
    
  
  delay(1);
  if (Firebase.ready() && !taskCompleted){
    parentPath= databasePath;
     json.set("count", String(count));
     
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());

    Serial.print("Uploading picture... ");
      if (Firebase.Storage.upload(&fbdo, 
 STORAGE_BUCKET_ID /* Firebase Storage bucket id */,
    FILE_PHOTO /* path to local file */, 
    mem_storage_type_flash 
    /* memory storage type, 
    mem_storage_type_flash and mem_storage_type_sd */, 
    FILE_PHOTO /* path of remote file stored in the bucket */, 
    "image/jpeg" /* mime type */)){
      Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());
      takeNewPhoto = false;
    }
    
    else{
      Serial.println(fbdo.errorReason());
    }
  }
  }

if (Firebase.RTDB.getString(&fbdo, "fire"))
{
String read_data = fbdo.stringData();

Serial.print("Data received: ");

Serial.println(read_data); //print the data received from the Firebase database
 if(read_data=="1")
 takeNewPhoto = true;

}

/*if (Firebase.RTDB.getString(&fbdo, "Subdata"))
{
String read_data = fbdo.stringData();

Serial.print("Data received: ");

Serial.println(read_data); //print the data received from the Firebase database
 if(read_data[2]=='1')
 takeNewPhoto = true;
}*/
  

  
}
