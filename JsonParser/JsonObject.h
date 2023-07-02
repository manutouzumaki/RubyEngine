#pragma once

enum JsonValueType
{
    VALUE_NONE,
    VALUE_CHARACTER,
    VALUE_FLOAT,
    VALUE_BOOL,
    VALUE_NULL,
    VALUE_OBJECT
};

struct JsonObject;

struct JsonValue
{
    union
    {
        char* valueChar;
        float valueFloat;
        bool  valueBool;
        void* valueNull;
        JsonObject* valueObject;
    };
    JsonValueType type;
    JsonValue* next;
};

class JsonObject
{
public:
    char* name;
    JsonValue* firstValue;
    JsonValue* lastValue;

    JsonObject* firstChild;
    JsonObject* firstSibling;

    JsonObject();
    ~JsonObject();

    JsonObject* GetChild();
    JsonObject* GetSibling();
    JsonValue* GetFirstValue();

    JsonObject* GetChildByName(const char* name);
    JsonObject* GetSiblingByName(const char* name);

    void SetName(const char *name);
    void AddValue(const char* valueChar);
    void AddValue(float valueFloat);
    void AddValue(bool valueBool);
    void AddValue(void* null);
    void AddValue(JsonObject* valueObject);
    void AddChild(JsonObject* child);
    void AddSibling(JsonObject* sibling);

    bool SaveToFile(const char* filepath);

private:
    void SetValue(JsonValue* value);

    void SaveRoot(void* hFile);
    void SaveChild(void* hFile);
    void SaveSibling(void* hFile);

    void SaveName(void* hFile);
    void SaveValue(void* hFile);

    

};


