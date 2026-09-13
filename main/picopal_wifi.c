#include "picopal_wifi.h"

#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/ip4_addr.h"
#include "nvs.h"
#include "picopal_events.h"
#include "picopal_timer.h"

#define WIFI_NAMESPACE "wifi"
#define WIFI_SSID_KEY "ssid"
#define WIFI_PASSWORD_KEY "password"
#define WIFI_SETUP_SSID "PicoPal-Setup"
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define WIFI_MAXIMUM_RETRIES 5
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT BIT1
#define CONFIGURE_BODY_SIZE 160

static const char *TAG = "wifi";
static EventGroupHandle_t s_wifi_events;
static httpd_handle_t s_http_server;
static picopal_wifi_mode_t s_mode = PICOPAL_WIFI_MODE_SETUP;
static bool s_connected;
static bool s_station_connecting;
static unsigned s_retry_count;
static char s_ip_address[16] = "0.0.0.0";

static const char s_setup_page[] =
    "<!doctype html><html><head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>PicoPal Setup</title>"
    "<style>body{font-family:system-ui;margin:0;background:#f4f1e8;color:#202020}"
    "main{max-width:420px;margin:12vh auto;padding:28px}"
    "form{display:grid;gap:14px;background:white;padding:24px;border-radius:18px}"
    "input,button{font:inherit;padding:12px;border-radius:10px;border:1px solid #aaa}"
    "button{background:#202020;color:white}</style></head>"
    "<body><main><h1>PicoPal</h1><p>Connect PicoPal to your home Wi-Fi.</p>"
    "<form method='post' action='/configure'>"
    "<label>Wi-Fi name<input name='ssid' maxlength='32' required></label>"
    "<label>Password<input name='password' type='password' maxlength='63'></label>"
    "<button type='submit'>Connect</button></form></main></body></html>";

static const char s_control_page[] =
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>PicoPal</title><style>"
    ":root{font-family:system-ui;color:#202020;background:#f4f1e8}"
    "body{margin:0}main{max-width:520px;margin:auto;padding:24px}"
    "nav{display:flex;gap:8px;margin-bottom:20px}button{font:inherit;border:0;"
    "border-radius:12px;padding:12px 16px;background:#202020;color:white}"
    "nav button{flex:1;background:#ddd;color:#222}nav .on{background:#202020;color:white}"
    ".card{background:white;border-radius:20px;padding:24px}"
    ".time{font:700 56px ui-monospace,monospace;text-align:center;margin:24px 0}"
    ".controls{display:flex;gap:10px;justify-content:center}"
    ".secondary{background:#ddd;color:#222}.hidden{display:none}"
    "canvas{width:100%;image-rendering:pixelated;background:#000;border:1px solid #222}"
    "small{color:#666}</style></head><body><main>"
    "<h1>PicoPal</h1><nav><button id='focusTab' class='on'>Focus</button>"
    "<button id='drawTab'>Drawing</button></nav>"
    "<section id='focus' class='card'><small id='state'>Loading…</small>"
    "<div id='time' class='time'>000:00</div><div class='controls'>"
    "<button id='toggle'>Start</button><button id='reset' class='secondary'>Reset</button>"
    "</div></section><section id='drawing' class='card hidden'>"
    "<canvas id='canvas' width='128' height='64'></canvas>"
    "<p>Drawing tools and upload are added in Lab 3.</p></section>"
    "<p><button id='forget' class='secondary'>Forget Wi-Fi</button></p>"
    "<script>const q=s=>document.querySelector(s);let running=false;"
    "async function refresh(){try{const r=await fetch('/api/timer');const d=await r.json();"
    "running=d.running;q('#time').textContent=d.display;q('#state').textContent="
    "running?'Timer running':'Timer paused';q('#toggle').textContent=running?'Pause':'Start'}"
    "catch(e){q('#state').textContent='Device unavailable'}}"
    "async function post(path){await fetch(path,{method:'POST'});await refresh()}"
    "q('#toggle').onclick=()=>post('/api/timer/toggle');"
    "q('#reset').onclick=()=>post('/api/timer/reset');"
    "q('#forget').onclick=async()=>{if(confirm('Forget Wi-Fi and return to setup mode?'))"
    "await post('/api/wifi/forget')};"
    "function tab(draw){q('#focus').classList.toggle('hidden',draw);"
    "q('#drawing').classList.toggle('hidden',!draw);q('#focusTab').classList.toggle('on',!draw);"
    "q('#drawTab').classList.toggle('on',draw)}"
    "q('#focusTab').onclick=()=>tab(false);q('#drawTab').onclick=()=>tab(true);"
    "refresh();setInterval(refresh,500)</script></main></body></html>";

