#include "esp_modbus_common.h"
#include "esp_modbus_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mdns.h"
#include "protocol_examples_common.h"
#include "modbus_params.h"  // for modbus parameters structures
#include "mbcontroller.h"
#include "sdkconfig.h"
// define zone: khoi tao cac bien trong chuong trinh
#define MASTER_MAX_CIDS num_device_parameters

// Number of reading of parameters from slave
#define MASTER_MAX_RETRY                (30)

// Timeout to update cid over Modbus
#define UPDATE_CIDS_TIMEOUT_MS          (500)
#define UPDATE_CIDS_TIMEOUT_TICS        (UPDATE_CIDS_TIMEOUT_MS / portTICK_RATE_MS)

// Timeout between polls
#define POLL_TIMEOUT_MS                 (1)
#define POLL_TIMEOUT_TICS               (POLL_TIMEOUT_MS / portTICK_RATE_MS)
#define MB_MDNS_PORT                    (502)

#define MASTER_TAG "MASTER_TEST"

// The macro to get offset for parameter in the appropriate structure
#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) + 1))
#define INPUT_OFFSET(field) ((uint16_t)(offsetof(input_reg_params_t, field) + 1))
#define COIL_OFFSET(field) ((uint16_t)(offsetof(coil_reg_params_t, field) + 1))
#define DISCR_OFFSET(field) ((uint16_t)(offsetof(discrete_reg_params_t, field) + 1))
#define STR(fieldname) ((const char*)( fieldname ))

// Options can be used as bit masks or parameter limits
#define OPTS(min_val, max_val, step_val) { .opt1 = min_val, .opt2 = max_val, .opt3 = step_val }

#define MB_ID_BYTE0(id) ((uint8_t)(id))
#define MB_ID_BYTE1(id) ((uint8_t)(((uint16_t)(id) >> 8) & 0xFF))
#define MB_ID_BYTE2(id) ((uint8_t)(((uint32_t)(id) >> 16) & 0xFF))
#define MB_ID_BYTE3(id) ((uint8_t)(((uint32_t)(id) >> 24) & 0xFF))

#define MB_ID2STR(id) MB_ID_BYTE0(id), MB_ID_BYTE1(id), MB_ID_BYTE2(id), MB_ID_BYTE3(id)

#if CONFIG_FMB_CONTROLLER_SLAVE_ID_SUPPORT
#define MB_DEVICE_ID (uint32_t)CONFIG_FMB_CONTROLLER_SLAVE_ID
#else
#define MB_DEVICE_ID (uint32_t)0x00112233
#endif

#define MB_MDNS_INSTANCE(pref) pref"mb_master_tcp"

//coding zone

// Định nghĩa địa chỉ của các slave được master truy cập
    enum {
        MB_DEVICE_ADDR1  = 1, 
        MB_DEVICE_ADDR2  = 2, 
        MB_DEVICE_ADDR3  = 3,
        MB_DEVICE_ADDR4  = 4,
        MB_DEVICE_ADDR5  = 5,
        MB_DEVICE_ADDR6  = 6,
        MB_DEVICE_ADDR7  = 7,
        MB_DEVICE_ADDR8  = 8,
        MB_DEVICE_ADDR9  = 9,
        MB_DEVICE_ADDR10 = 10,
        MB_DEVICE_ADDR11 = 11,
        MB_DEVICE_ADDR12 = 12,
        MB_DEVICE_ADDR13 = 13,
        MB_DEVICE_ADDR14 = 14,
        MB_DEVICE_ADDR15 = 15,
        MB_DEVICE_ADDR16 = 16,
        MB_SLAVE_COUNT 
        };
