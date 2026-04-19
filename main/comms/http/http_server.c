#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "http";

// 🔥 Endpoint /api/production
static esp_err_t production_get_handler(httpd_req_t *req)
{
    const char *resp = "{ \"metros\": 123.4, \"velocidad\": 3.2 }";

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

// 🔥 Página principal simple
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char *html =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>DeepMove</title>"

    // 🔥 Chart.js desde CDN
    "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"

    "<style>"
    "body { font-family: Arial; text-align: center; background:#111; color:#fff; }"
    ".card { background:#222; padding:20px; margin:10px; border-radius:10px; }"
    ".value { font-size:40px; color:#00ff88; }"
    "canvas { max-width: 100%; }"
    "</style>"

    "</head>"
    "<body>"

    "<h1>DeepMove Controller</h1>"

    "<div class='card'>"
    "<h2>Velocidad</h2>"
    "<div class='value' id='speed'>0</div>"
    "</div>"

    "<div class='card'>"
    "<h2>Metros</h2>"
    "<div class='value' id='meters'>0</div>"
    "</div>"

    "<div class='card'>"
    "<h2>Gráfico Velocidad</h2>"
    "<canvas id='chart'></canvas>"
    "</div>"

    "<script>"

    "let chart = new Chart(document.getElementById('chart'), {"
    " type: 'line',"
    " data: {"
    "  labels: [],"
    "  datasets: [{"
    "   label: 'Velocidad',"
    "   data: [],"
    "   borderColor: '#00ff88'"
    "  }]"
    " }"
    "});"

    "setInterval(async () => {"
    " const res = await fetch('/api/production');"
    " const data = await res.json();"

    " document.getElementById('speed').innerText = data.velocidad;"
    " document.getElementById('meters').innerText = data.metros;"

    " chart.data.labels.push('');"
    " chart.data.datasets[0].data.push(data.velocidad);"

    " if (chart.data.labels.length > 20) {"
    "  chart.data.labels.shift();"
    "  chart.data.datasets[0].data.shift();"
    " }"

    " chart.update();"

    "}, 1000);"

    "</script>"

    "</body></html>";

    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

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

    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &production);

    ESP_LOGI(TAG, "HTTP server started");
}