static void wifi_event_handler(
    void *context,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    (void)context;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED &&
        s_station_connecting) {
        s_connected = false;
        if (s_retry_count < WIFI_MAXIMUM_RETRIES) {
            ++s_retry_count;
            ESP_LOGW(TAG, "Wi-Fi disconnected; retry %u/%u",
                     s_retry_count, WIFI_MAXIMUM_RETRIES);
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_wifi_events, WIFI_FAILED_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *got_ip = event_data;
        snprintf(
            s_ip_address,
            sizeof(s_ip_address),
            IPSTR,
            IP2STR(&got_ip->ip_info.ip)
        );
        s_retry_count = 0;
        s_connected = true;
        s_station_connecting = false;
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t load_credentials(
    char *ssid,
    size_t ssid_size,
    char *password,
    size_t password_size
)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(WIFI_NAMESPACE, NVS_READONLY, &handle);
    if (result != ESP_OK) {
        return result;
    }
    result = nvs_get_str(handle, WIFI_SSID_KEY, ssid, &ssid_size);
    if (result == ESP_OK) {
        result = nvs_get_str(
            handle,
            WIFI_PASSWORD_KEY,
            password,
            &password_size
        );
    }
    nvs_close(handle);
    return result;
}

static esp_err_t save_credentials(const char *ssid, const char *password)
{
    nvs_handle_t handle;
    ESP_RETURN_ON_ERROR(
        nvs_open(WIFI_NAMESPACE, NVS_READWRITE, &handle),
        TAG,
        "Could not open Wi-Fi settings"
    );
    esp_err_t result = nvs_set_str(handle, WIFI_SSID_KEY, ssid);
    if (result == ESP_OK) {
        result = nvs_set_str(handle, WIFI_PASSWORD_KEY, password);
    }
    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    return result;
}

static bool hex_value(char character, uint8_t *value)
{
    if (character >= '0' && character <= '9') {
        *value = (uint8_t)(character - '0');
        return true;
    }
    if (character >= 'a' && character <= 'f') {
        *value = (uint8_t)(character - 'a' + 10);
        return true;
    }
    if (character >= 'A' && character <= 'F') {
        *value = (uint8_t)(character - 'A' + 10);
        return true;
    }
    return false;
}

static bool decode_form_value(
    const char *encoded,
    size_t encoded_length,
    char *decoded,
    size_t decoded_size
)
{
    size_t output = 0;
    for (size_t input = 0; input < encoded_length; ++input) {
        if (output + 1 >= decoded_size) {
            return false;
        }
        if (encoded[input] == '+') {
            decoded[output++] = ' ';
        } else if (encoded[input] == '%' && input + 2 < encoded_length) {
            uint8_t high = 0;
            uint8_t low = 0;
            if (!hex_value(encoded[input + 1], &high) ||
                !hex_value(encoded[input + 2], &low)) {
                return false;
            }
            decoded[output++] = (char)((high << 4) | low);
            input += 2;
        } else {
            decoded[output++] = encoded[input];
        }
    }
    decoded[output] = '\0';
    return true;
}

static bool find_form_value(
    const char *body,
    const char *key,
    char *value,
    size_t value_size
)
{
    size_t key_length = strlen(key);
    const char *field = body;
    while (*field != '\0') {
        const char *end = strchr(field, '&');
        if (end == NULL) {
            end = field + strlen(field);
        }
        if ((size_t)(end - field) > key_length &&
            strncmp(field, key, key_length) == 0 &&
            field[key_length] == '=') {
            return decode_form_value(
                field + key_length + 1,
                (size_t)(end - field - key_length - 1),
                value,
                value_size
            );
        }
        field = *end == '&' ? end + 1 : end;
    }
    return false;
}

static esp_err_t root_handler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    if (s_mode == PICOPAL_WIFI_MODE_SETUP) {
        return httpd_resp_send(request, s_setup_page, HTTPD_RESP_USE_STRLEN);
    }

    return httpd_resp_send(request, s_control_page, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t timer_status_handler(httpd_req_t *request)
{
    uint64_t total_seconds = picopal_timer_elapsed_seconds();
    uint64_t minutes = total_seconds / 60U;
    uint64_t seconds = total_seconds % 60U;
    if (minutes > 999U) {
        minutes = 999U;
        seconds = 59U;
    }
    char response[112];
    snprintf(
        response,
        sizeof(response),
        "{\"running\":%s,\"elapsed_seconds\":%llu,\"display\":\"%03llu:%02llu\"}",
        picopal_timer_state() == PICOPAL_TIMER_RUNNING ? "true" : "false",
        (unsigned long long)total_seconds,
        (unsigned long long)minutes,
        (unsigned long long)seconds
    );
    httpd_resp_set_type(request, "application/json");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_sendstr(request, response);
}

static esp_err_t publish_timer_event(
    httpd_req_t *request,
    picopal_event_type_t type
)
{
    if (!picopal_events_publish((picopal_event_t){.type = type})) {
        httpd_resp_set_status(request, "503 Service Unavailable");
        return httpd_resp_sendstr(request, "Event queue full");
    }
    httpd_resp_set_status(request, "202 Accepted");
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_sendstr(request, "{\"accepted\":true}");
}

static esp_err_t timer_toggle_handler(httpd_req_t *request)
{
    return publish_timer_event(request, PICOPAL_EVENT_TIMER_REMOTE_TOGGLE);
}

static esp_err_t timer_reset_handler(httpd_req_t *request)
{
    return publish_timer_event(request, PICOPAL_EVENT_TIMER_REMOTE_RESET);
}

static esp_err_t forget_wifi_handler(httpd_req_t *request)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(WIFI_NAMESPACE, NVS_READWRITE, &handle);
    if (result != ESP_OK) {
        return httpd_resp_send_err(
            request,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Could not open Wi-Fi settings"
        );
    }
    result = nvs_erase_all(handle);
    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi settings cleared; restarting");
        httpd_resp_sendstr(request, "{\"restarting\":true}");
        nvs_close(handle);
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
        return ESP_OK;
    }
    nvs_close(handle);
    return httpd_resp_send_err(
        request,
        HTTPD_500_INTERNAL_SERVER_ERROR,
        "Could not clear Wi-Fi settings"
    );
}

