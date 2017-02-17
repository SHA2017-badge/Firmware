#include "matrix_client.h"
/**
 * Based on code from https://matrix.org/docs/projects/other/matrix-esp8266.html
 * By matt-williams
 */

#define MATRIX_SERVER "https://chat.weho.st"
#define MATRIX_ROOM "!KkvzNRuBRnkSqbfYuC:chat.weho.st"

using namespace std;

void createLoginBody(char* buffer, int bufferLen, string user, string password) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = "m.login.password";
  root["user"] = user;
  root["password"] = password;
  root.printTo(buffer, bufferLen);
}

void createMessageBody(char* buffer, int bufferLen, string message) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["msgtype"] = "m.text";
  root["body"] = message;
  root.printTo(buffer, bufferLen);
}

bool login(string user, string password) {
  bool success = false;

  char buffer[512];
  createLoginBody(buffer, 512, user, password);

  string url = MATRIX_SERVER + "/_matrix/client/r0/login";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int rc = http.POST(buffer);
  if (rc > 0) {
    if (rc == HTTP_CODE_OK) {
      string body = http.getString();
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(body);
      string myAccessToken = root["access_token"];
      accessToken = string(myAccessToken.c_str());
      Serial.println(accessToken);
      success = true;
    }
  } else {
    ets_printf("Error: %s\n", http.errorTostring(rc).c_str());
  }

  return success;
}

bool getMessages(string roomId) {
  bool success = false;

  string url = MATRIX_SERVER + "/_matrix/client/r0/rooms/" + roomId + "/messages?access_token=" + accessToken + "&limit=1";
  if (lastMessageToken == "") {
    url += "&dir=b";
  } else {
    url += "&dir=f&from=" + lastMessageToken;
  }
  ets_printf("GET %s\n", url.c_str());

  http.begin(url);
  int rc = http.GET();
  if (rc > 0) {
    ets_printf("%d\n", rc);
    if (rc == HTTP_CODE_OK) {
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(http.getString());
      if (lastMessageToken != "") {
        JsonArray& chunks = root["chunk"];
        JsonObject& chunk = chunks[0];
        string sender = chunk["sender"];
        JsonObject& content = chunk["content"];
        if (content.containsKey("body")) {
          string body = content["body"];
          Serial.println(body);
          serial2.println(body);
        }
      }
      string myLastMessageToken = root["end"];
      lastMessageToken = string(myLastMessageToken.c_str());
      Serial.println(lastMessageToken);
      success = true;
    }
  } else {
    ets_printf("Error: %s\n", http.errorToString(rc).c_str());
  }

  return success;
}

bool sendMessage(string roomId, string message) {
  bool success = false;

  char buffer[512];
  createMessageBody(buffer, 512, message);

  string url = MATRIX_SERVER + "/_matrix/client/r0/rooms/" + roomId + "/send/m.room.message/" + string(millis()) + "?access_token=" + accessToken + "&limit=1";
  ets_printf("PUT %s\n", url.c_str());

  http.begin(url);
  int rc = http.sendRequest("PUT", buffer);
  if (rc > 0) {
//    ets_printf("%d\n", rc);
    if (rc == HTTP_CODE_OK) {
      success = true;
    }
  } else {
    ets_printf("Error: %s\n", http.errorToString(rc).c_str());
  }
  return success;
}
