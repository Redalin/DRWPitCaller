// add this line including the hash: 
// #include "config.h" 
// config.h
#ifndef CONFIG_H
#define CONFIG_H

// The device hostname
const char* hostname = "pitcaller";

// Wifi network credentials
const char* KNOWN_SSID[] = {"DRW", "ChrisnAimee.com"};
const char* KNOWN_PASSWORD[] = {"wellington", "carbondell"};
const int   KNOWN_SSID_COUNT = sizeof(KNOWN_SSID) / sizeof(KNOWN_SSID[0]); // number of known networks

#endif