//Định nghĩa CID
    enum {
CID_XAXIS_1 = 0,
CID_YAXIS_1 = 1,
CID_ZAXYS_1 = 2,
CID_TEMP_1  = 3,

CID_XAXIS_2 = 4,
CID_YAXIS_2 = 5,
CID_ZAXYS_2 = 6,
CID_TEMP_2  = 7,

CID_XAXIS_3 = 8,
CID_YAXIS_3 = 9,
CID_ZAXYS_3 = 10,
CID_TEMP_3  = 11,

CID_XAXIS_4 = 12,
CID_YAXIS_4 = 13,
CID_ZAXYS_4 = 14,
CID_TEMP_4  = 15,

CID_XAXIS_5 = 16,
CID_YAXIS_5 = 17,
CID_ZAXYS_5 = 18,
CID_TEMP_5  = 19,

CID_XAXIS_6 = 20,
CID_YAXIS_6 = 21,
CID_ZAXYS_6 = 22,
CID_TEMP_6  = 23,

CID_XAXIS_7 = 24,
CID_YAXIS_7 = 25,
CID_ZAXYS_7 = 26,
CID_TEMP_7  = 27,

CID_XAXIS_8 = 28,
CID_YAXIS_8 = 29,
CID_ZAXYS_8 = 30,
CID_TEMP_8  = 31,

CID_XAXIS_9 = 32,
CID_YAXIS_9 = 33,
CID_ZAXYS_9 = 34,
CID_TEMP_9  = 35,

CID_XAXIS_10 = 36,
CID_YAXIS_10 = 37,
CID_ZAXYS_10 = 38,
CID_TEMP_10  = 39,

CID_XAXIS_11 = 40,
CID_YAXIS_11 = 41,
CID_ZAXYS_11 = 42,
CID_TEMP_11  = 43,

CID_XAXIS_12 = 44,
CID_YAXIS_12 = 45,
CID_ZAXYS_12 = 46,
CID_TEMP_12  = 47,

CID_XAXIS_13 = 48,
CID_YAXIS_13 = 49,
CID_ZAXYS_13 = 50,
CID_TEMP_13  = 51,

CID_XAXIS_14 = 52,
CID_YAXIS_14 = 53,
CID_ZAXYS_14 = 54,
CID_TEMP_14  = 55,

CID_XAXIS_15 = 56,
CID_YAXIS_15 = 57,
CID_ZAXYS_15 = 58,
CID_TEMP_15  = 59,

CID_XAXIS_16 = 60,
CID_YAXIS_16 = 61,
CID_ZAXYS_16 = 62,
CID_TEMP_16  = 63,

};
// tọa data dictionary cho modbus master
mb_parameter_descriptor_t device_parameters[] = {
    // CID,                Tên,                      Đơn vị,  Địa chỉ Modbus, Loại thanh ghi, Địa chỉ bắt đầu, Độ dài, Offset, Kiểu dữ liệu, Kích thước dữ liệu, Tùy chọn,                 Quyền truy cập
    { CID_XAXIS_1,         STR("Register_1_Slave_1"), STR("--"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0,          1,      0,      PARAM_TYPE_FLOAT, 4,               OPTS(-180, 180, 1),   PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_YAXIS_1,         STR("Register_2_Slave_1"), STR("--"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 1,          1,      2,      PARAM_TYPE_FLOAT, 4,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_ZAXYS_1,         STR("Register_3_Slave_1"), STR("--"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 2,          1,      4,      PARAM_TYPE_FLOAT, 4,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_TEMP_1,          STR("Register_4_Slave_1"), STR("--"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 3,          1,      6,      PARAM_TYPE_FLOAT, 4,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    
    { CID_XAXIS_2,         STR("Register_1_Slave_2"), STR("--"), MB_DEVICE_ADDR2, MB_PARAM_HOLDING, 0,          1,      8,      PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_YAXIS_2,         STR("Register_2_Slave_2"), STR("--"), MB_DEVICE_ADDR2, MB_PARAM_HOLDING, 1,          1,      10,     PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_ZAXYS_2,         STR("Register_3_Slave_2"), STR("--"), MB_DEVICE_ADDR2, MB_PARAM_HOLDING, 2,          1,      12,     PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_TEMP_2,          STR("Register_4_Slave_2"), STR("--"), MB_DEVICE_ADDR2, MB_PARAM_HOLDING, 3,          1,      14,     PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },

    { CID_XAXIS_3,         STR("Register_1_Slave_3"), STR("--"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING, 0,          1,      16,     PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_YAXIS_3,         STR("Register_2_Slave_3"), STR("--"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING, 1,          1,      18,     PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_ZAXYS_3,         STR("Register_3_Slave_3"), STR("--"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING, 2,          1,      20,     PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_TEMP_3,          STR("Register_4_Slave_3"), STR("--"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING, 3,          1,      22,     PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },

    // Tiếp tục cho đến CID_TEMP_16 theo cùng cấu trúc...
    
    { CID_XAXIS_16,        STR("Register_1_Slave_16"), STR("--"), MB_DEVICE_ADDR16, MB_PARAM_HOLDING, 0,        1,      60,     PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_YAXIS_16,        STR("Register_2_Slave_16"), STR("--"), MB_DEVICE_ADDR16, MB_PARAM_HOLDING, 1,        1,      62,     PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_ZAXYS_16,        STR("Register_3_Slave_16"), STR("--"), MB_DEVICE_ADDR16, MB_PARAM_HOLDING, 2,        1,      64,     PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_TEMP_16,         STR("Register_4_Slave_16"), STR("--"), MB_DEVICE_ADDR16, MB_PARAM_HOLDING, 3,        1,      66,     PARAM_TYPE_FLOAT, 2,               OPTS(0, 0, 0),        PAR_PERMS_READ_WRITE_TRIGGER }
};
