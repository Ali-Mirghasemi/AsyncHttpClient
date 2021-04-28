#include <AsyncHttpClient.h>

/**
 * Configuration
 */
static const char WIFI_SSID[]         = "yourSSID";
static const char WIFI_PASS[]         = "yourPass";

static const String WEB_SERVER_URL    = "www.google.com";
static const uint16_t WEB_SERVER_PORT = 80;

static const String URL_GET           = "/";

/**
 * Callbacks
 */
static void HttpClient_onResponse(Response response);
static void HttpClient_onData(char *data, size_t len);
static void HttpClient_onError(int error);

static AsyncHttpClient httpClient(WEB_SERVER_URL, WEB_SERVER_PORT);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(50);
  }
  Serial.println();

  // connect to wifi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print('.');
  }
  Serial.println("Connected");

  // initialize http client
  httpClient.onResponse(HttpClient_onResponse);
  httpClient.onData(HttpClient_onData);
  httpClient.onError(HttpClient_onError);

}

void loop() {
  // request every 5 sec, if not in receiveing data
  if (!httpClient.client()->connected()) {
    httpClient.get(URL_GET);
  }
  delay(10000);
}

static void HttpClient_onResponse(Response response) {
  Serial.println("\r\n------------- Response ---------------");
  Serial.print("Status Code: ");
  Serial.println(response.StatusCode);
  Serial.print("Status Text: ");
  Serial.println(response.StatusText);
  Serial.print("Content-Type: ");
  Serial.println(response.ContentType);
  Serial.print("Content-Size: ");
  Serial.println(response.ContentSize);
  Serial.println("--------------------------------------");
}
static void HttpClient_onData(char *data, size_t len) {
  Serial.println("\r\n------------- Data ---------------");
  while (len-- > 0) {
    Serial.print(*data++);
  }
  Serial.println("\r\n--------------------------------------");
}
static void HttpClient_onError(int error) {
  Serial.print("Error: ");
  Serial.println(error);
}
