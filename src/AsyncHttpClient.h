#ifndef HttpClient_h
#define HttpClient_h

#include <Arduino.h>
#include <IPAddress.h>
#include "AsyncTCP.h"

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
	IPAddress address;
	String hostname;
	uint16_t port;
	String path;
	HttpMethod method;
	String contentType;
	char *data;
	size_t dataSize;
};

struct Response {
	uint8_t statusCode = 0;
	String statusText = "";
	String contentType = "";
	size_t contentSize = 0;
	String *headers;
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
	private:
		AsyncClient *client;
		HttpState state;
		Request request;
		Response response;
		ResponseHandler responseHandler = NULL;
		DataHandler dataHandler = NULL;
		ErrorHandler errorHandler = NULL;
		size_t bodyDownloadedSize;
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
