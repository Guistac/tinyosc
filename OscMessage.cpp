#include "OscMessage.h"

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#if _WIN32
#include <winsock2.h>
#define tosc_strncpy(_dst, _src, _len) strncpy_s(_dst, _len, _src, _TRUNCATE)
#else
#include <netinet/in.h>
#define tosc_strncpy(_dst, _src, _len) strncpy(_dst, _src, _len)
#endif
#if __unix__ && !__APPLE__
#include <endian.h>
#define htonll(x) htobe64(x)
#define ntohll(x) be64toh(x)
#endif

#include "OscMessage.h"


int OscMessage::getBuffer(char* outBuffer, int size) {
    const char* address = address_string;
    int len = size;
    char* buffer = outBuffer;
    char format[64];
    for (int i = 0; i < arguments.size(); i++) {
        format[i] = arguments[i].getChar();
    }
    format[arguments.size()] = 0;


    memset(buffer, 0, len); // clear the buffer
    uint32_t i = (uint32_t)strlen(address);
    if (address == NULL || i >= len) return -1;
    tosc_strncpy(outBuffer, address, len);
    i = (i + 4) & ~0x3;
    buffer[i++] = ',';
    int s_len = (int)strlen(format);
    if (format == NULL || (i + s_len) >= len) return -2;
    tosc_strncpy(buffer + i, format, len - i - s_len);
    i = (i + 4 + s_len) & ~0x3;

    for (int j = 0; format[j] != '\0'; ++j) {
        switch (format[j]) {
        case 'b': {
            OscBlob* oscBlob = (OscBlob*)&arguments[j];
            const uint32_t n = (uint32_t)oscBlob->size; // length of blob
            if (i + 4 + n > len) return -3;
            char* b = (char*)oscBlob->data; // pointer to binary data
            *((uint32_t*)(buffer + i)) = htonl(n); i += 4;
            memcpy(buffer + i, b, n);
            i = (i + 3 + n) & ~0x3;
            break;
        }
        case 'f': {
            OscFloat* oscFloat = (OscFloat*)&arguments[j];
            if (i + 4 > len) return -3;
            const float f = (float)oscFloat->data;
            *((uint32_t*)(buffer + i)) = htonl(*((uint32_t*)&f));
            i += 4;
            break;
        }
        case 'd': {
            OscDouble* oscDouble = (OscDouble*)&arguments[j];
            if (i + 8 > len) return -3;
            const double f = (double)oscDouble->data;
            *((uint64_t*)(buffer + i)) = htonll(*((uint64_t*)&f));
            i += 8;
            break;
        }
        case 'i': {
            OscInt32* oscInt32 = (OscInt32*)&arguments[j];
            if (i + 4 > len) return -3;
            const uint32_t k = (uint32_t)oscInt32->data;
            *((uint32_t*)(buffer + i)) = htonl(k);
            i += 4;
            break;
        }
        case 'm': {
            OscMidi* oscMidi = (OscMidi*)&arguments[i];
            if (i + 4 > len) return -3;
            const unsigned char* const k = (unsigned char*)oscMidi->portId;
            memcpy(buffer + i, k, 4);
            i += 4;
            break;
        }
        case 't':
        case 'h': {
            OscInt64* oscInt64 = (OscInt64*)&arguments[i];
            if (i + 8 > len) return -3;
            const uint64_t k = (uint64_t)oscInt64->data;
            *((uint64_t*)(buffer + i)) = htonll(k);
            i += 8;
            break;
        }
        case 's': {
            OscString* oscString = (OscString*)&arguments[i];
            const char* str = (const char*)oscString->data;
            s_len = (int)strlen(str);
            if (i + s_len >= len) return -3;
            tosc_strncpy(buffer + i, str, len - i - s_len);
            i = (i + 4 + s_len) & ~0x3;
            break;
        }
        case 'T': // true
        case 'F': // false
        case 'N': // nil
        case 'I': // infinitum
            break;
        default: return -4; // unknown type
        }
    }

    return i; // return the total number of bytes written
}



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