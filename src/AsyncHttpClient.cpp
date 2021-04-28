#include "AsyncHttpClient.h"

AsyncHttpClient::AsyncHttpClient(IPAddress address, int port) {
	_request.Address = address;
	_request.Port = port;
	init();
}

AsyncHttpClient::AsyncHttpClient(String hostname, int port) {
	_request.Hostname = hostname;
	_request.Port = port;
	init();
}

void AsyncHttpClient::init() {
	_responseHandler = NULL;
	_dataHandler = NULL;
	_errorHandler = NULL;
	_client = new AsyncClient();
	_client->onConnect([this](void *arg, AsyncClient *client) { this->handleConnect(); });
	_client->onDisconnect([this](void *arg, AsyncClient *client) { this->handleDisconnect(); });
	_client->onTimeout([this](void *arg, AsyncClient *client, int timeout) { this->handleTimeout(timeout); });
	_client->onError([this](void *arg, AsyncClient *client, int error) { this->handleError(error); });
	_client->onData([this](void *arg, AsyncClient *client, void *data, size_t length) { this->handleData((char *)data, length); });
}

void AsyncHttpClient::onData(DataHandler handler) {
	_dataHandler = handler;
}

void AsyncHttpClient::onError(ErrorHandler handler) {
	_errorHandler = handler;
}

void AsyncHttpClient::onResponse(ResponseHandler handler) {
	_responseHandler = handler;
}

void AsyncHttpClient::startRequest() {
	if (!_client->connected()) {
		if (_request.Hostname != "")
			_client->connect(_request.Hostname.c_str(), _request.Port);	
		else
			_client->connect(_request.Address, _request.Port);	
		return;
	}
	_state = READING_STATUS_LINE;
	sendRequest();
}

void AsyncHttpClient::handleConnect() {
	_state = READING_STATUS_LINE;
	sendRequest();
}

void AsyncHttpClient::handleDisconnect() {
	_state = IDLE;
}

void AsyncHttpClient::handleTimeout(int timeout) {
	_state = IDLE;
	if (_errorHandler != NULL)
		_errorHandler(HTTP_ERROR_TIMED_OUT);
}

void AsyncHttpClient::handleError(int error) {
	_state = IDLE;
	if (_errorHandler != NULL)
		_errorHandler(HTTP_ERROR_CONNECTION_FAILED);
}

void AsyncHttpClient::handleData(char *data, size_t length) {
	int index = 0;
	while (_state == READING_STATUS_LINE || _state == READING_HEADERS) {
		String line = "";
		while (index < length) {
			char c = data[index];
			index++;
			if (c == '\n')
				break;
			else if (c != '\r')
				line += c;
		}
		if (_state == READING_STATUS_LINE) {
			processStatusLine(line);
			_state = READING_HEADERS;
		} else if (_state == READING_HEADERS) {
			if (line.length() != 0)
				processHeaderLine(line);
			else {
				_state = READING_BODY;
				if (_responseHandler != NULL)
					_responseHandler(_response);
			}
		}
		if (index == length)
			break;
	}
	if (_state == READING_BODY) {
		size_t size = length - index;
		_bodyDownloadedSize += size;
		if (_dataHandler != NULL)
			_dataHandler(data + index, size);
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
	_response.StatusCode = 200;
	_response.StatusText = "";
}

void AsyncHttpClient::processHeaderLine(String line) {
	unsigned long split = line.indexOf(": ");
	String headerName = line.substring(0, split);
	String headerValue = line.substring(split + 2);
	if (!strcasecmp(headerName.c_str(), "content-length"))
		_response.ContentSize = headerValue.toInt();
	if (!strcasecmp(headerName.c_str(), "content-type"))
		_response.ContentType = headerValue;
//	int headersSize = sizeof(response.headers) / sizeof(String);
//	String *h = new String[headersSize + 1];
//	std::copy(response.headers, response.headers + headersSize, h);
//	delete[] response.headers;
//	response.headers = h;
//	response.headers[headersSize + 1] = line;
}

void AsyncHttpClient::sendRequest() {
	String s;
	if (_request.Method == GET)
		s = "GET";
	else if (_request.Method == POST)
		s = "POST";
	s += " " + String(_request.Path) + " HTTP/1.1\n";
	s += "Host: " + _request.Hostname + "\n";
	if (_request.Method == POST)
		s += "Content-Type: " + _request.ContentType + "\n";
		s += "Content-Size: " + String(_request.DataSize) + "\n";
	s += "\n";
	_client->add(s.c_str(), s.length());
	if (_request.Method == POST)
		_client->add(_request.Data, _request.DataSize);
	_client->send();
}

void AsyncHttpClient::get(String path) {
	_request.Path = path;
	_request.Method = GET;
	_request.ContentType = "";
	_request.Data = NULL;
	_request.DataSize = 0;
	startRequest();
}

void AsyncHttpClient::post(String path, String contentType, char *data, size_t len) {
	_request.Path = path;
	_request.Method = POST;
	_request.ContentType = contentType;
	_request.Data = data;
	_request.DataSize = len;
	startRequest();
}

AsyncClient* AsyncHttpClient::client(void) {
	return _client;
}
