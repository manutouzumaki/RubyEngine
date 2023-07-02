#include "JsonParser.h"
#include <Windows.h>

JsonParser::JsonParser() 
{
    mRoot = nullptr;
    mCurrent = 0;
}

JsonParser::~JsonParser()
{
    if (mRoot) delete mRoot;
}

size_t JsonParser::ParseFile(const char* filepath)
{

    HANDLE hFile = CreateFileA(filepath, GENERIC_READ,
        FILE_SHARE_READ, 0, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error openging file: %s\n", filepath);
        return 0;
    }

    LARGE_INTEGER bytesToRead = {};
    GetFileSizeEx(hFile, &bytesToRead);
    if (bytesToRead.QuadPart <= 0)
    {
        printf("Error file: %s is empty\n", filepath);
        return 0;
    }

    char* fileData = new char[bytesToRead.QuadPart];
    size_t bytesReaded = 0;

    if (!ReadFile(hFile, (LPVOID)fileData, bytesToRead.QuadPart, (LPDWORD)&bytesReaded, 0))
    {
        printf("Error reading file: %s\n", filepath);
        return 0;
    }

    CloseHandle(hFile);

    JsonScanner* scanner = new JsonScanner();

    scanner->Scan(fileData, bytesToRead.QuadPart);

    //scanner->PrintTokens();

    std::vector<JsonToken>& tokens = scanner->GetTokens();
    char* source = scanner->GetSource();

    GenerateJsonTree(&mRoot, tokens, source);

    delete scanner;
    delete[] fileData;

    return bytesToRead.QuadPart;
}

JsonObject* JsonParser::GetRoot()
{
    return mRoot;
}

void JsonParser::FillObject(JsonObject* object, std::vector<JsonToken>& tokens, char* source)
{
    JsonObject* currentObject = object;
    JsonToken token = tokens[mCurrent];
    while (token.type != TOKEN_COMMA && token.type != TOKEN_RIGHT_BRACE && token.type != TOKEN_EOF) {

        // set name
        if (token.type == TOKEN_STRING && tokens[mCurrent + 1].type == TOKEN_COLOM)
        {
            JsonToken token = tokens[mCurrent];
            currentObject->name = new char[token.size + 1];
            memcpy(currentObject->name, source + token.offset, token.size);
            currentObject->name[token.size] = '\0';

        }
        // set string
        else if (token.type == TOKEN_STRING) {

            JsonValue* newValue = new JsonValue;
            memset(newValue, 0, sizeof(JsonValue));

            newValue->valueChar = new char[token.size + 1];
            memcpy(newValue->valueChar, source + token.offset, token.size);
            newValue->valueChar[token.size] = '\0';
            newValue->type = VALUE_CHARACTER;
            newValue->next = nullptr;

            currentObject->firstValue = newValue;
            currentObject->lastValue = currentObject->firstValue;
        }
        // set number
        else if (token.type == TOKEN_NUMBER) {

            JsonValue* newValue = new JsonValue;
            memset(newValue, 0, sizeof(JsonValue));

            static char buffer[32];
            memcpy(buffer, source + token.offset, token.size);
            buffer[token.size] = '\0';
            newValue->valueFloat = (float)atof(buffer);
            newValue->type = VALUE_FLOAT;
            newValue->next = nullptr;

            currentObject->firstValue = newValue;
            currentObject->lastValue = currentObject->firstValue;

        }
        // set bool
        else if (token.type == TOKEN_BOOL) {

            JsonValue* newValue = new JsonValue;
            memset(newValue, 0, sizeof(JsonValue));

            bool value = true;
            if (strncmp(source + token.offset, "false", token.size) == 0)
            {
                value = false;
            }

            newValue->valueBool = value;
            newValue->type = VALUE_BOOL;
            newValue->next = nullptr;

            currentObject->firstValue = newValue;
            currentObject->lastValue = currentObject->firstValue;
        }
        // set null
        else if (token.type == TOKEN_NULL)
        {
            JsonValue* newValue = new JsonValue;
            memset(newValue, 0, sizeof(JsonValue));
            newValue->valueNull = nullptr;
            newValue->type = VALUE_NULL;
            newValue->next = nullptr;

            currentObject->firstValue = newValue;
            currentObject->lastValue = currentObject->firstValue;

        }
        // add a child
        else if (token.type == TOKEN_LEFT_BRACE)
        {
            mCurrent++;
            AddObject(&object->firstChild, tokens, source);
        }
        // add array
        else if (token.type == TOKEN_LEFT_BRACK)
        {
            mCurrent++;
            AddArray(object, tokens, source);
        }

        if (mCurrent < (tokens.size() - 1))
        {
            token = tokens[++mCurrent];
        }

    }

    return;
}

