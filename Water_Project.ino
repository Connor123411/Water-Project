#include <WiFi.h>
#include <ESP_Mail_Client.h>

/*
TODO:
Turn on the water pump
Turn on the LED's 
Check the humidity of the soil - soil sensor 
Check temperature
Check Water level
*/

// Pin define
#define GREEN_LED 23
#define RED_LED 18
#define WATER_SWITCH 2
#define SOIL_PIN 32
#define PUMP_CONTROL 19
#define TEMPERATURE_SENSOR 4


// WiFi credentials
#define WIFI_SSID "Phone" // Name of the wifi
#define WIFI_PASSWORD "Spiderman" // Wifi password

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

// Email credentials
#define AUTHOR_EMAIL "esp32waterthing@gmail.com" // Sender email address
#define AUTHOR_PASSWORD "lbls clbf dlhd gseb" // Sender application password 
#define RECIPENT_EMAIL "connorfleming57@gmail.com" // Recipent email, password for gmail account 1password123

// Static variables
static bool waterEmailSent = false;
static bool tempEmailSent = false;


// For the subject and text field of what is sent to the Recipent's email

const char* SUBJECTS[] = {"⚠️⚠️⚠️ERROR⚠️⚠️⚠️", "Warning: Plant temperature out of range", "Warning: Plant temperature out of range", "Warning: Low water level"};

const char* TEXTS[] = {"ERROR occured 🦖, device has shutdown, manual fix required. Watch out for the dinosaur! 🦖", 
  "Plant's environment is too hot, this could damage the plant or kill the plant.", 
  "Plant's environment is too cold, this could damage the plant or kill the plant.", 
  "Device is running out of water, please fill the reserviour. "};

typedef enum {
  ERROR = 0,
  HOT,
  COLD,
  WATER,
} subjects_t;

typedef enum {
    FULL,
    EMPTY
} ReservoirState_t;

// Create an SMTP session
SMTPSession smtp;

// Forward declarations
void wifiStart();
void emailSend(const char*, const char*);   // using const char* to reduce space
void checkWaterLevel();
void ledInit();
void setLeds(ReservoirState_t state);
void waterSwitchInit();
void pumpInit();
void setPump();
void runPumpCheck();
// void checkTemperature();

void setup() 
{
  Serial.begin(115200);
  wifiStart(); // call the wifi start function
  Serial.println("Wifi Connected");
  ledInit();
  waterSwitchInit();
  pumpInit();
}

void loop()
{
  checkWaterLevel();
  runPumpCheck();
  // checkTemperature();
  delay(500); // Delay 500 ms
}


void wifiStart()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);   // Begin connecting to the wifi 
  
  while (WiFi.status() != WL_CONNECTED) {    // Checking when the wifi is connected, keep retrying for 10s
    delay(500);
  }
}

void emailSend(const char* subject, const char* textMesssage)
{
  // Send email after connecting to WiFi
  MailClient.networkReconnect(true);
  smtp.debug(1);      // enable debugging if sending fails

  Session_Config config;  // Configure the sending criteria
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";

  config.time.ntp_server = "0.nz.pool.ntp.org"; // In theory shouldn't need NTP time as no scheduling should occur

  SMTP_Message message;   // Configure the message that is going to be sent

  message.sender.name = "ESP32";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = subject;
  message.addRecipient("User", RECIPENT_EMAIL);

  message.text.content = textMesssage;
  message.text.charSet = "UTF-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_8bit;     // 8 bit encoding
  
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;    // Set the priority to low as its not super important how long it takes to be recieved
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  
  /* Connect to the server */
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()){
    Serial.println("\nNot yet logged in.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
  Serial.println("Email sent successfully");
}

void ledInit()
{ //Initialises the LEDS
    pinMode(GREEN_LED, OUTPUT);
    digitalWrite(GREEN_LED, LOW);
    pinMode(RED_LED, OUTPUT);
    digitalWrite(RED_LED, LOW);
}

void setLeds(ReservoirState_t state)
{
  if (state == FULL) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
  }
}

void waterSwitchInit()
{
  pinMode(WATER_SWITCH, INPUT_PULLUP);
}

void checkWaterLevel()
{
  if (digitalRead(WATER_SWITCH)) {
    setPump(false);
    setLeds(EMPTY);
    if (!waterEmailSent) {
      emailSend(SUBJECTS[WATER], TEXTS[WATER]);
    }

    waterEmailSent = true;
  } else {
    setLeds(FULL);
    waterEmailSent = false;
  }
}

float checkMoisture() 

{
  float maxVal = 3100;
  float moistureAdc = analogRead(SOIL_PIN);
  float humidity = (moistureAdc / maxVal); // humidity as a percent
  return humidity;
}

void pumpInit()
{
  pinMode(PUMP_CONTROL, OUTPUT);
  digitalWrite(PUMP_CONTROL, LOW);
}

void setPump(bool turnOn)
{
  if (turnOn) {
    digitalWrite(PUMP_CONTROL, HIGH);
  } else {
    digitalWrite(PUMP_CONTROL, LOW);
  }
}

void runPumpCheck()
{
  float humidity = checkMoisture();
  if (!waterEmailSent && humidity <=0.7) {
    unsigned long startTime = millis();
    Serial.println("Dispensing Water");
    while (millis()-startTime < 2500 && !waterEmailSent) {
      setPump(true);
      checkWaterLevel();
    }
    setPump(false);
  }
}
/*
void checkTemperature()
{
  float temp = getTemperature();
  if (temp > 40) {
    //emailsend(SUBJECTS[1], TEXTS[1]);
    continue;
  } else if {
    //emailsend(SUBJECTS[2], TEXTS[2]);
    continue;
  }
}


float getTemperature()
{
  float temperature = analogRead(TEMPERATURE_SENSOR) * 500 / 1023;
  return temperature; 
}
*/