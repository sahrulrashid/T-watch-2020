#include "config.h"
#include "DudleyWatch.h"
#include "personal_info.h"
#include <WiFi.h>
#include <HTTPClient.h>

static String payload;

// do http or https request, leave result in payload, possibly copy to result
String get_thing(char *url, char *result, int rlength) {
int errcnt;
  errcnt = 0;
  do {
    HTTPClient http;
    http.begin(url);
    // start connection and send HTTP header
    int httpCode = http.GET();
    // httpCode will be negative on error
    if(httpCode > 0) {
	// HTTP header has been send and server response header has been handled
	if(httpCode == HTTP_CODE_OK
	|| httpCode == 301) {	// success!
	  payload = http.getString();
	  // Serial.println(payload);
	  int max_length = 1 + payload.length();
	  if(max_length > rlength) {
	    max_length = rlength;
	  }
	  if(result && max_length) {
	    payload.toCharArray(result, max_length);
	  }
	}
	else {
	  Serial.printf("http.GET problem, code: %d\n", httpCode);
	  payload = http.getString();
	  Serial.println(payload);
	  errcnt++;
	}
    }
    else {
	Serial.printf("http.GET failed, error: %s\n", http.errorToString(httpCode).c_str());
	errcnt++;
	delay(10);
    }
    http.end();
  } while(errcnt && errcnt < 3);
  return payload;
}

static const char *ip_apis[] = {
//"https://helloacm.com/api/what-is-my-ip-address/",	// unusable
  "https://www.dudley.nu/printip.cgi",
  "http://bot.whatismyipaddress.com/",
//"https://api.ipify.org",	// doesn't work, no idea why
  "http://danml.com/myip/"
};

char str_latitude[20];
char str_longitude[20];
char city[50];

boolean is_non_routable_ip (char *my_ipaddress) {
  if(!strncmp(my_ipaddress, "192.168.", 8)
  || !strncmp(my_ipaddress, "10.", 3)
  || !strncmp(my_ipaddress, "172.16.", 7)
  || !strncmp(my_ipaddress, "172.17.", 7)
  || !strncmp(my_ipaddress, "172.18.", 7)
  || !strncmp(my_ipaddress, "172.19.", 7)
  || (!strncmp(my_ipaddress, "172.2", 5) && isdigit(my_ipaddress[5]) && my_ipaddress[6] == '.')
  || !strncmp(my_ipaddress, "172.30.", 7)
  || !strncmp(my_ipaddress, "172.31.", 7)) {
    return true;
  }
  return false;
}

void remove_trailing_newline(char *s) {
  int iplastchar = strlen(s) - 1;
  while(iplastchar > 1 && s[iplastchar] < ' ') {
    s[iplastchar] = '\0';
    iplastchar = strlen(s) - 1;
  }
}

// 0 result means success
// -1 result means default to home IP
int get_lat_lon (void) {
int result;
  result = -1;
  str_latitude[0] = '\0';
  str_longitude[0] = '\0';
  int home_ip_len = strlen(HOME_IP);
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
    result = 0;
    char my_ipaddress[20];
    char request_url[200];
    memset(my_ipaddress, '\0', sizeof(my_ipaddress));
    for(int ii = 0 ; ii < sizeof(ip_apis)/sizeof(char *) ; ii++) {
      strcpy(request_url, ip_apis[ii]);
      Serial.printf("trying %s\n", request_url);
      get_thing(request_url, my_ipaddress, sizeof(my_ipaddress));
      remove_trailing_newline(my_ipaddress);
      Serial.printf("after attempt %d, my ip = %s\n", ii+1, my_ipaddress);
      if(is_non_routable_ip(my_ipaddress)) {
	my_ipaddress[0] = '\0';
	Serial.println("non-routable!");
      }
      if(my_ipaddress[0]) {
	break;
      }
    }
    if(!my_ipaddress[0] || !isdigit(my_ipaddress[0]) || is_non_routable_ip(my_ipaddress)) {
      Serial.printf("last attempt got bad or non-routable IP\n");
      strcpy(my_ipaddress, HOME_IP);
      Serial.printf("can't get ip address, using home %s\n", my_ipaddress);
      result = -1;
    }
    Serial.printf("my ip is %s\n", my_ipaddress);
    // if NOT my ip AND ( !str_latitude OR !str_longitude), DO
    // if it's NOT my home IP address, look up str_latitude and str_longitude
    if(strncmp(my_ipaddress, HOME_IP, home_ip_len)
    && (!str_latitude[0] || !str_longitude[0])) {
      sprintf(request_url, "%s%s/latitude", "https://ipapi.co/", my_ipaddress);
      Serial.printf("trying %s\n", request_url);
      get_thing(request_url, str_latitude, sizeof(str_latitude));
      Serial.printf("my latitude is %s\n", str_latitude);
      sprintf(request_url, "%s%s/longitude", "https://ipapi.co/", my_ipaddress);
      Serial.printf("trying %s\n", request_url);
      get_thing(request_url, str_longitude, sizeof(str_longitude));
      Serial.printf("my longitude is %s\n", str_longitude);
      sprintf(request_url, "%s%s/city", "https://ipapi.co/", my_ipaddress);
      Serial.printf("trying %s\n", request_url);
      get_thing(request_url, city, sizeof(city));
      Serial.printf("my city is %s\n", city);
    }
    if(!strncmp(my_ipaddress, HOME_IP, home_ip_len)) {
      strcpy(city, HOME_CITY);
    }
    if(!str_latitude[0]) {
      strcpy(str_latitude, HOME_LATITUDE);
      Serial.printf("at home or latitude get failed, using default %s\n", str_latitude);
      result = -1;
    }
    if(!str_longitude[0]) {
      strcpy(str_longitude, HOME_LONGITUDE);
      Serial.printf("at home or longitude get failed, using default %s\n", str_longitude);
      result = -1;
    }
  }
  return result;
}