#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "drivers/rtc/rtc.h"

static const char *TAG = "http";

// =========================
// PRODUCTION (YA TENÍAS)
// =========================
static esp_err_t production_get_handler(httpd_req_t *req)
{
    const char *resp = "{ \"metros\": 123.4, \"velocidad\": 3.2 }";

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

// =========================
// LISTAR PERFILES
// =========================
static esp_err_t profiles_list_handler(httpd_req_t *req)
{
    DIR *dir = opendir("/sdcard/profiles");
    if (!dir)
    {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No se pudo abrir carpeta");
    }

    // 🔥 usar HEAP en vez de stack
    char *json = malloc(4096);
    if (!json)
    {
        closedir(dir);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No mem");
    }

    strcpy(json, "[");

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            strcat(json, "\"");
            strcat(json, entry->d_name);
            strcat(json, "\",");
        }
    }

    closedir(dir);

    int len = strlen(json);
    if (len > 1 && json[len - 1] == ',')
    {
        json[len - 1] = '\0';
    }

    strcat(json, "]");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);

    free(json);

    return ESP_OK;
}

// =========================
// LEER PERFIL
// =========================
static esp_err_t profile_get_handler(httpd_req_t *req)
{
    char filename[128];
    char filepath[160];

    if (httpd_req_get_url_query_str(req, filename, sizeof(filename)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta query");

    char param[64];
    if (httpd_query_key_value(filename, "name", param, sizeof(param)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta name");

    sprintf(filepath, "/sdcard/profiles/%s", param);

    FILE *f = fopen(filepath, "r");
    if (!f)
        return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Archivo no encontrado");

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer)
    {
        fclose(f);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No mem");
    }

    fread(buffer, 1, size, f);
    buffer[size] = 0;

    fclose(f);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buffer, HTTPD_RESP_USE_STRLEN);
    free(buffer);
    return ESP_OK;
}

// =========================
// GUARDAR PERFIL (POST)
// =========================
static esp_err_t profile_save_handler(httpd_req_t *req)
{
    char filepath[128];
    char query[128];
    char name[64];

    // =========================
    // Obtener nombre
    // =========================
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta query");

    if (httpd_query_key_value(query, "name", name, sizeof(name)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta name");

    sprintf(filepath, "/sdcard/profiles/%s", name);

    // =========================
    // Leer body COMPLETO
    // =========================
    int total_len = req->content_len;
    int received = 0;

    char *content = malloc(total_len + 1);
    if (!content)
        return ESP_FAIL;

    while (received < total_len)
    {
        int r = httpd_req_recv(req, content + received, total_len - received);
        if (r <= 0)
        {
            free(content);
            return ESP_FAIL;
        }
        received += r;
    }

    content[total_len] = 0;

    // =========================
    // Guardar archivo
    // =========================
    FILE *f = fopen(filepath, "w");
    if (!f)
    {
        free(content);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No se pudo abrir archivo");
    }

    fwrite(content, 1, strlen(content), f);
    fclose(f);

    free(content);

    ESP_LOGI(TAG, "Perfil guardado: %s", filepath);

    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

// =========================
// BORRAR PERFIL
// =========================
static esp_err_t profile_delete_handler(httpd_req_t *req)
{
    char query[128];
    char name[64];
    char filepath[160];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta query");

    if (httpd_query_key_value(query, "name", name, sizeof(name)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta name");

    sprintf(filepath, "/sdcard/profiles/%s", name);

    if (remove(filepath) != 0)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error al borrar");

    httpd_resp_sendstr(req, "Deleted");

    return ESP_OK;
}

// =========================
// VER LOGS
// =========================
static esp_err_t logs_handler(httpd_req_t *req)
{
    char filepath[128];
    char date[32];

    // 🔥 obtener fecha actual
    rtc_get_date_filename_string(date);

    sprintf(filepath,
            "/sdcard/logs/production_%s.csv",
            date);

    FILE *f = fopen(filepath, "r");
    if (!f)
    {
        return httpd_resp_send_err(req,
                                   HTTPD_404_NOT_FOUND,
                                   "CSV no encontrado");
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer)
    {
        fclose(f);
        return httpd_resp_send_err(req,
                                   HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "No mem");
    }

    fread(buffer, 1, size, f);
    buffer[size] = 0;

    fclose(f);

    // 🔥 mejor como CSV
    httpd_resp_set_type(req, "text/csv");

    httpd_resp_send(req, buffer, HTTPD_RESP_USE_STRLEN);

    free(buffer);
    return ESP_OK;
}

// =========================
// HTML PRINCIPAL (MEJORADO)
// =========================
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char *html =
        "<!DOCTYPE html>"
        "<html><head>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>Control de extrusión</title>"

        "<style>"

        "body { font-family: Arial; background:#0f1115; color:#eee; margin:0; }"

        "header { padding:15px; background:#0a0c10; font-size:20px; font-weight:bold; }"

        ".container { display:flex; height:calc(100vh - 60px); }"

        ".left { width:30%; background:#151821; padding:15px; overflow:auto; }"
        ".right { width:70%; padding:15px; }"

        ".card { background:#1c1f26; padding:15px; margin-bottom:10px; border-radius:8px; }"

        "button { padding:8px 12px; border:none; border-radius:6px; cursor:pointer; }"

        ".btn { background:#2ecc71; color:#000; }"
        ".btn-red { background:#e74c3c; color:#fff; }"
        ".btn-gray { background:#555; color:#fff; }"

        ".file { padding:8px; margin:5px 0; background:#222; border-radius:6px; cursor:pointer; }"
        ".file:hover { background:#2ecc71; color:#000; }"

        "textarea { width:100%; height:300px; background:#000; color:#0f0; padding:10px; border-radius:6px; }"

        "</style>"

        "</head><body>"

        "<header>DeepMove Controller</header>"

        "<div class='container'>"

        "<div class='left'>"
        "<div class='card'>"
        "<h3>Perfiles</h3>"
        "<button class='btn' onclick='loadProfiles()'>Actualizar</button>"
        "<div id='list'></div>"
        "</div>"

        "<div class='card'>"
        "<h3>Logs</h3>"
        "<button class='btn-gray' onclick='loadLogs()'>Ver logs</button>"
        "<pre id='logs' style='max-height:200px; overflow:auto;'></pre>"
        "</div>"
        "</div>"

        "<div class='right'>"
        "<div class='card'>"
        "<h3 id='selected'>Editor</h3>"
        "<textarea id='editor'></textarea><br><br>"

        "<button class='btn' onclick='saveProfile()'>Guardar</button>"
        "<button class='btn-red' onclick='deleteProfile()'>Eliminar</button>"
        "</div>"
        "</div>"

        "</div>"

        "<script>"

        "let currentFile = '';"

        // =========================
        // LISTA
        // =========================
        "async function loadProfiles(){"
        " let res = await fetch('/api/profiles');"
        " let data = await res.json();"

        " let list = document.getElementById('list');"
        " list.innerHTML='';"

        " data.forEach(p => {"
        "   let div = document.createElement('div');"
        "   div.className='file';"
        "   div.innerText = p;"
        "   div.onclick = () => openProfile(p);"
        "   list.appendChild(div);"
        " });"
        "}"

        // =========================
        // ABRIR
        // =========================
        "async function openProfile(name){"
        " currentFile = name;"

        " let res = await fetch('/api/profile?name=' + name);"
        " let text = await res.text();"

        " document.getElementById('editor').value = text;"
        " document.getElementById('selected').innerText = 'Editando: ' + name;"
        "}"

        // =========================
        // GUARDAR
        // =========================
        "async function saveProfile(){"
        " if(!currentFile){ alert('Seleccioná un perfil'); return; }"

        " let content = document.getElementById('editor').value;"

        " let res = await fetch('/api/profile?name=' + currentFile, {"
        "   method:'POST',"
        "   body: content"
        " });"

        " alert(res.ok ? 'Guardado OK' : 'Error');"
        "}"

        // =========================
        // BORRAR
        // =========================
        "async function deleteProfile(){"
        " if(!currentFile) return;"

        " if(!confirm('¿Eliminar perfil?')) return;"

        " let res = await fetch('/api/profile?name=' + currentFile, {"
        "   method:'DELETE'"
        " });"

        " if(res.ok){"
        "   alert('Eliminado');"
        "   loadProfiles();"
        "   document.getElementById('editor').value='';"
        " }"
        "}"

        // =========================
        // LOGS
        // =========================
        "async function loadLogs(){"
        " let res = await fetch('/api/logs');"
        " let txt = await res.text();"
        " document.getElementById('logs').innerText = txt;"
        "}"

        "</script>"

        "</body></html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}
// =========================
// START SERVER
// =========================
void start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    httpd_start(&server, &config);

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler};

    httpd_uri_t production = {
        .uri = "/api/production",
        .method = HTTP_GET,
        .handler = production_get_handler};

    httpd_uri_t profiles = {
        .uri = "/api/profiles",
        .method = HTTP_GET,
        .handler = profiles_list_handler};

    httpd_uri_t profile_get = {
        .uri = "/api/profile",
        .method = HTTP_GET,
        .handler = profile_get_handler};

    httpd_uri_t profile_save = {
        .uri = "/api/profile",
        .method = HTTP_POST,
        .handler = profile_save_handler};

    httpd_uri_t profile_delete = {
        .uri = "/api/profile",
        .method = HTTP_DELETE,
        .handler = profile_delete_handler};

    httpd_uri_t logs = {
        .uri = "/api/logs",
        .method = HTTP_GET,
        .handler = logs_handler};

    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &production);
    httpd_register_uri_handler(server, &profiles);
    httpd_register_uri_handler(server, &profile_get);
    httpd_register_uri_handler(server, &profile_save);
    httpd_register_uri_handler(server, &profile_delete);
    httpd_register_uri_handler(server, &logs);

    ESP_LOGI(TAG, "HTTP server started");
}