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
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "LittleFS.h"
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
#define MB_SLAVE_COUNT 16
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
        MB_DEVICE_ADDR
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
    // CID,                Tên,                      Đơn vị,  Địa chỉ Modbus, Loại thanh ghi, Địa chỉ bắt đầu, Độ dài, Offset,  Kiểu dữ liệu,     Kích thước dữ liệu,                             Quyền truy cập
    { CID_XAXIS_1,         STR("Register_1_Slave_1"), STR("Hz"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0,          1,      0,      PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_YAXIS_1,         STR("Register_2_Slave_1"), STR("Hz"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 1,          1,      2,      PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_ZAXYS_1,         STR("Register_3_Slave_1"), STR("Hz"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 2,          1,      4,      PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_TEMP_1,          STR("Register_4_Slave_1"), STR("C"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING,  3,           1,      6,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    
    { CID_XAXIS_2,         STR("Register_1_Slave_2"), STR("Hz"), MB_DEVICE_ADDR2, MB_PARAM_HOLDING, 4,          1,      8,      PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_YAXIS_2,         STR("Register_2_Slave_2"), STR("Hz"), MB_DEVICE_ADDR2, MB_PARAM_HOLDING, 5,          1,      10,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_ZAXYS_2,         STR("Register_3_Slave_2"), STR("Hz"), MB_DEVICE_ADDR2, MB_PARAM_HOLDING, 6,          1,      12,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_TEMP_2,          STR("Register_4_Slave_2"), STR("C"), MB_DEVICE_ADDR2, MB_PARAM_HOLDING,  7,          1,      14,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },

    { CID_XAXIS_3,         STR("Register_1_Slave_3"), STR("Hz"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING, 8,          1,       16,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_YAXIS_3,         STR("Register_2_Slave_3"), STR("Hz"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING, 9,          1,       18,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_ZAXYS_3,         STR("Register_3_Slave_3"), STR("Hz"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING, 10,          1,      20,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_TEMP_3,          STR("Register_4_Slave_3"), STR("C"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING,  11,          1,      22,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },

    { CID_XAXIS_4,         STR("Register_1_Slave_4"), STR("Hz"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING, 12,          1,      24,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_YAXIS_4,         STR("Register_2_Slave_4"), STR("Hz"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING, 13,          1,      26,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_ZAXYS_4,         STR("Register_3_Slave_4"), STR("Hz"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING, 14,          1,      28,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_TEMP_4,          STR("Register_4_Slave_4"), STR("C"), MB_DEVICE_ADDR3, MB_PARAM_HOLDING,  15,          1,      30,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                      PAR_PERMS_READ_WRITE_TRIGGER },
    // Tiếp tục cho đến CID_TEMP_16 theo cùng cấu trúc...
    
    { CID_XAXIS_16,        STR("Register_1_Slave_16"), STR("--"), MB_DEVICE_ADDR16, MB_PARAM_HOLDING, 16,        1,      60,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                       PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_YAXIS_16,        STR("Register_2_Slave_16"), STR("--"), MB_DEVICE_ADDR16, MB_PARAM_HOLDING, 17,        1,      62,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                       PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_ZAXYS_16,        STR("Register_3_Slave_16"), STR("--"), MB_DEVICE_ADDR16, MB_PARAM_HOLDING, 18,        1,      64,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                       PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_TEMP_16,         STR("Register_4_Slave_16"), STR("--"), MB_DEVICE_ADDR16, MB_PARAM_HOLDING, 19,        1,      66,     PARAM_TYPE_FLOAT, PARAM_MAX_SIZE,                       PAR_PERMS_READ_WRITE_TRIGGER },
};

const uint16_t num_device_parameters = (sizeof(device_parameters)/sizeof(device_parameters[0]));
// Ip table for 16 slaves
char* slave_ip_address_table[MB_SLAVE_COUNT] = {
#if CONFIG_MB_SLAVE_IP_FROM_STDIN
         
    "192.168.1.11",   // Địa chỉ IP của slave 1
    "192.168.1.12",   // Địa chỉ IP của slave 2
    "192.168.1.13",   // Địa chỉ IP của slave 3
    "192.168.1.14",   // Địa chỉ IP của slave 4
    "192.168.1.15",   // Địa chỉ IP của slave 5
    "192.168.1.16",   // Địa chỉ IP của slave 6
    "192.168.1.17",   // Địa chỉ IP của slave 7
    "192.168.1.18",   // Địa chỉ IP của slave 8
    "192.168.1.19",   // Địa chỉ IP của slave 9
    "192.168.1.20",   // Địa chỉ IP của slave 10
    "192.168.1.21",   // Địa chỉ IP của slave 11
    "192.168.1.22",   // Địa chỉ IP của slave 12
    "192.168.1.23",   // Địa chỉ IP của slave 13
    "192.168.1.24",   // Địa chỉ IP của slave 14
    "192.168.1.25",   // Địa chỉ IP của slave 15
    "192.168.1.26",   // Địa chỉ IP của slave 16    
    NULL
#elif CONFIG_MB_MDNS_IP_RESOLVER
    NULL,
    NULL
#endif
};

// Scan slaves address
#if CONFIG_MB_SLAVE_IP_FROM_STDIN
char* master_scan_addr(int* index, char* buffer) {
  char* ip_str = NULL;
  unsigned int a[1] = {0};
  int buf_cnt = 0;

#if !CONFIG_EXAMPLE_CONNECT_IPV6
  buf_cnt = sscanf(buffer, "IP%d="IPSTR, index, &a, &a[2], &a[3], &a[4]);
  if (buf_cnt == 5 && *index < MB_SLAVE_COUNT) {
    if (-1 == asprintf(&ip_str, IPSTR, a, a[2], a[3], a[4])) {
      abort();
    }
  }
#else
  buf_cnt = sscanf(buffer, "IP%d="IPV6STR, index, &a, &a[2], &a[3], &a[4], &a[5], &a[6], &a[7], &a[8]);
  if (buf_cnt == 9 && *index < MB_SLAVE_COUNT) {
    if (-1 == asprintf(&ip_str, IPV6STR, a, a[2], a[3], a[4], a[5], a[6], a[7], a[8])) {
      abort();
    }
  }
#endif

  return ip_str;
}


static int master_get_slave_ip_stdin(char** addr_table)
{
    char buf[128];
    int index;
    char* ip_str = NULL;
    int buf_cnt = 0;
    int ip_cnt = 0;

    if (!addr_table) {
        return 0;
    }

    ESP_ERROR_CHECK(example_configure_stdin_stdout());
    while(1) {
        if (addr_table[ip_cnt] && strcmp(addr_table[ip_cnt], "FROM_STDIN") == 0) {
            printf("Waiting IP%d from stdin:\r\n", ip_cnt);
            while (fgets(buf, sizeof(buf), stdin) == NULL) {
                fputs(buf, stdout);
            }
            buf_cnt = strlen(buf);
            buf[buf_cnt - 1] = '\0';
            fputc('\n', stdout);
            ip_str = master_scan_addr(&index, buf);
            if (ip_str != NULL) {
                ESP_LOGI(MASTER_TAG, "IP(%d) = [%s] set from stdin.", ip_cnt, ip_str);
                if ((ip_cnt >= MB_SLAVE_COUNT) || (index != ip_cnt)) {
                    addr_table[ip_cnt] = NULL;
                    break;
                }
                addr_table[ip_cnt++] = ip_str;
            } else {
                // End of configuration
                addr_table[ip_cnt++] = NULL;
                break;
            }
        } else {
            if (addr_table[ip_cnt]) {
                ESP_LOGI(MASTER_TAG, "Leave IP(%d) = [%s] set manually.", ip_cnt, addr_table[ip_cnt]);
                ip_cnt++;
            } else {
                ESP_LOGI(MASTER_TAG, "IP(%d) is not set in the table.", ip_cnt);
                break;
            }
        }
    }
    return ip_cnt;
}
#elif CONFIG_MB_MDNS_IP_RESOLVER

// convert MAC from binary format to string
static inline char* gen_mac_str(const uint8_t* mac, char* pref, char* mac_str)
{
    sprintf(mac_str, "%s%02X%02X%02X%02X%02X%02X", pref, MAC2STR(mac));
    return mac_str;
}

static inline char* gen_id_str(char* service_name, char* slave_id_str)
{
    sprintf(slave_id_str, "%s%02X%02X%02X%02X", service_name, MB_ID2STR(MB_DEVICE_ID));
    return slave_id_str;
}

static void master_start_mdns_service()
{
    char temp_str[32] = {0};
    uint8_t sta_mac[6] = {0};
    ESP_ERROR_CHECK(esp_read_mac(sta_mac, ESP_MAC_WIFI_STA));
    char* hostname = gen_mac_str(sta_mac, MB_MDNS_INSTANCE("")"_", temp_str);
    // initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
    // set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK(mdns_hostname_set(hostname));
    ESP_LOGI(MASTER_TAG, "mdns hostname set to: [%s]", hostname);

    // set default mDNS instance name
    ESP_ERROR_CHECK(mdns_instance_name_set(MB_MDNS_INSTANCE("esp32_")));

    // structure with TXT records
    mdns_txt_item_t serviceTxtData[] = {
        {"board","esp32"}
    };

    // initialize service
    ESP_ERROR_CHECK(mdns_service_add(MB_MDNS_INSTANCE(""), "_modbus", "_tcp", MB_MDNS_PORT, serviceTxtData, 1));
    // add mac key string text item
    ESP_ERROR_CHECK(mdns_service_txt_item_set("_modbus", "_tcp", "mac", gen_mac_str(sta_mac, "\0", temp_str)));
    // add slave id key txt item
    ESP_ERROR_CHECK( mdns_service_txt_item_set("_modbus", "_tcp", "mb_id", gen_id_str("\0", temp_str)));
}

static char* master_get_slave_ip_str(mdns_ip_addr_t* address, mb_tcp_addr_type_t addr_type)
{
    mdns_ip_addr_t* a = address;
    char* slave_ip_str = NULL;

    while (a) {
        if ((a->addr.type == ESP_IPADDR_TYPE_V6) && (addr_type == MB_IPV6)) {
            if (-1 == asprintf(&slave_ip_str, IPV6STR, IPV62STR(a->addr.u_addr.ip6))) {
                abort();
            }
        } else if ((a->addr.type == ESP_IPADDR_TYPE_V4) && (addr_type == MB_IPV4)) {
            if (-1 == asprintf(&slave_ip_str, IPSTR, IP2STR(&(a->addr.u_addr.ip4)))) {
                abort();
            }
        }
        if (slave_ip_str) {
            break;
        }
        a = a->next;
    }
    return slave_ip_str;
}

static esp_err_t master_resolve_slave(const char* name, mdns_result_t* result, char** resolved_ip,
                                        mb_tcp_addr_type_t addr_type)
{
    if (!name || !result) {
        return ESP_ERR_INVALID_ARG;
    }
    mdns_result_t* r = result;
    int t;
    char* slave_ip = NULL;
    for (; r ; r = r->next) {
        if ((r->ip_protocol == MDNS_IP_PROTOCOL_V4) && (addr_type == MB_IPV6)) {
            continue;
        } else if ((r->ip_protocol == MDNS_IP_PROTOCOL_V6) && (addr_type == MB_IPV4)) {
            continue;
        }
        // Check host name for Modbus short address and
        // append it into slave ip address table
        if ((strcmp(r->instance_name, name) == 0) && (r->port == CONFIG_FMB_TCP_PORT_DEFAULT)) {
            printf("  PTR : %s\n", r->instance_name);
            if (r->txt_count) {
                printf("  TXT : [%u] ", r->txt_count);
                for ( t = 0; t < r->txt_count; t++) {
                    printf("%s=%s; ", r->txt[t].key, r->txt[t].value?r->txt[t].value:"NULL");
                }
                printf("\n");
            }
            slave_ip = master_get_slave_ip_str(r->addr, addr_type);
            if (slave_ip) {
                ESP_LOGI(MASTER_TAG, "Resolved slave %s[%s]:%u", r->hostname, slave_ip, r->port);
                *resolved_ip = slave_ip;
                return ESP_OK;
            }
        }
    }
    *resolved_ip = NULL;
    ESP_LOGD(MASTER_TAG, "Fail to resolve slave: %s", name);
    return ESP_ERR_NOT_FOUND;
}

static int master_create_slave_list(mdns_result_t* results, char** addr_table,
                                        mb_tcp_addr_type_t addr_type)
{
    if (!results) {
        return -1;
    }
    int i, addr, resolved = 0;
    const mb_parameter_descriptor_t* pdescr = &device_parameters[0];
    char** ip_table = addr_table;
    char slave_name[22] = {0};
    char* slave_ip = NULL;

    for (i = 0; (i < num_device_parameters && pdescr); i++, pdescr++) {
        addr = pdescr->mb_slave_addr;
        if (-1 == sprintf(slave_name, "mb_slave_tcp_%02X", addr)) {
            ESP_LOGI(MASTER_TAG, "Fail to create instance name for index: %d", addr);
            abort();
        }
        if (!ip_table[addr - 1]) {
            esp_err_t err = master_resolve_slave(slave_name, results, &slave_ip, addr_type);
            if (err != ESP_OK) {
                ESP_LOGE(MASTER_TAG, "Index: %d, sl_addr: %d, name:%s, failed to resolve!",
                                        i, addr, slave_name);
                // Set correspond index to NULL indicate host not resolved
                ip_table[addr - 1] = NULL;
                continue;
            }
            ip_table[addr - 1] = slave_ip; //slave_name;
            ESP_LOGI(MASTER_TAG, "Index: %d, sl_addr: %d, name:%s, resolve to IP: [%s]",
                                    i, addr, slave_name, slave_ip);
            resolved++;
        } else {
            ESP_LOGI(MASTER_TAG, "Index: %d, sl_addr: %d, name:%s, set to IP: [%s]",
                                    i, addr, slave_name, ip_table[addr - 1]);
            resolved++;
        }
    }
    return resolved;
}

static void master_destroy_slave_list(char** table)
{
    for (int i = 0; ((i < MB_DEVICE_COUNT) && table[i] != NULL); i++) {
        if (table[i]) {
            free(table[i]);
            table[i] = NULL;
        }
    }
}

static int master_query_slave_service(const char * service_name, const char * proto,
                                        mb_tcp_addr_type_t addr_type)
{
    ESP_LOGI(MASTER_TAG, "Query PTR: %s.%s.local", service_name, proto);

    mdns_result_t* results = NULL;
    int count = 0;

    esp_err_t err = mdns_query_ptr(service_name, proto, 3000, 20, &results);
    if(err){
        ESP_LOGE(MASTER_TAG, "Query Failed: %s", esp_err_to_name(err));
        return count;
    }
    if(!results){
        ESP_LOGW(MASTER_TAG, "No results found!");
        return count;
    }

    count = master_create_slave_list(results, slave_ip_address_table, addr_type);

    mdns_query_results_free(results);
    return count;
}
#endif



// The function to get pointer to parameter storage (instance) according to parameter description table
static void* master_get_param_data(const mb_parameter_descriptor_t* param_descriptor)
{
    assert(param_descriptor != NULL);
    void* instance_ptr = NULL;
    if (param_descriptor->param_offset != 0) {
       switch(param_descriptor->mb_param_type)
       {
           case MB_PARAM_HOLDING:
               instance_ptr = ((int*)&holding_reg_params + param_descriptor->param_offset - 1);
               break;
           
           default:
               instance_ptr = NULL;
               break;
       }
    } else {
        ESP_LOGE(MASTER_TAG, "Wrong parameter offset for CID #%d", param_descriptor->cid);
        assert(instance_ptr != NULL);
    }
    return instance_ptr;
}


// User operation function to read slave values and check alarm
static void master_operation_func(void *arg)
{
    esp_err_t err = ESP_OK;
    float value = 0;
    bool alarm_state = false;
    const mb_parameter_descriptor_t* param_descriptor = NULL;

    ESP_LOGI(MASTER_TAG, "Start modbus test...");

    for(uint16_t retry = 0; retry <= MASTER_MAX_RETRY && (!alarm_state); retry++) {
        // Read all found characteristics from slave(s)
        for (uint16_t cid = 0; (err != ESP_ERR_NOT_FOUND) && cid < MASTER_MAX_CIDS; cid++)
        {
            // Get data from parameters description table
            // and use this information to fill the characteristics description table
            // and having all required fields in just one table
            err = mbc_master_get_cid_info(cid, &param_descriptor);
            if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL)) {
                void* temp_data_ptr = master_get_param_data(param_descriptor);
                assert(temp_data_ptr);
                uint8_t type = 0;
                err = mbc_master_get_parameter(cid, (char*)param_descriptor->param_key,
                                                    (uint8_t*)&value, &type);
                if (err == ESP_OK) {
                    *(float*)temp_data_ptr = value;
                    if ((param_descriptor->mb_param_type == MB_PARAM_HOLDING) ||
                        (param_descriptor->mb_param_type == MB_PARAM_INPUT)) {
                        ESP_LOGI(MASTER_TAG, "Characteristic #%d %s (%s) value = %f (0x%x) read successful.",
                                        param_descriptor->cid,
                                        (char*)param_descriptor->param_key,
                                        (char*)param_descriptor->param_units,
                                        value,
                                        *(uint32_t*)temp_data_ptr);

                    } else {
                        uint16_t state = *(uint16_t*)temp_data_ptr;
                        ESP_LOGI(MASTER_TAG, "Characteristic #%d %s (%s) value = %s (0x%x) read successful.",
                                        param_descriptor->cid,
                                        (char*)param_descriptor->param_key,
                                        (char*)param_descriptor->param_units,
                                        (const char*)rw_str,
                                        *(uint16_t*)temp_data_ptr);
                        
                    }
                } else {
                    ESP_LOGE(MASTER_TAG, "Characteristic #%d (%s) read fail, err = %d (%s).",
                                        param_descriptor->cid,
                                        (char*)param_descriptor->param_key,
                                        (int)err,
                                        (char*)esp_err_to_name(err));
                }
                vTaskDelay(POLL_TIMEOUT_TICS); // timeout between polls
            }
        }
        vTaskDelay(UPDATE_CIDS_TIMEOUT_TICS);
    }

    if (alarm_state) {
        ESP_LOGI(MASTER_TAG, "Alarm triggered by cid #%d.",
                                        param_descriptor->cid);
    } else {
        ESP_LOGE(MASTER_TAG, "Alarm is not triggered after %d retries.",
                                        MASTER_MAX_RETRY);
    }
    ESP_LOGI(MASTER_TAG, "Destroy master...");
    vTaskDelay(100);
}

static esp_err_t init_services(mb_tcp_addr_type_t ip_addr_type)
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      result = nvs_flash_init();
    }
    ESP_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
                            MASTER_TAG,
                            "nvs_flash_init fail, returns(0x%x).",
                            (uint32_t)result);
    result = esp_netif_init();
    ESP_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
                            MASTER_TAG,
                            "esp_netif_init fail, returns(0x%x).",
                            (uint32_t)result);
    result = esp_event_loop_create_default();
    ESP_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
                            MASTER_TAG,
                            "esp_event_loop_create_default fail, returns(0x%x).",
                            (uint32_t)result);
#if CONFIG_MB_MDNS_IP_RESOLVER
    // Start mdns service and register device
    master_start_mdns_service();
#endif
    // This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
    // Read "Establishing Wi-Fi or Ethernet Connection" section in
    // examples/protocols/README.md for more information about this function.
    result = example_connect();
    ESP_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
                                MASTER_TAG,
                                "example_connect fail, returns(0x%x).",
                                (uint32_t)result);
#if CONFIG_EXAMPLE_CONNECT_WIFI
   result = esp_wifi_set_ps(WIFI_PS_NONE);
   ESP_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
                                   MASTER_TAG,
                                   "esp_wifi_set_ps fail, returns(0x%x).",
                                   (uint32_t)result);
#endif
#if CONFIG_MB_MDNS_IP_RESOLVER
    int res = 0;
    for (int retry = 0; (res < num_device_parameters) && (retry < 10); retry++) {
        res = master_query_slave_service("_modbus", "_tcp", ip_addr_type);
    }
    if (res < num_device_parameters) {
        ESP_LOGE(MASTER_TAG, "Could not resolve one or more slave IP addresses, resolved: %d out of %d.", res, num_device_parameters );
        ESP_LOGE(MASTER_TAG, "Make sure you configured all slaves according to device parameter table and they alive in the network.");
        return ESP_ERR_NOT_FOUND;
    }
    mdns_free();
#elif CONFIG_MB_SLAVE_IP_FROM_STDIN
    int ip_cnt = master_get_slave_ip_stdin(slave_ip_address_table);
    if (ip_cnt) {
        ESP_LOGI(MASTER_TAG, "Configured %d IP addresse(s).", ip_cnt);
    } else {
        ESP_LOGE(MASTER_TAG, "Fail to get IP address from stdin. Continue.");
        return ESP_ERR_NOT_FOUND;
    }
#endif
    return ESP_OK;
}

static esp_err_t destroy_services(void)
{
    esp_err_t err = ESP_OK;
#if CONFIG_MB_MDNS_IP_RESOLVER
    master_destroy_slave_list(slave_ip_address_table);
#endif
    err = example_disconnect();
    ESP_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                   MASTER_TAG,
                                   "example_disconnect fail, returns(0x%x).",
                                   (uint32_t)err);
    err = esp_event_loop_delete_default();
    ESP_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                       MASTER_TAG,
                                       "esp_event_loop_delete_default fail, returns(0x%x).",
                                       (uint32_t)err);
    err = esp_netif_deinit();
    ESP_RETURN_ON_FALSE((err == ESP_OK || err == ESP_ERR_NOT_SUPPORTED), ESP_ERR_INVALID_STATE,
                                        MASTER_TAG,
                                        "esp_netif_deinit fail, returns(0x%x).",
                                        (uint32_t)err);
    err = nvs_flash_deinit();
    ESP_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                MASTER_TAG,
                                "nvs_flash_deinit fail, returns(0x%x).",
                                (uint32_t)err);
    return err;
}

