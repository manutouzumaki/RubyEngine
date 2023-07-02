#pragma once

// TODO: try to create a better json parser than before
#include "JsonScanner.h"
#include "JsonObject.h"

class JsonParser
{
public:

    JsonParser();
    ~JsonParser();

    size_t ParseFile(const char* filepath);
    JsonObject* GetRoot();

private:

    void FillObject(JsonObject* object, std::vector<JsonToken>& tokens, char* source);
    void AddArray(JsonObject* object, std::vector<JsonToken>& tokens, char* source);
    void AddObject(JsonObject** object, std::vector<JsonToken>& tokens, char* source);
    void GenerateJsonTree(JsonObject** root, std::vector<JsonToken>& tokens, char* source);

    JsonObject* mRoot;
    size_t mCurrent;
};



