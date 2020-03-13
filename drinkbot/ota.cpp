#include "ota.h"

String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

void perform_ota(WiFiClient* client) {
  long content_length = 0;
  bool is_valid_content_type = false;
	
  Serial.println("Connecting to: " + String(OTA_HOST));
  // Connect to S3
  if (client->connect(OTA_HOST, OTA_PORT)) {
	// Connection Succeed.
	// Fecthing the FIRMWARE_BIN
	Serial.println("Fetching FIRMWARE_BIN: " + String(FIRMWARE_BIN));

	// Get the contents of the FIRMWARE_BIN file
	client->print(String("GET ") + FIRMWARE_BIN + " HTTP/1.1\r\n" +
				 "host: " + OTA_HOST + "\r\n" +
				 "Cache-Control: no-cache\r\n" +
				 "Connection: close\r\n\r\n");

	// Check what is being sent
	//    Serial.print(String("GET ") + FIRMWARE_BIN + " HTTP/1.1\r\n" +
	//                 "OTA_HOST: " + OTA_HOST + "\r\n" +
	//                 "Cache-Control: no-cache\r\n" +
	//                 "Connection: close\r\n\r\n");

	unsigned long timeout = millis();
	while (client->available() == 0) {
	  if (millis() - timeout > 5000) {
		Serial.println("Client Timeout !");
		client->stop();
		return;
	  }
	}
	// Once the response is available,
	// check stuff

	while (client->available()) {
	  // read line till /n
	  String line = client->readStringUntil('\n');
	  // remove space, to check if the line is end of headers
	  line.trim();

	  // if the the line is empty,
	  // this is end of headers
	  // break the while and feed the
	  // remaining `client` to the
	  // Update.writeStream();
	  if (!line.length()) {
		//headers ended
		break; // and get the OTA started
	  }

	  // Check if the HTTP Response is 200
	  // else break and Exit Update
	  if (line.startsWith("HTTP/1.1")) {
		if (line.indexOf("200") < 0) {
		  Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
		  break;
		}
	  }

	  // extract headers here
	  // Start with content length
	  if (line.startsWith("Content-Length: ")) {
		content_length = atol((getHeaderValue(line, "Content-Length: ")).c_str());
		Serial.println("Got " + String(content_length) + " bytes from server");
	  }

	  // Next, the content type
	  if (line.startsWith("Content-Type: ")) {
		String contentType = getHeaderValue(line, "Content-Type: ");
		Serial.println("Got " + contentType + " payload.");
		if (contentType == "application/octet-stream") {
		  is_valid_content_type = true;
		}
	  }
	}
  } else {
	// Connect to S3 failed
	// May be try?
	// Probably a choppy network?
	Serial.println("Connection to " + String(OTA_HOST) + " failed. Please check your setup");
	// retry??
	// execOTA();
  }

  // Check what is the content_length and if content type is `application/octet-stream`
  Serial.println("content_length : " + String(content_length) + ", is_valid_content_type : " + String(is_valid_content_type));

  // check content_length and content type
  if (content_length && is_valid_content_type) {
	// Check if there is enough to OTA Update
	bool canBegin = Update.begin(content_length);

	// If yes, begin
	if (canBegin) {
	  Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
	  // No activity would appear on the Serial monitor
	  // So be patient. This may take 2 - 5mins to complete

	Update.writeStream(*client);

	  if (Update.end()) {
		Serial.println("OTA done!");
		if (Update.isFinished()) {
		  Serial.println("Update successfully completed. Rebooting.");
		  ESP.restart();
		} else {
		  Serial.println("Update not finished? Something went wrong!");
		}
	  } else {
		Serial.println("Error Occurred. Error #: " + String(Update.getError()));
	  }
	} else {
	  // not enough space to begin OTA
	  // Understand the partitions and
	  // space availability
	  Serial.println("Not enough space to begin OTA");
	  client->flush();
	}
  } else {
	Serial.println("There was no content in the response");
	client->flush();
  }
}