// Modbus master initialization
static esp_err_t master_init(mb_communication_info_t* comm_info)
{
    void* master_handler = NULL;

    esp_err_t err = mbc_master_init_tcp(&master_handler);
    ESP_RETURN_ON_FALSE((master_handler != NULL), ESP_ERR_INVALID_STATE,
                                MASTER_TAG,
                                "mb controller initialization fail.");
    ESP_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                            MASTER_TAG,
                            "mb controller initialization fail, returns(0x%x).",
                            (uint32_t)err);

    err = mbc_master_setup((void*)comm_info);
    ESP_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                            MASTER_TAG,
                            "mb controller setup fail, returns(0x%x).",
                            (uint32_t)err);

    err = mbc_master_set_descriptor(&device_parameters[0], num_device_parameters);
    ESP_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                MASTER_TAG,
                                "mb controller set descriptor fail, returns(0x%x).",
                                (uint32_t)err);
    ESP_LOGI(MASTER_TAG, "Modbus master stack initialized...");

    err = mbc_master_start();
    ESP_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                            MASTER_TAG,
                            "mb controller start fail, returns(0x%x).",
                            (uint32_t)err);
    vTaskDelay(5);
    return err;
}

static esp_err_t master_destroy(void)
{
    esp_err_t err = mbc_master_destroy();
    ESP_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                MASTER_TAG,
                                "mbc_master_destroy fail, returns(0x%x).",
                                (uint32_t)err);
    ESP_LOGI(MASTER_TAG, "Modbus master stack destroy...");
    return err;
}

