/*
victron-smartshunt.c
  see:
  https://community.victronenergy.com/questions/93919/victron-bluetooth-ble-protocol-publication.html
  
  First you have to enable the "third party implementation"-protocol in the victron app under Produkt-Info:
  activate "Bluetooth GATT service"

  New values:
      '65970383-4bda-4c1e-af4b-551c4cf74769'
      '65970382-4bda-4c1e-af4b-551c4cf74769'
      '6597edec-4bda-4c1e-af4b-551c4cf74769'
      
 deps: https://github.com/labapart/gattlib.git


MIT License

Copyright (c) 2022 dr-ni

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "gattlib.h"

volatile bool g_operation_completed;

const char* uuid_str_SOC = "65970fff-4bda-4c1e-af4b-551c4cf74769";
const char* uuid_str_Vbank = "6597ed8d-4bda-4c1e-af4b-551c4cf74769";
const char* uuid_str_Deviation = "65970383-4bda-4c1e-af4b-551c4cf74769";
const char* uuid_str_A = "6597ed8c-4bda-4c1e-af4b-551c4cf74769";
const char* uuid_str_P = "6597ed8e-4bda-4c1e-af4b-551c4cf74769";
const char* uuid_str_Consumed= "6597eeff-4bda-4c1e-af4b-551c4cf74769";
const char* uuid_str_tempK= "6597edec-4bda-4c1e-af4b-551c4cf74769";
const char* uuid_str_t2go= "65970ffe-4bda-4c1e-af4b-551c4cf74769";

#define MINS_PER_HOUR ((int16_t)(60 ))
#define MINS_PER_DAY  ((int16_t)(MINS_PER_HOUR * 24 ))

static  void calcDuration(int16_t minutesIn, int16_t *days, int16_t *hours, int16_t *mins ){
    uint16_t remainingMinutes = minutesIn;
    *days =  (remainingMinutes/ MINS_PER_DAY);
    remainingMinutes = minutesIn - (*days * MINS_PER_DAY);
    *hours =  (remainingMinutes / MINS_PER_HOUR) ;
    remainingMinutes = minutesIn -  ( (*days * MINS_PER_DAY) + (*hours * MINS_PER_HOUR));
    *mins = remainingMinutes;
}


static void read_shunt(gatt_connection_t* connection,  void* user_data) {
    if (connection != NULL) {
        uuid_t uuid;
        int err = GATTLIB_SUCCESS;
        size_t len;
        uint8_t *buffer = NULL;
        double Cval, Vval, Pval, Eval, Dval; 
        //printf("Connected \n");

        // get SOC
        if ( gattlib_string_to_uuid(uuid_str_SOC, strlen(uuid_str_SOC) + 1, &uuid) == 0){
            err =  gattlib_read_char_by_uuid(connection, &uuid, (void **)&buffer, &len);
            if(err == GATTLIB_SUCCESS){
                int16_t i = (buffer[1] << 8) | buffer[0];
                Cval = (double) (i) / 100.0;
                printf("%s:\t%0.0f%%\n", "Charged", Cval);
                free(buffer);
            }
         }

        // Time remaining
        if ( gattlib_string_to_uuid(uuid_str_t2go, strlen(uuid_str_t2go) + 1, &uuid) == 0){
            err =  gattlib_read_char_by_uuid(connection, &uuid, (void **)&buffer, &len);
            if(err == GATTLIB_SUCCESS){
                int16_t i = (buffer[1] << 8) | buffer[0];
                if(i == -1)
                    printf("%s:\t%s\n", "Time left", "Infinite");
                else{ 
                    int16_t  days, hours, mins;
                    calcDuration(i, &days, &hours, &mins);
                    //printf("%02x %02x  " , buffer[0],buffer[1]);
                    printf("%s:\t","Time left");
                    if(days > 0) printf("%d Days ",days);
                    if(hours > 0) printf("%d Hours ", hours);
                    printf("%d Minutes ", mins);
                    printf("\n");
                }
                free(buffer);
            }
         }

        // Get Battery-bank Voltage
        if ( gattlib_string_to_uuid(uuid_str_Vbank, strlen(uuid_str_Vbank) + 1, &uuid) == 0){
            err =  gattlib_read_char_by_uuid(connection, &uuid, (void **)&buffer, &len);
            if(err == GATTLIB_SUCCESS){
                int16_t i = (buffer[1] << 8) | buffer[0];
                Vval = (double) (i) / 100.0;
                printf("%s:\t%0.2fV\n", "Voltage", Vval);
                free(buffer);
            }
         }

/* Get Battery Current not correct for small values 
     if ( gattlib_string_to_uuid(uuid_str_A, strlen(uuid_str_A) + 1, &uuid) == 0){
            err =  gattlib_read_char_by_uuid(connection, &uuid, (void **)&buffer, &len);
            if(err == GATTLIB_SUCCESS){
                uint16_t i = (buffer[1] << 8 ) | buffer[0];
                if(buffer[1]==255) i=0;
                Ival = (double) (i) / 1000.0;
                printf("%s:\t%0.2fA %d\n", "Current", Ival,buffer[1]);
                free(buffer);
            }
         }
*/

        // Get Battery Power
        if ( gattlib_string_to_uuid(uuid_str_P, strlen(uuid_str_P) + 1, &uuid) == 0){
            err =  gattlib_read_char_by_uuid(connection, &uuid, (void **)&buffer, &len);
            if(err == GATTLIB_SUCCESS){
                int16_t i = (buffer[1] << 8) | buffer[0];
                Pval = (double) (i);
                printf("%s:\t%0.2fA\n", "Current", Pval/Vval);
                printf("%s:\t\t%0.2fW\n", "Power", Pval);
                free(buffer);
            }
         }
         // Consumed Ah
        if ( gattlib_string_to_uuid(uuid_str_Consumed, strlen(uuid_str_Consumed) + 1, &uuid) == 0){
            err =  gattlib_read_char_by_uuid(connection, &uuid, (void **)&buffer, &len);
            if(err == GATTLIB_SUCCESS){
                int16_t i = (buffer[1] << 8) | buffer[0];
                Eval = (double) (i) / 10.0;
                    printf("%s:\t%0.2fAh\n", "Consumed", Eval);
                free(buffer);
            }
         }