static esp_err_t health_handler(httpd_req_t *request)
{
    char response[96];
    snprintf(
        response,
        sizeof(response),
        "{\"status\":\"ok\",\"mode\":\"%s\",\"ip\":\"%s\"}",
        s_mode == PICOPAL_WIFI_MODE_SETUP ? "setup" : "station",
        s_ip_address
    );
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_sendstr(request, response);
}

static esp_err_t configure_handler(httpd_req_t *request)
{
    if (s_mode != PICOPAL_WIFI_MODE_SETUP || request->content_len <= 0 ||
        request->content_len >= CONFIGURE_BODY_SIZE) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid request");
    }

    char body[CONFIGURE_BODY_SIZE];
    size_t received = 0;
    while (received < (size_t)request->content_len) {
        int result = httpd_req_recv(
            request,
            body + received,
            (size_t)request->content_len - received
        );
        if (result <= 0) {
            return ESP_FAIL;
        }
        received += (size_t)result;
    }
    body[received] = '\0';

    char ssid[33];
    char password[64];
    if (!find_form_value(body, "ssid", ssid, sizeof(ssid)) || ssid[0] == '\0' ||
        !find_form_value(body, "password", password, sizeof(password))) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid Wi-Fi values");
    }
    ESP_RETURN_ON_ERROR(
        save_credentials(ssid, password),
        TAG,
        "Could not save Wi-Fi settings"
    );

    ESP_LOGI(TAG, "Wi-Fi settings saved; restarting");
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    httpd_resp_sendstr(
        request,
        "<!doctype html><meta name='viewport' content='width=device-width'>"
        "<h1>Saved</h1><p>PicoPal is restarting and connecting.</p>"
    );
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

