#include "JsonScanner.h"

static const char* gTokenStrings[] =
{
    "TOKEN_LEFT_PAREN  ",
    "TOKEN_RIGHT_PAREN ",
    "TOKEN_LEFT_BRACE  ",
    "TOKEN_RIGHT_BRACE ",
    "TOKEN_LEFT_BRACK  ",
    "TOKEN_RIGHT_BRACK ",
    "TOKEN_COMMA       ",
    "TOKEN_COLOM       ",
    "TOKEN_SPECIAL     ",
    "TOKEN_STRING      ",
    "TOKEN_NUMBER      ",
    "TOKEN_NULL        ",
    "TOKEN_BOOL        ",
    "TOKEN_EOF         "
};

JsonScanner::JsonScanner()
    : mSource(nullptr), mEnd(0), mCurrent(0), mLine(0)
{
    mTokens.clear();
}

JsonScanner::~JsonScanner() 
{
    mTokens.clear();
}

bool JsonScanner::IsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

bool JsonScanner::IsNegativeDigit(char c, char n)
{
    return (c == '-' && IsDigit(n));
}

char JsonScanner::Advance()
{
    return mSource[mCurrent++];
}

char JsonScanner::Peek()
{
    if (mCurrent >= mEnd) return '\0';
    return mSource[mCurrent];
}

char JsonScanner::PeekNext()
{
    if (mCurrent + 1 >= mEnd) return '\0';
    return mSource[mCurrent + 1];
}

char JsonScanner::PeekNextNext()
{
    if (mCurrent + 2 >= mEnd) return '\0';
    return mSource[mCurrent + 2];
}

void JsonScanner::AddToken(JsonTokenType type)
{
    JsonToken token{};
    token.type = type;
    token.offset = mCurrent - 1;
    token.size = 1;

    mTokens.push_back(token);
}

void JsonScanner::AddNumberToken()
{
    JsonToken token;

    token.offset = mCurrent - 1;

    while (IsDigit(Peek()))
    {
        Advance();
    }
    // look for the fractional part
    if (Peek() == '.' && IsDigit(PeekNext()))
    {
        Advance();
        while (IsDigit(Peek())) Advance();
    }
    // look for scientific notation
    if ((Peek() == 'e' || Peek() == 'E') &&
        (PeekNext() == '-' || PeekNext() == '+') &&
        IsDigit(PeekNextNext()))
    {
        Advance(); Advance();
        while (IsDigit(Peek())) Advance();
    }

    token.size = mCurrent - token.offset;
    token.type = TOKEN_NUMBER;

    mTokens.push_back(token);

}

void JsonScanner::AddStringToken()
{
    JsonToken token;
    token.type = TOKEN_STRING;
    token.offset = mCurrent;

    while (Peek() != '"' && mCurrent <= mEnd)
    {
        if (Peek() == '\n')
        {
            mLine++;
        }
        Advance();
    }

    if (mCurrent >= mEnd)
    {
        printf("Error: Unterminated String");
        return;
    }
    Advance();

    token.size = (mCurrent - 1) - token.offset;

    mTokens.push_back(token);
}

void JsonScanner::AddConstantToken(JsonTokenType type)
{
    JsonToken token;
    token.type = type;
    token.offset = mCurrent - 1;

    char* constant = mSource + token.offset;
    if (strncmp(constant, "false", 5) == 0)
    {
        mCurrent += 4;
        token.size = mCurrent - token.offset;
        mTokens.push_back(token);
    }
    else if (strncmp(constant, "true", 4) == 0)
    {
        mCurrent += 3;
        token.size = mCurrent - token.offset;
        mTokens.push_back(token);
    }
    else if (strncmp(constant, "null", 4) == 0)
    {
        mCurrent += 3;
        token.size = mCurrent - token.offset;
        mTokens.push_back(token);
    }
    else
    {
        printf("Error: Invalid Constant\n");
        while (Peek() != ' ' && mCurrent <= mEnd)
        {
            if (Peek() == '\n')
            {
                mLine++;
            }
            Advance();
        }
    }
}

void JsonScanner::PrintTokens()
{
    for (int i = 0; i < mTokens.size(); ++i)
    {
        JsonToken token = mTokens[i];

        char* nullTerString = (char*)malloc(token.size + 1);
        memcpy((void*)nullTerString, (void*)(mSource + token.offset), token.size);
        nullTerString[token.size] = '\0';

        printf("%s: offset %zd, size: %zd data: %s\n", gTokenStrings[token.type], token.offset, token.size, nullTerString);

        free(nullTerString);

    }
}

void JsonScanner::Scan(char* buffer, size_t bufferSize)
{
    mTokens.clear();
    mSource = buffer;
    mEnd = bufferSize;
    mCurrent = 0;
    mLine = 0;

    do
    {
        char c = Advance();
        switch (c)
        {
        case '(': AddToken(TOKEN_LEFT_PAREN); break;
        case ')': AddToken(TOKEN_RIGHT_PAREN); break;
        case '{': AddToken(TOKEN_LEFT_BRACE); break;
        case '}': AddToken(TOKEN_RIGHT_BRACE); break;
        case '[': AddToken(TOKEN_LEFT_BRACK); break;
        case ']': AddToken(TOKEN_RIGHT_BRACK); break;
        case ',': AddToken(TOKEN_COMMA); break;
        case ':': AddToken(TOKEN_COLOM); break;
        case 'f': AddConstantToken(TOKEN_BOOL); break;
        case 't': AddConstantToken(TOKEN_BOOL); break;
        case 'n': AddConstantToken(TOKEN_NULL); break;

            // ignore ...
        case ' ':
        case '\r':
        case '\t':
        case '.':
        case '\0':
            break;
            // keep track of the line we are in
        case '\n':
            mLine++;
            break;
        case '"':
            AddStringToken();
            break;
        default:
        {
            if (IsDigit(c))
            {
                AddNumberToken();
            }
            else if (IsNegativeDigit(c, Peek()))
            {
                AddNumberToken();
            }
            else
            {
                printf("Error: character not Handled LINE: %zd\n", mLine + 1);
            }
        } break;
        }
    } while (mCurrent < mEnd);

    JsonToken token{};
    token.offset = mCurrent;
    token.size = 0;
    token.type = TOKEN_EOF;
    mTokens.push_back(token);

}

std::vector<JsonToken>& JsonScanner::GetTokens()
{
    return mTokens;
}

char* JsonScanner::GetSource()
{
    return mSource;
}
