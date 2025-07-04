#include <ESP8266WiFi.h>                                     // Imports WiFi functions needed to connect ESP8266 to internet
#include <ESP_Mail_Client.h>                                // Imports library to send emails using SMTP from ESP8266
#include <ThingSpeak.h>                                    // Imports the ThingSpeak library
 
#define SMTP_HOST "smtp.gmail.com"                        // To tell ESP8266 to communicate with Gmail (Defines Gmail’s server address)
#define SMTP_PORT 465                                    // To tell ESP8266, when sending emails, use Gmail's email door number 465 to talk securely.
#define AUTHOR_EMAIL "your email id"
#define AUTHOR_PASSWORD "app password"          // An App Password is a more secure way to allow devices to access your Gmail without giving your real password

char ssid[] = "wifi name";                              // WiFi SSID
char pass[] = "wifi password";                        // WiFi password

#define MQ02_sensor A0                             // Input pin for the smoke sensor
#define flame_sensor 16                           // Input pin for flame sensor
int MQ02value = 0;                               // To store the value coming from the MQ02 sensor
int flame_sensorval = HIGH;                     // To get flame sensor value 
bool alertSent = false;                        // To see if email has been sent or not
int threshold = 400;                          // Fix value received from MQ02 above which smoke is dangerous

String recipients[] = {                      // Email address of the recipients
  "recipient email id(s)",
};

int n = sizeof(recipients) / sizeof(recipients[0]);

SMTPSession smtp;                               // Creates the email-sending engine ESP8266 will use to talk to Gmail and send alerts

unsigned long channelID =  <thingspeak channel id>;      // ThingSpeak channel ID
const char *writeAPIKey = "thingspeak Write API key";  // ThingSpeak Write API Key

WiFiClient client;                           // Client object for ThingSpeak communication

void setup() {
  Serial.begin(115200);
  delay(1500);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  smtp.callback(smtpCallback);         
  pinMode(flame_sensor, INPUT);
  pinMode(MQ02_sensor, INPUT);
  setupSessionConfig();
  ThingSpeak.begin(client);                            // Initialize ThingSpeak
}

Session_Config sessionConfig;

void setupSessionConfig() {
  sessionConfig.server.host_name = SMTP_HOST;
  sessionConfig.server.port = SMTP_PORT;
  sessionConfig.login.email = AUTHOR_EMAIL;
  sessionConfig.login.password = AUTHOR_PASSWORD;
}

void loop() {
  MQ02value = analogRead(MQ02_sensor);
  flame_sensorval = digitalRead(flame_sensor);
  sendDataToThingSpeak(MQ02value, flame_sensorval);
  delay(10000);                                                             // Delay to avoid rapid updates                                
    if ((MQ02value > threshold || flame_sensorval == LOW) && !alertSent) {
    sendAlertEmails("Sensors detected flame/smoke! Check immediately!");
    alertSent = true;
  }
  if (MQ02value <= threshold) {
    alertSent = false;                                                     // Reset when value goes back to normal
  }

}

void sendAlertEmails(String messageText) {
  for (int i = 0; i < n; i++) {
    SMTP_Message message;                                                 // Message is the variable from data type SMTP_Message that will hold the email.
    message.sender.name = "ESP8266";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "Fire Detected!";
    message.addRecipient("Dear User", recipients[i]);
    message.text.content = messageText;  // Receives value for the parameter from the void loop block
    message.text.charSet = "utf-8";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;       

    if (!smtp.connect(&sessionConfig)) {  // Sends a message if failure occurs to connect the device with smtp
      Serial.println("SMTP connection failed: " + smtp.errorReason());
      return;  // Similar to break but used for exiting functions because break only works for loops
    }
    if (!MailClient.sendMail(&smtp, &message)) {  // Checks if the email recipient has been sent the mail or not (sendMail used to send)
      Serial.println("Sending failed: " + smtp.errorReason());
    } else {
      Serial.println("Email sent to: " + recipients[i]);
    }
    smtp.closeSession();
  }
}

void smtpCallback(SMTP_Status status) {
  Serial.println("SMTP Callback:");
  Serial.println(status.info());
}

void sendDataToThingSpeak(int smokeValue, int flameValue) {
  ThingSpeak.setField(1, smokeValue);       // Set the smoke sensor value to field 1 on ThingSpeak
  ThingSpeak.setField(2, flameValue);       // Set the flame sensor value to field 2 on ThingSpeak
  int response = ThingSpeak.writeFields(channelID, writeAPIKey); // Send the data to ThingSpeak
  if (response == 200) {
    Serial.println("Data sent to ThingSpeak successfully.");
  } else {
    Serial.println("Failed to send data to ThingSpeak. Response code: " + String(response));
  }
}
