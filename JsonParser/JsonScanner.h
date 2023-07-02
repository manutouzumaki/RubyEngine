#pragma once

#include <vector>

enum JsonTokenType
{
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACK, TOKEN_RIGHT_BRACK,
    TOKEN_COMMA,
    TOKEN_COLOM,
    TOKEN_SPECIAL,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_NULL,
    TOKEN_BOOL,
    TOKEN_EOF
};

struct JsonToken
{
    JsonTokenType type;
    size_t offset;
    size_t size;
};

class JsonScanner
{
public:

    JsonScanner();
    ~JsonScanner();

    void PrintTokens();
    void Scan(char* buffer, size_t bufferSize);
    std::vector<JsonToken>& GetTokens();
    char* GetSource();

private:

    bool IsDigit(char c);
    bool IsNegativeDigit(char c, char n);
    char Advance();
    char Peek();
    char PeekNext();
    char PeekNextNext();

    void AddToken(JsonTokenType type);
    void AddNumberToken();
    void AddStringToken();
    void AddConstantToken(JsonTokenType type);

    char* mSource;
    size_t mEnd;
    size_t mCurrent;
    size_t mLine;
    std::vector<JsonToken> mTokens;
};