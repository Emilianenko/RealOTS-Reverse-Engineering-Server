#pragma once

static constexpr uint16_t NETWORKMESSAGE_MAXSIZE = 16384;

class NetworkMessage
{
public:
	enum { HEADER_LENGTH = 2 };
	enum { XTEA_MULTIPLE = 8 };
	enum { MAX_BODY_LENGTH = NETWORKMESSAGE_MAXSIZE - HEADER_LENGTH - XTEA_MULTIPLE };

	void setLength(uint16_t new_length) {
		length = new_length;
	}
	uint16_t getLength() const {
		return length;
	}
	void setPosition(uint16_t new_pos) {
		position = new_pos;
	}
	uint16_t getPosition() const {
		return position;
	}
	uint8_t* getBuffer() {
		return buffer;
	}

	uint16_t getLengthHeader() const;
	void skipBytes(int16_t amount);

	uint8_t readByte();
	uint16_t readWord();
	uint32_t readQuad();
	std::string readString(uint16_t stringLen = 0);

	void writeByte(uint8_t value);
	void writeWord(uint16_t value);
	void writeQuad(uint32_t value);
	void writePaddingBytes(uint32_t n);
	void writeString(const std::string& value);
	void writeHeader();

	void xteaEncrypt(uint32_t* key);
	bool xteaDecrypt(uint32_t* key);

	bool rsaDecrypt();
private:
	inline bool canWrite(uint32_t size) const;
	inline bool canRead(int32_t size) const;

	uint16_t header_position = 4;
	uint16_t position = 4;
	uint16_t length = 0;
	uint8_t buffer[NETWORKMESSAGE_MAXSIZE]{};
};