/*

        // Temperaure in Kelvin Ah
        if ( gattlib_string_to_uuid(uuid_str_tempK, strlen(uuid_str_tempK) + 1, &uuid) == 0){
             err =  gattlib_read_char_by_uuid(connection, &uuid, (void **)&buffer, &len);
             if(err == GATTLIB_SUCCESS){
                int16_t i = (buffer[1] << 8) | buffer[0];
                Tval = (double) (i) / 100.0;
                Tval = (Tval - 273.15)* 1.8000 + 32.00;
                printf("%s:\t%0.2f°F\n", "Bat Temp", Tval);
                free(buffer);
             }
         }
*/

        // Get Midpoint-Deviation
        if ( gattlib_string_to_uuid(uuid_str_Deviation, strlen(uuid_str_Deviation) + 1, &uuid) == 0){
             err =  gattlib_read_char_by_uuid(connection, &uuid, (void **)&buffer, &len);
             if(err == GATTLIB_SUCCESS){
                int16_t i = (buffer[1] << 8) | buffer[0];
                Dval = (double) (i) / 10.0;
                printf("%s:\t%0.1f%\n", "Deviation", Dval);
                printf("%s:\t%0.2fV\n", "Battery 1", Vval*0.5*(1+Dval/100.0));
                printf("%s:\t%0.2fV\n", "Battery 2", Vval*0.5*(1-Dval/100.0));
                free(buffer);
             }
          }
    }
    g_operation_completed = true;
}

int main(int argc, char *arg[]) {
    gatt_connection_t* con;
    const char* deviceID = "F3:E5:DF:E5:A3:25";
    g_operation_completed = false;
    con = gattlib_connect_async(NULL, deviceID, BDADDR_LE_RANDOM,  read_shunt, 0);
    if (con == NULL) {
            fprintf(stderr, "Fail to connect to the bluetooth device %s.\n",deviceID);
            return 1;
        } else {
            while (!g_operation_completed);
            gattlib_disconnect(con);
            //printf("Disconnected \n");
        }
    return 0;
}