void app_main(void)
{
    mb_tcp_addr_type_t ip_addr_type;
#if !CONFIG_EXAMPLE_CONNECT_IPV6
    ip_addr_type = MB_IPV4;
#else
    ip_addr_type = MB_IPV6;
#endif
    ESP_ERROR_CHECK(init_services(ip_addr_type));

    mb_communication_info_t comm_info = { MB_MODE_TCP };
    comm_info.ip_port = 502;
    comm_info.ip_addr_type = ip_addr_type;
    comm_info.ip_mode = MB_MODE_TCP;
    comm_info.ip_addr = (void*)slave_ip_address_table;
    //comm_info.ip_netif_ptr = (void*)get_example_netif();

    ESP_ERROR_CHECK(master_init(&comm_info));
    vTaskDelay(10);

    master_operation_func(NULL);
    ESP_ERROR_CHECK(master_destroy());
    ESP_ERROR_CHECK(destroy_services());
}


/*Phan setup wifi cho master
Đầu tiên cần phải cấu hình IP cho master, sau đó ta sẽ khỏi tạo các file dữ liệu cho web nhằm mục đích
sửa địa chỉ và mật khẩu wifi khi mât kết nối 
Link tham khảo: https://randomnerdtutorials.com/esp32-wi-fi-manager-asyncwebserver/*/

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

// Set LED GPIO
const int ledPin = 2;
// Stores LED state

String ledState;

// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Read File from LittleFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to LittleFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());


  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

// Replaces placeholder with LED state value
String processor(const String& var) {
  if(var == "STATE") {
    if(digitalRead(ledPin)) {
      ledState = "ON";
    }
    else {
      ledState = "OFF";
    }
    return ledState;
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  initLittleFS();

  // Set GPIO 2 as an OUTPUT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Load values saved in LittleFS
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  ip = readFile(LittleFS, ipPath);
  gateway = readFile (LittleFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);

  if(initWiFi()) {
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    server.serveStatic("/", LittleFS, "/");
    
    // Route to set GPIO state to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, HIGH);
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    // Route to set GPIO state to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, LOW);
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    server.begin();
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", LittleFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        const AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(LittleFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(LittleFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(LittleFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
}

void loop() {

}