#include "pch.h"

#include "networkmessage.h"
#include "rsa.h"

uint16_t NetworkMessage::getLengthHeader() const
{
	return static_cast<uint16_t>(buffer[0] | buffer[1] << 8);
}

 void NetworkMessage::skipBytes(int16_t amount)
{
	position += amount;
}

 uint8_t NetworkMessage::readByte()
 {
	 if (!canRead(1)) {
		 return 0;
	 }

	 return buffer[position++];
 }

 uint16_t NetworkMessage::readWord()
{
	if (!canRead(2)) {
		return 0;
	}

	const uint16_t value = buffer[position + 1] << 8 | buffer[position];
	position += 2;
	return value;
}

 uint32_t NetworkMessage::readQuad()
{
	if (!canRead(4)) {
		return 0;
	}

	uint32_t value = 0;
	value |= buffer[position + 3] << 24;
	value |= buffer[position + 2] << 16;
	value |= buffer[position + 1] << 8;
	value |= buffer[position];
	position += 4;
	return value;
}

std::string NetworkMessage::readString(uint16_t stringLen/* = 0*/)
{
	if (stringLen == 0) {
		stringLen = readWord();
	}

	if (!canRead(stringLen)) {
		return std::string();
	}

	char* v = reinterpret_cast<char*>(buffer) + position; //does not break strict aliasing
	position += stringLen;
	return std::string(v, stringLen);
}

 void NetworkMessage::writeByte(uint8_t value)
{
	if (!canWrite(1)) {
		return;
	}

	buffer[position++] = value;
	length++;
}

 void NetworkMessage::writeWord(uint16_t value)
{
	if (!canWrite(2)) {
		return;
	}

	buffer[position] = value;
	buffer[position + 1] = value >> 8;
	position += 2;
	length += 2;
}

 void NetworkMessage::writeQuad(uint32_t value)
{
	if (!canWrite(4)) {
		return;
	}

	buffer[position] = value;
	buffer[position + 1] = value >> 8;
	buffer[position + 2] = value >> 16;
	buffer[position + 3] = value >> 24;
	position += 4;
	length += 4;
}

void NetworkMessage::writeString(const std::string& value)
{
	const uint32_t stringLen = value.length();
	if (!canWrite(stringLen + 2) || stringLen > 8192) {
		return;
	}

	writeWord(stringLen);
	memcpy(buffer + position, value.c_str(), stringLen);
	position += stringLen;
	length += stringLen;
}

 void NetworkMessage::writeHeader()
{
	header_position -= 2;
	memcpy(buffer + header_position, &length, 2);
	length += 2;
}

void NetworkMessage::xteaEncrypt(uint32_t* key)
{
	const uint32_t delta = 0x61C88647;

	// The message must be a multiple of 8
	const uint32_t paddingBytes = length % 8;
	if (paddingBytes != 0) {
		writePaddingBytes(8 - paddingBytes);
	}

	uint8_t* bufferPtr = buffer + header_position;
	const uint32_t messageLength = length;
	uint32_t readPos = 0;
	const uint32_t k[] = { key[0], key[1], key[2], key[3] };
	while (readPos < messageLength) {
		uint32_t v0;
		memcpy(&v0, bufferPtr + readPos, 4);
		uint32_t v1;
		memcpy(&v1, bufferPtr + readPos + 4, 4);

		uint32_t sum = 0;

		for (int32_t i = 32; --i >= 0;)
 {
			v0 += ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
			sum -= delta;
			v1 += ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[(sum >> 11) & 3]);
		}

		memcpy(bufferPtr + readPos, &v0, 4);
		readPos += 4;
		memcpy(bufferPtr + readPos, &v1, 4);
		readPos += 4;
	}
}

bool NetworkMessage::xteaDecrypt(uint32_t* key)
{
	if (((length - 2) % 8) != 0) {
		return false;
	}

	const uint32_t delta = 0x61C88647;

	uint8_t* bufferPtr = buffer + position;
	const uint32_t messageLength = length - 6;
	uint32_t readPos = 0;
	const uint32_t k[] = { key[0], key[1], key[2], key[3] };
	while (readPos < messageLength) {
		uint32_t v0;
		memcpy(&v0, bufferPtr + readPos, 4);
		uint32_t v1;
		memcpy(&v1, bufferPtr + readPos + 4, 4);

		uint32_t sum = 0xC6EF3720;

		for (int32_t i = 32; --i >= 0;) {
			v1 -= ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[(sum >> 11) & 3]);
			sum += delta;
			v0 -= ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
		}

		memcpy(bufferPtr + readPos, &v0, 4);
		readPos += 4;
		memcpy(bufferPtr + readPos, &v1, 4);
		readPos += 4;
	}

	const int innerLength = readWord();
	if (innerLength > length - 4) {
		return false;
	}

	length = innerLength;
	return true;
}

bool NetworkMessage::rsaDecrypt()
{
	if ((length - position) < 128) {
		return false;
	}

	RSA.decrypt(reinterpret_cast<char*>(buffer + position)); //does not break strict aliasing
	return readByte() == 0;
}

 bool NetworkMessage::canWrite(uint32_t size) const
{
	return (size + position) < MAX_BODY_LENGTH;
}

 bool NetworkMessage::canRead(int32_t size) const
 {
	if ((position + size) > (length + 8) || size >= (NETWORKMESSAGE_MAXSIZE - position)) {
		return false;
	}
	return true;
}

void NetworkMessage::writePaddingBytes(uint32_t n)
{
	if (!canWrite(n)) {
		return;
	}

	memset(buffer + position, 0x33, n);
	length += n;
}
