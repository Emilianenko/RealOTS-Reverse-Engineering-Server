#pragma once

enum TOKENTYPE
{
	TOKEN_ENDOFFILE,
	TOKEN_IDENTIFIER,
	TOKEN_STRING,
	TOKEN_NUMBER,
	TOKEN_SPECIAL,
};

class ScriptReader
{
public:
	~ScriptReader();

	bool canRead() const {
		return file[recursion_depth] && is_open;
	}

	bool loadScript(const std::string& filename_);

	void error(const std::string& Text);

	TOKENTYPE getToken() const;
	TOKENTYPE nextToken();
	int32_t getNumber();
	const std::string& getString();
	const std::string& getIdentifier();
	char getSpecial();

	int32_t readNumber();
	const std::string& readString();
	const std::string& readIdentifier();
	char readSpecial();
	char readSymbol(char symbol);
private:
	std::array<FILE*, 4> file;
	std::string filename[4];
	uint32_t line[4]{ 0, 0, 0, 0 };
	TOKENTYPE token = TOKEN_ENDOFFILE;
	std::string string;
	std::string identifier;
	int32_t number = 0;
	char special = 0;
	bool is_open = false;
	int32_t recursion_depth = -1;
};