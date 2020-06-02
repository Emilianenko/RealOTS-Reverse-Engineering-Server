#include "pch.h"

#include "script.h"
#include "tools.h"

#include <boost/lexical_cast.hpp>

ScriptReader::~ScriptReader() {
	for (int32_t i = recursion_depth; i != -1; i--) {
		fclose(file[i]);
	}
}

bool ScriptReader::loadScript(const std::string& filename_)
{
	if (recursion_depth > static_cast<int32_t>(file.size())) {
		fmt::printf("ERROR - ScriptReader::loadScript: recursion-depth too high %d-%d '%s'.\n", recursion_depth, file.size(), filename_);
		return false;
	}

	recursion_depth++;
	file[recursion_depth] = fopen(filename_.c_str(), "rb");
	if (file[recursion_depth] == nullptr) {
		recursion_depth--;
		file[recursion_depth] = nullptr;
		fmt::printf("ERROR: Failed to open script-file '%s'\n", filename_);
		return false;
	}

	filename[recursion_depth] = filename_;
	line[recursion_depth] = 1;
	is_open = true;
	return true;
}

void ScriptReader::error(const std::string& text)
{
	if (!is_open) {
		return;
	}

	is_open = false;
	fmt::printf("ERROR - ScriptReader::error: script-file '%s': %s:%d\n", filename[recursion_depth], text, line[recursion_depth]);
}

TOKENTYPE ScriptReader::getToken() const
{
	return token;
}

TOKENTYPE ScriptReader::nextToken()
{
	while (canRead()) {
		char c = getc(file[recursion_depth]);
		if (c == -1) {
			if (recursion_depth == 0) {
				is_open = false;
			}

			token = TOKEN_ENDOFFILE;

			if (recursion_depth > 0) {
				fclose(file[recursion_depth]);
				--recursion_depth;
			}

			break;
		}

		if (c == '@') {
			if (!loadScript(readString())) {
				error("failed to load script-file");
				return TOKEN_ENDOFFILE;
			}
		} else if (c == ' ' || c == '\t') {
			continue;
		} else if (c == '\n') {
			line[recursion_depth]++;
		} else if (c == '#') {
			while (canRead()) {
				c = getc(file[recursion_depth]);
				if (c == '\n') {
					line[recursion_depth]++;
					break;
				}
			}
		} else if (isalpha(c)) {
			std::ostringstream ss;
			ss << c;

			while (canRead()) {
				c = getc(file[recursion_depth]);
				if (isalpha(c) || c == '_') {
					ss << c;
				} else {
					ungetc(c, file[recursion_depth]);
					break;
				}
			}

			identifier = ss.str();
			std::transform(identifier.begin(), identifier.end(), identifier.begin(), ::tolower);
			token = TOKEN_IDENTIFIER;
			return token;
		} else if (isdigit(c)) {
			std::ostringstream ss;
			ss << c;
			while (canRead()) {
				c = getc(file[recursion_depth]);
				if (isdigit(c)) {
					ss << c;
				} else {
					ungetc(c, file[recursion_depth]);
					break;
				}
			}

			number = ::getNumber(ss.str());
			token = TOKEN_NUMBER;
			return token;
		} else if (c == '"') {
			std::ostringstream ss;

			while (canRead()) {
				c = getc(file[recursion_depth]);

				if (c == '"') {
					break;
				} else if (c == '\\') {
					c = getc(file[recursion_depth]);
					if (c == 'n') {
						ss << '\n';
					} else if (c == '\\') {
						ss << '\\';
					} else if (c == '"') {
						ss << '"';
					} else {
						ss.clear();
						ss << "unknown special-char '" << static_cast<char>(c) << "'";
						error(ss.str());
						return TOKEN_ENDOFFILE;
					}
				} else {
					ss << c;
				}
			}

			string = ss.str();
			token = TOKEN_STRING;
			return token;
		} else {
			special = c;
			token = TOKEN_SPECIAL;
			return token;
		}
	}

	return TOKEN_ENDOFFILE;
}

int32_t ScriptReader::getNumber()
{
	if (token != TOKEN_NUMBER) {
		error("number expected");
	}

	return number;
}

const std::string& ScriptReader::getString()
{
	if (token != TOKEN_STRING) {
		error("string expected");
	}

	return string;
}

const std::string& ScriptReader::getIdentifier()
{
	if (token != TOKEN_IDENTIFIER) {
		error("identifier expected");
	}

	return identifier;
}

char ScriptReader::getSpecial()
{
	if (token != TOKEN_SPECIAL) {
		error("special expected");
	}

	return special;
}

int32_t ScriptReader::readNumber()
{
	nextToken();

	int32_t value = 1;
	if (token == TOKEN_SPECIAL && special == '-') {
		value = -1;
		nextToken();
	}

	if (token != TOKEN_NUMBER) {
		error("number expected");
	}

	return number * value;
}

const std::string& ScriptReader::readString()
{
	nextToken();
	if (token != TOKEN_STRING) {
		error("string expected");
	}

	return string;
}

const std::string& ScriptReader::readIdentifier()
{
	nextToken();
	if (token != TOKEN_IDENTIFIER) {
		error("identifier expected");
	}

	return identifier;
}

char ScriptReader::readSpecial()
{
	nextToken();
	if (token != TOKEN_SPECIAL) {
		error("special-char expected");
	}

	return special;
}

char ScriptReader::readSymbol(char symbol)
{
	nextToken();
	if (token != TOKEN_SPECIAL) {
		error("special-char expected");
	}

	if (special != symbol) {
		std::ostringstream ss;
		ss << "'" << symbol << "' expected";
		error(ss.str());
	}

	return special;
}