static esp_err_t start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_RETURN_ON_ERROR(
        httpd_start(&s_http_server, &config),
        TAG,
        "HTTP server start failed"
    );

    const httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
    };
    const httpd_uri_t health = {
        .uri = "/health",
        .method = HTTP_GET,
        .handler = health_handler,
    };
    const httpd_uri_t configure = {
        .uri = "/configure",
        .method = HTTP_POST,
        .handler = configure_handler,
    };
    const httpd_uri_t timer_status = {
        .uri = "/api/timer",
        .method = HTTP_GET,
        .handler = timer_status_handler,
    };
    const httpd_uri_t timer_toggle = {
        .uri = "/api/timer/toggle",
        .method = HTTP_POST,
        .handler = timer_toggle_handler,
    };
    const httpd_uri_t timer_reset = {
        .uri = "/api/timer/reset",
        .method = HTTP_POST,
        .handler = timer_reset_handler,
    };
    const httpd_uri_t forget_wifi = {
        .uri = "/api/wifi/forget",
        .method = HTTP_POST,
        .handler = forget_wifi_handler,
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_http_server, &root), TAG, "Root route failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_http_server, &health), TAG, "Health route failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_http_server, &configure), TAG, "Configure route failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_http_server, &timer_status), TAG, "Timer status route failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_http_server, &timer_toggle), TAG, "Timer toggle route failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_http_server, &timer_reset), TAG, "Timer reset route failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_http_server, &forget_wifi), TAG, "Wi-Fi forget route failed");
    return ESP_OK;
}

static esp_err_t start_setup_access_point(void)
{
    s_station_connecting = false;
    s_connected = false;
    s_mode = PICOPAL_WIFI_MODE_SETUP;
    strcpy(s_ip_address, "192.168.4.1");

    wifi_config_t config = {0};
    strcpy((char *)config.ap.ssid, WIFI_SETUP_SSID);
    config.ap.ssid_len = strlen(WIFI_SETUP_SSID);
    config.ap.channel = 1;
    config.ap.authmode = WIFI_AUTH_OPEN;
    config.ap.max_connection = 4;

    esp_err_t stop_result = esp_wifi_stop();
    if (stop_result != ESP_OK && stop_result != ESP_ERR_WIFI_NOT_STARTED) {
        ESP_RETURN_ON_ERROR(stop_result, TAG, "Wi-Fi stop failed");
    }
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "AP mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &config), TAG, "AP config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "AP start failed");
    ESP_LOGI(TAG, "Setup network ready: %s", WIFI_SETUP_SSID);
    ESP_LOGI(TAG, "Open http://192.168.4.1");
    return ESP_OK;
}

static bool connect_station(const char *ssid, const char *password)
{
    wifi_config_t config = {0};
    strlcpy((char *)config.sta.ssid, ssid, sizeof(config.sta.ssid));
    strlcpy((char *)config.sta.password, password, sizeof(config.sta.password));
    config.sta.threshold.authmode = WIFI_AUTH_OPEN;

    s_mode = PICOPAL_WIFI_MODE_STATION;
    s_station_connecting = true;
    s_retry_count = 0;
    xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT | WIFI_FAILED_BIT);

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
        esp_wifi_set_config(WIFI_IF_STA, &config) != ESP_OK ||
        esp_wifi_start() != ESP_OK || esp_wifi_connect() != ESP_OK) {
        s_station_connecting = false;
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_events,
        WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS)
    );
    if ((bits & WIFI_CONNECTED_BIT) != 0) {
        ESP_LOGI(TAG, "Connected to Wi-Fi; open http://%s", s_ip_address);
        return true;
    }
    ESP_LOGW(TAG, "Could not connect to saved Wi-Fi; entering setup mode");
    return false;
}

static void wifi_start_task(void *context)
{
    (void)context;
    char ssid[33] = {0};
    char password[64] = {0};
    esp_err_t result = ESP_OK;

    if (load_credentials(ssid, sizeof(ssid), password, sizeof(password)) != ESP_OK ||
        !connect_station(ssid, password)) {
        result = start_setup_access_point();
    }
    if (result == ESP_OK) {
        result = start_http_server();
    }
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi startup failed: %s", esp_err_to_name(result));
    }
    vTaskDelete(NULL);
}

esp_err_t picopal_wifi_init(void)
{
    s_wifi_events = xEventGroupCreate();
    if (s_wifi_events == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Network interface init failed");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "Event loop init failed");
    if (esp_netif_create_default_wifi_sta() == NULL ||
        esp_netif_create_default_wifi_ap() == NULL) {
        return ESP_ERR_NO_MEM;
    }

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_config), TAG, "Wi-Fi init failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL),
        TAG,
        "Wi-Fi event handler failed"
    );
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL),
        TAG,
        "IP event handler failed"
    );

    BaseType_t created = xTaskCreate(
        wifi_start_task,
        "picopal_wifi",
        4096,
        NULL,
        3,
        NULL
    );
    return created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

picopal_wifi_mode_t picopal_wifi_mode(void)
{
    return s_mode;
}

bool picopal_wifi_connected(void)
{
    return s_connected;
}
