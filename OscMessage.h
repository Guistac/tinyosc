#pragma once

#include "tinyosc.h"
#include <vector>
#include <memory>

class OscArgument {
public:
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
	virtual char getChar() = 0;// { return 'u'; }
};

class OscBlob : public OscArgument {
public:
	OscBlob(const char* d, int s) {
		data = new char[s];
		size = s;
		memcpy(data, d, s);
		type = Type::BLOB;
	}
	~OscBlob() {
		delete[] data;
	}
	virtual char getChar() { return 'b'; }
	char* data;
	int size;
};

class OscFloat : public OscArgument {
public:
	OscFloat(float d) : data(d) { type = Type::FLOAT; }
	virtual char getChar() { return 'f'; }
	float data;
};

class OscDouble : public OscArgument {
public:
	OscDouble(double d) : data(d) { type = Type::DOUBLE; }
	virtual char getChar() { return 'd'; }
	double data;
};

class OscInt32 : public OscArgument {
public:
	OscInt32(int32_t d) : data(d) { type = Type::INT32; }
	virtual char getChar() { return 'i'; }
	int32_t data;
};

class OscInt64 : public OscArgument {
public:
	OscInt64(int64_t d) : data(d) { type = Type::INT64; }
	virtual char getChar() { return 'h'; }
	int64_t data;
};

class OscString : public OscArgument {
public:
	OscString(const char* d) {
		strcpy(data, d);
		type = Type::STRING;
	}
	virtual char getChar() { return 's'; }
	char data[128];
};

class OscMidi : public OscArgument {
public:
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

class OscTimetag : public OscArgument {
public:
	OscTimetag(uint64_t d) : data(d) { type = Type::TIMETAG; }
	virtual char getChar() { return 't'; }
	uint64_t data;
};

class OscBool : public OscArgument {
public:
	OscBool(bool d) : data(d) { type = Type::BOOL; }
	virtual char getChar() { return data ? 'T' : 'F'; }
	bool data;
};



class OscMessage {
public:

	OscMessage(char* inputBuffer, size_t size);
	OscMessage(const char* address) {
		strcpy(address_string, address);
	}
	~OscMessage() {
		for (auto argument : arguments) delete argument;
	}

	void addBlob(char* buffer, size_t size) { arguments.push_back(new OscBlob(buffer, size)); }
	void addFloat(float data) { arguments.push_back(new OscFloat(data)); }
	void addDouble(double data) { arguments.push_back(new OscDouble(data)); }
	void addInt32(int32_t data) { arguments.push_back(new OscInt32(data)); }
	void addInt64(int64_t data) { arguments.push_back(new OscInt64(data)); }
	void addString(const char* data) { arguments.push_back(new OscString(data)); }
	void addMidi(char port, char statusByte, char data1, char data2) { arguments.push_back(new OscMidi(port, statusByte, data1, data2)); }
	void addTimetag(uint64_t data) { arguments.push_back(new OscTimetag(data)); }
	void addBool(bool data) { arguments.push_back(new OscBool(data)); }

	bool isBlob(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex]->type == OscArgument::Type::BLOB)		return true; else return false; }
	bool isFloat(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex]->type == OscArgument::Type::FLOAT)	return true; else return false; }
	bool isDouble(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex]->type == OscArgument::Type::DOUBLE)	return true; else return false; }
	bool isInt32(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex]->type == OscArgument::Type::INT32)	return true; else return false; }
	bool isInt64(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex]->type == OscArgument::Type::INT64)	return true; else return false; }
	bool isString(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex]->type == OscArgument::Type::STRING)	return true; else return false; }
	bool isMidi(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex]->type == OscArgument::Type::MIDI)		return true; else return false; }
	bool isTimetag(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex]->type == OscArgument::Type::TIMETAG)	return true; else return false; }
	bool isBool(int argumentIndex) { if (argumentIndex < arguments.size() && arguments[argumentIndex]->type == OscArgument::Type::BOOL)		return true; else return false; }

	int getBlob(int argumentIndex, char** output);
	float getFloat(int argumentIndex);
	double getDouble(int argumentIndex);
	int32_t getInt32(int argumentIndex);
	int64_t getInt64(int argumentIndex);
	const char* getString(int argumentIndex);
	void getMidi(int argumentIndex, char* portInfo, char* statusByte, char* data1, char* data2);
	uint64_t getTimetag(int argumentIndex);
	bool getBool(int argumentIndex);


	bool matchesAddress(const char* address) { return strcmp(address, address_string) == 0; }
	const char* getAddress() { return address_string; }
	uint64_t getPacketTimetag() { return timetag; }

	int getBuffer(char* outBuffer, int size);

private:

	uint64_t timetag = 0;
	char address_string[128];
	std::vector<OscArgument*> arguments;
};


namespace OscPacket {

	std::vector<std::shared_ptr<OscMessage>> getOscMessages(char* inBuffer, size_t size);

}