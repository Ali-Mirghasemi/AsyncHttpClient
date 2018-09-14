#include <Arduino.h>
#include <AsyncTCP.h>
#include "AsyncHttpClient.h"

AsyncHttpClient::AsyncHttpClient(IPAddress address, int port) {
	request.address = address;
	request.port = port;
	init();
}

AsyncHttpClient::AsyncHttpClient(String hostname, int port) {
	request.hostname = hostname;
	request.port = port;
	init();
}

void AsyncHttpClient::init() {
	responseHandler = NULL;
	dataHandler = NULL;
	errorHandler = NULL;
	client = new AsyncClient();
	client->onConnect([this](void *arg, AsyncClient *client) { this->handleConnect(); });
	client->onDisconnect([this](void *arg, AsyncClient *client) { this->handleDisconnect(); });
	client->onTimeout([this](void *arg, AsyncClient *client, int timeout) { this->handleTimeout(timeout); });
	client->onError([this](void *arg, AsyncClient *client, int error) { this->handleError(error); });
	client->onData([this](void *arg, AsyncClient *client, void *data, size_t length) { this->handleData((char *)data, length); });
}

void AsyncHttpClient::onData(DataHandler handler) {
	dataHandler = handler;
}

void AsyncHttpClient::onError(ErrorHandler handler) {
	errorHandler = handler;
}

void AsyncHttpClient::onResponse(ResponseHandler handler) {
	responseHandler = handler;
}

void AsyncHttpClient::startRequest() {
	if (!client->connected()) {
		if (request.hostname != "")
			client->connect(request.hostname.c_str(), request.port);	
		else
			client->connect(request.address, request.port);	
		return;
	}
	state = READING_STATUS_LINE;
	sendRequest();
}

void AsyncHttpClient::handleConnect() {
	state = READING_STATUS_LINE;
	sendRequest();
}

void AsyncHttpClient::handleDisconnect() {
	state = IDLE;
}

void AsyncHttpClient::handleTimeout(int timeout) {
	state = IDLE;
	if (errorHandler != NULL)
		errorHandler(HTTP_ERROR_TIMED_OUT);
}

void AsyncHttpClient::handleError(int error) {
	state = IDLE;
	if (errorHandler != NULL)
		errorHandler(HTTP_ERROR_CONNECTION_FAILED);
}

void AsyncHttpClient::handleData(char *data, size_t length) {
	int index = 0;
	while (state == READING_STATUS_LINE || state == READING_HEADERS) {
		String line = "";
		while (index < length) {
			char c = data[index];
			index++;
			if (c == '\n')
				break;
			else if (c != '\r')
				line += c;
		}
		if (state == READING_STATUS_LINE) {
			processStatusLine(line);
			state = READING_HEADERS;
		} else if (state == READING_HEADERS) {
			if (line.length() != 0)
				processHeaderLine(line);
			else {
				state = READING_BODY;
				if (responseHandler != NULL)
					responseHandler(response);
			}
		}
		if (index == length)
			break;
	}
	if (state == READING_BODY) {
		size_t size = length - index;
		bodyDownloadedSize += size;
		if (dataHandler != NULL)
			dataHandler(data + index, size);
	}
}

void AsyncHttpClient::processStatusLine(String line) {
	const char* statusPrefix = "HTTP/*.* ";
	int index = 0;
	// prefix
	while (index < line.length()) {
		char c = line[index];
		if (statusPrefix[index] == '*' || statusPrefix[index] == c) {
			index++;
			if (index == sizeof(statusPrefix))
				break;
		} else {
		}
	}
	// status code
	int s = 0;
	while (index < line.length()) {
		char c = line[index];
		if (isdigit(c)) {
			s = s * 10 + (c - '0');
			index++;
		} else {
			break;
		}
	}
	response.statusCode = 200;
	response.statusText = "";
}

void AsyncHttpClient::processHeaderLine(String line) {
	unsigned long split = line.indexOf(": ");
	String headerName = line.substring(0, split);
	String headerValue = line.substring(split + 2);
	if (!strcasecmp(headerName.c_str(), "content-length"))
		response.contentSize = headerValue.toInt();
	if (!strcasecmp(headerName.c_str(), "content-type"))
		response.contentType = headerValue;
//	int headersSize = sizeof(response.headers) / sizeof(String);
//	String *h = new String[headersSize + 1];
//	std::copy(response.headers, response.headers + headersSize, h);
//	delete[] response.headers;
//	response.headers = h;
//	response.headers[headersSize + 1] = line;
}

void AsyncHttpClient::sendRequest() {
	Serial.println("sendRequest");
	String s;
	if (request.method == GET)
		s = "GET";
	else if (request.method == POST)
		s = "POST";
	s += " " + String(request.path) + " HTTP/1.1\n";
	s += "Host: " + request.hostname + "\n";
	if (request.method == POST)
		s += "Content-Type: " + request.contentType + "\n";
		s += "Content-Size: " + String(request.dataSize) + "\n";
	s += "\n";
	client->add(s.c_str(), s.length());
	if (request.method == POST)
		client->add(request.data, request.dataSize);
	client->send();
}

void AsyncHttpClient::get(String path) {
	request.path = path;
	request.method = GET;
	request.contentType = "";
	request.data = NULL;
	request.dataSize = 0;
	startRequest();
}

void AsyncHttpClient::post(String path, String contentType, char *data, size_t len) {
	request.path = path;
	request.method = POST;
	request.contentType = contentType;
	request.data = data;
	request.dataSize = len;
	startRequest();
}
