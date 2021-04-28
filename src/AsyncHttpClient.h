#ifndef HttpClient_h
#define HttpClient_h

#include <Arduino.h>
#include <IPAddress.h>

#ifdef ESP32
    #include <WiFi.h>
    #include <AsyncTCP.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESPAsyncTCP.h>
#else
    #error Platform not supported
#endif

static const int HTTP_ERROR_CONNECTION_FAILED = -1;
static const int HTTP_ERROR_TIMED_OUT = -2;
static const int HTTP_ERROR_INVALID_RESPONSE = -3;

enum HttpMethod {
    GET,
    POST
};

enum HttpState {
    IDLE,
    START,
    READING_STATUS_LINE,
    READING_HEADERS,
    READING_BODY
};

struct Request {
    IPAddress 	Address;
    String 		Hostname;
    uint16_t 	Port;
    String 		Path;
    HttpMethod 	Method;
    String 		ContentType;
    char*		Data;
    size_t 		DataSize;
};

struct Response {
    uint8_t StatusCode 		= 0;
    String 	StatusText 		= "";
    String 	ContentType 	= "";
    size_t 	ContentSize 	= 0;
    String*	Headers; 			/**< Not Support yet! */
};

typedef std::function<void(Response response)> ResponseHandler;
typedef std::function<void(char *data, size_t len)> DataHandler;
typedef std::function<void(int error)> ErrorHandler;

class AsyncHttpClient {
    public:
        AsyncHttpClient(String hostname, int port);
        AsyncHttpClient(IPAddress address, int port);
        void onResponse(ResponseHandler handler);
        void onData(DataHandler handler);
        void onError(ErrorHandler handler);
        void get(String path);
        void post(String path);
        void post(String path, String contentType, char *data, size_t len);

        AsyncClient* client(void);
    private:
        AsyncClient *_client;
        HttpState _state;
        Request _request;
        Response _response;
        ResponseHandler _responseHandler = NULL;
        DataHandler _dataHandler = NULL;
        ErrorHandler _errorHandler = NULL;
        size_t _bodyDownloadedSize;
        void init();
        void startRequest();
        void sendRequest();
        void processStatusLine(String line);
        void processHeaderLine(String line);
        void handleConnect();
        void handleDisconnect();
        void handleTimeout(int timeout);
        void handleError(int error);
        void handleData(char *data, size_t length);
};

#endif