void JsonParser::AddArray(JsonObject* object, std::vector<JsonToken>& tokens, char* source)
{
    JsonObject* currentObject = object;
    JsonToken token = tokens[mCurrent];
    while (token.type != TOKEN_RIGHT_BRACK && token.type != TOKEN_EOF) {

        // set string
        if (token.type == TOKEN_STRING) {

            JsonValue* newValue = new JsonValue;
            memset(newValue, 0, sizeof(JsonValue));
            newValue->valueChar = new char[token.size + 1];
            memcpy(newValue->valueChar, source + token.offset, token.size);
            newValue->valueChar[token.size] = '\0';
            newValue->type = VALUE_CHARACTER;
            newValue->next = nullptr;

            if (currentObject->firstValue == nullptr)
            {
                currentObject->firstValue = newValue;
                currentObject->lastValue = currentObject->firstValue;
            }
            else
            {
                currentObject->lastValue->next = newValue;
                currentObject->lastValue = currentObject->lastValue->next;
            }
        }
        // set number
        else if (token.type == TOKEN_NUMBER) {

            JsonValue* newValue = new JsonValue;
            memset(newValue, 0, sizeof(JsonValue));
            static char buffer[32];
            memcpy(buffer, source + token.offset, token.size);
            buffer[token.size] = '\0';
            newValue->valueFloat = (float)atof(buffer);
            newValue->type = VALUE_FLOAT;
            newValue->next = nullptr;

            if (currentObject->firstValue == nullptr)
            {
                currentObject->firstValue = newValue;
                currentObject->lastValue = currentObject->firstValue;
            }
            else
            {
                currentObject->lastValue->next = newValue;
                currentObject->lastValue = currentObject->lastValue->next;
            }
        }
        // set bool
        else if (token.type == TOKEN_BOOL) {

            JsonValue* newValue = new JsonValue;
            memset(newValue, 0, sizeof(JsonValue));
            bool value = true;
            if (strncmp(source + token.offset, "false", token.size) == 0)
            {
                value = false;
            }

            newValue->valueBool = value;
            newValue->type = VALUE_BOOL;
            newValue->next = nullptr;

            if (currentObject->firstValue == nullptr)
            {
                currentObject->firstValue = newValue;
                currentObject->lastValue = currentObject->firstValue;
            }
            else
            {
                currentObject->lastValue->next = newValue;
                currentObject->lastValue = currentObject->lastValue->next;
            }
        }
        // set null
        else if (token.type == TOKEN_NULL)
        {
            JsonValue* newValue = new JsonValue;
            memset(newValue, 0, sizeof(JsonValue));
            newValue->valueNull = nullptr;
            newValue->type = VALUE_NULL;
            newValue->next = nullptr;

            if (currentObject->firstValue == nullptr)
            {
                currentObject->firstValue = newValue;
                currentObject->lastValue = currentObject->firstValue;
            }
            else
            {
                currentObject->lastValue->next = newValue;
                currentObject->lastValue = currentObject->lastValue->next;
            }
        }
        // add a child
        else if (token.type == TOKEN_LEFT_BRACE)
        {
            
            JsonValue* newValue = new JsonValue;
            memset(newValue, 0, sizeof(JsonValue));
            newValue->type = VALUE_OBJECT;
            newValue->next = nullptr;

            if (currentObject->firstValue == nullptr)
            {
                currentObject->firstValue = newValue;
                currentObject->lastValue = currentObject->firstValue;
            }
            else
            {
                currentObject->lastValue->next = newValue;
                currentObject->lastValue = currentObject->lastValue->next;
            }

            mCurrent++;
            AddObject(&currentObject->lastValue->valueObject, tokens, source);
        }

        if (mCurrent < (tokens.size() - 1))
        {
            token = tokens[++mCurrent];
        }

    }
    return;
}

void JsonParser::AddObject(JsonObject** object, std::vector<JsonToken>& tokens, char* source)
{
    (*object) = new JsonObject;
    FillObject((*object), tokens, source);
    JsonToken token = tokens[mCurrent];
    if (token.type == TOKEN_COMMA)
    {
        mCurrent++;
        AddObject(&(*object)->firstSibling, tokens, source);
    }
}

void JsonParser::GenerateJsonTree(JsonObject** root, std::vector<JsonToken>& tokens, char* source)
{
    (*root) = new JsonObject;
    FillObject((*root), tokens, source);

}
