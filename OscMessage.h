#pragma once

#include "tinyosc.h"
#include <vector>
#include <memory>

struct OscArgument {
	enum class Type {
		BLOB,
		FLOAT,
		DOUBLE,
		INT32,
		INT64,
		STRING,
		MIDI,
		TIMETAG,
		BOOL,
		UNKNOWN
	};
	Type type = Type::UNKNOWN;
	virtual char getChar() { return 'u'; }
};

struct OscBlob : public OscArgument {
	OscBlob(const char* d, int s) {
		data = new char[s];
		size = s;
		memcpy(data, d, s);
		type = Type::BLOB;
	}
	virtual char getChar() { return 'b'; }
	char* data;
	int size;
};

struct OscFloat : public OscArgument {
	OscFloat(float d) : data(d) { type = Type::FLOAT; }
	virtual char getChar() { return 'f'; }
	float data;
};

struct OscDouble : public OscArgument {
	OscDouble(double d) : data(d) { type = Type::DOUBLE; }
	virtual char getChar() { return 'd'; }
	double data;
};

struct OscInt32 : public OscArgument {
	OscInt32(int32_t d) : data(d) { type = Type::INT32; }
	virtual char getChar() { return 'i'; }
	int32_t data;
};

struct OscInt64 : public OscArgument {
	OscInt64(int64_t d) : data(d) { type = Type::INT64; }
	virtual char getChar() { return 'h'; }
	int64_t data;
};

struct OscString : public OscArgument {
	OscString(const char* d) {
		strcpy(data, d);
		type = Type::STRING;
	}
	virtual char getChar() { return 's'; }
	char* data;
};

struct OscMidi : public OscArgument {
	OscMidi(unsigned char* d) {
		portId = d[0];
		statusByte = d[1];
		data1 = d[2];
		data2 = d[3];
		type = Type::MIDI;
	}
	OscMidi(char port, char status, char d1, char d2) : portId(port), statusByte(status), data1(d1), data2(d2) { type = Type::MIDI; }
	virtual char getChar() { return 'm'; }
	char portId;
	char statusByte;
	char data1;
	char data2;
};

struct OscTimetag : public OscArgument {
	OscTimetag(uint64_t d) : data(d) { type = Type::TIMETAG; }
	virtual char getChar() { return 't'; }
	uint64_t data;
};

struct OscBool : public OscArgument {
	OscBool(bool d) : data(d) { type = Type::BOOL; }
	virtual char getChar() { return data ? 'T' : 'F'; }
	bool data;
};



class OscMessage {
public:

	OscMessage(char* inputBuffer, size_t size) {
		tosc_message message;
		if (0 == tosc_parseMessage(&message, inputBuffer, size)) {
			strcpy(address_string, tosc_getAddress(&message));
			char* format = tosc_getFormat(&message);
			int argumentCount = strlen(format);
			for (int i = 0; i < argumentCount; i++) {
				char argument = format[i];
				switch (argument) {
				case 'b':
					const char* buffer;
					int size;
					tosc_getNextBlob(&message, &buffer, &size);
					arguments.push_back(OscBlob(buffer, size));
					break;
				case 'f': arguments.push_back(OscFloat(tosc_getNextFloat(&message))); break;
				case 'd': arguments.push_back(OscDouble(tosc_getNextDouble(&message))); break;
				case 'i': arguments.push_back(OscInt32(tosc_getNextInt32(&message))); break;
				case 'h': arguments.push_back(OscInt64(tosc_getNextInt64(&message))); break;
				case 's': arguments.push_back(OscString(tosc_getNextString(&message))); break;
				case 'm': arguments.push_back(OscMidi(tosc_getNextMidi(&message))); break;
				case 't': arguments.push_back(OscTimetag(tosc_getNextTimetag(&message))); break;
				case 'T': arguments.push_back(OscBool(true)); break;
				case 'F': arguments.push_back(OscBool(false)); break;
				case 'I':
				case 'N':
				default: arguments.push_back(OscArgument()); break;
				}
			}
		}
	}

	OscMessage(const char* address);

	void addBlob(char* buffer, size_t size) { arguments.push_back(OscBlob(buffer, size)); }
	void addFloat(float data) { arguments.push_back(OscFloat(data)); }
	void addDouble(double data) { arguments.push_back(OscDouble(data)); }
	void addInt32(int32_t data) { arguments.push_back(OscInt32(data)); }
	void addInt64(int64_t data) { arguments.push_back(OscInt64(data)); }
	void addString(const char* data) { arguments.push_back(OscString(data)); }
	void addMidi(char port, char statusByte, char data1, char data2) { arguments.push_back(OscMidi(port, statusByte, data1, data2)); }
	void addTimetag(uint64_t data) { arguments.push_back(OscTimetag(data)); }

	bool isBlob(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex].type == OscArgument::Type::BLOB)		return true; else return false; }
	bool isFloat(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex].type == OscArgument::Type::FLOAT)	return true; else return false; }
	bool isDouble(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex].type == OscArgument::Type::DOUBLE)	return true; else return false; }
	bool isInt32(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex].type == OscArgument::Type::INT32)	return true; else return false; }
	bool isInt64(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex].type == OscArgument::Type::INT64)	return true; else return false; }
	bool isString(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex].type == OscArgument::Type::STRING)	return true; else return false; }
	bool isMidi(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex].type == OscArgument::Type::MIDI)		return true; else return false; }
	bool isTimetag(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex].type == OscArgument::Type::TIMETAG)	return true; else return false; }

	bool matchesAddress(const char* address) { return strcmp(address, address_string) == 0; }
	const char* getAddress() { return address_string; }
	uint64_t getPacketTimetag() { return timetag; }

	int getBuffer(char* outBuffer, int size);

private:

	uint64_t timetag;
	char address_string[128];
	std::vector<OscArgument> arguments;
};


namespace OscPacket {

	std::vector<std::shared_ptr<OscMessage>> getOscMessages(char* inBuffer, size_t size) {
		std::vector<std::shared_ptr<OscMessage>> output;
		if (tosc_isBundle(inBuffer)) {
			tosc_bundle bundle;
			tosc_parseBundle(&bundle, inBuffer, size);
			tosc_message message;
			uint64_t timetag = tosc_getTimetag(&bundle);
			while (tosc_getNextMessage(&bundle, &message)) {
				output.push_back(std::make_shared<OscMessage>(message.buffer, message.len));
			}
		}
		else {
			tosc_message message;
			if (0 == tosc_parseMessage(&message, inBuffer, size)) {
				output.push_back(std::make_shared<OscMessage>(inBuffer, size));
			}
		}
		return output;
	}

}