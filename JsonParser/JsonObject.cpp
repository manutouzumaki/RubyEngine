#include "JsonObject.h"

#include <Windows.h>
#include <stdio.h>
#include <assert.h>

JsonObject::JsonObject()
    : name(nullptr), firstValue(nullptr), lastValue(nullptr),
      firstChild(nullptr), firstSibling(nullptr) { }

JsonObject::~JsonObject()
{
    // destroy the objects in the value part (string or other objects)
    JsonValue* jsonValue = firstValue;
    while (jsonValue != nullptr)
    {
        if (jsonValue->type == VALUE_CHARACTER)
        {
            delete[] jsonValue->valueChar;
        }
        else if (jsonValue->type == VALUE_OBJECT)
        {
            delete jsonValue->valueObject;
        }
        JsonValue* nextValue = jsonValue->next;
        delete jsonValue;
        jsonValue = nextValue;
    }

    // destroy sibling
    JsonObject* jsonSibling = firstSibling;
    if (jsonSibling != nullptr)
    {
        delete jsonSibling;
    }

    // destroy child
    JsonObject* jsonChild = firstChild;
    if (jsonChild != nullptr)
    {
        delete jsonChild;
    }

    if (name) {
        delete[] name;
    }
}

JsonObject* JsonObject::GetChild()
{
    return firstChild;
}

JsonObject* JsonObject::GetSibling()
{
    return firstSibling;
}

JsonValue* JsonObject::GetFirstValue()
{
    return firstValue;
}

JsonObject* JsonObject::GetChildByName(const char* name)
{
    JsonObject* current = firstChild;
    while (current)
    {
        if (strcmp(name, current->name) == 0)
        {
            return current;
        }
        current = current->firstSibling;
    }
    return nullptr;
}

JsonObject* JsonObject::GetSiblingByName(const char* name)
{
    JsonObject* current = firstSibling;
    while (current)
    {
        if (strcmp(name, current->name) == 0)
        {
            return current;
        }
        current = current->firstSibling;
    }
    return nullptr;
}

void JsonObject::SetName(const char* name)
{
    size_t nameSize = strlen(name);
    this->name = new char[nameSize + 1];
    memcpy(this->name, name, nameSize);
    this->name[nameSize] = '\0';
}

void JsonObject::AddValue(const char* valueChar)
{
    JsonValue* newValue = new JsonValue;
    memset(newValue, 0, sizeof(JsonValue));

    size_t valueSize = strlen(valueChar);
    newValue->valueChar = new char[valueSize + 1];
    memcpy(newValue->valueChar, valueChar, valueSize);
    newValue->valueChar[valueSize] = '\0';
    newValue->type = VALUE_CHARACTER;
    newValue->next = nullptr;

    SetValue(newValue);
}

void JsonObject::AddValue(float valueFloat)
{
    JsonValue* newValue = new JsonValue;
    memset(newValue, 0, sizeof(JsonValue));
    
    newValue->valueFloat = valueFloat;
    newValue->type = VALUE_FLOAT;
    newValue->next = nullptr;

    SetValue(newValue);
}

void JsonObject::AddValue(bool valueBool)
{
    JsonValue* newValue = new JsonValue;
    memset(newValue, 0, sizeof(JsonValue));

    newValue->valueBool = valueBool;
    newValue->type = VALUE_BOOL;
    newValue->next = nullptr;

    SetValue(newValue);
}

void JsonObject::AddValue(void* null)
{
    JsonValue* newValue = new JsonValue;
    memset(newValue, 0, sizeof(JsonValue));

    newValue->valueNull = null;
    newValue->type = VALUE_NULL;
    newValue->next = nullptr;

    SetValue(newValue);
}

void JsonObject::AddValue(JsonObject* valueObject)
{
    JsonValue* newValue = new JsonValue;
    memset(newValue, 0, sizeof(JsonValue));

    newValue->valueObject = valueObject;
    newValue->type = VALUE_OBJECT;
    newValue->next = nullptr;

    SetValue(newValue);
}

void JsonObject::SetValue(JsonValue* value)
{
    if (firstValue == nullptr)
    {
        firstValue = value;
        lastValue = firstValue;
    }
    else
    {
        lastValue->next = value;
        lastValue = lastValue->next;
    }
}

void JsonObject::AddChild(JsonObject* child)
{
    if (firstChild == nullptr)
    {
        firstChild = child;
    }
    else
    {
        JsonObject* lastSibling = firstChild;
        while (lastSibling->firstSibling != nullptr)
        {
            lastSibling = lastSibling->firstSibling;
        }
        lastSibling->firstSibling = child;
    }
}

void JsonObject::AddSibling(JsonObject* sibling)
{
    if (firstSibling == nullptr)
    {
        firstSibling = sibling;
    }
    else
    {
        JsonObject* lastSibling = firstSibling;
        while (lastSibling->firstSibling != nullptr)
        {
            lastSibling = lastSibling->firstSibling;
        }
        lastSibling->firstSibling = sibling;
    }
}

void JsonObject::SaveName(void* hFile)
{
    DWORD bytesWriten = 0;
    const char* openQuotes = "\"";
    WriteFile(hFile, openQuotes, strlen(openQuotes), &bytesWriten, 0);
    WriteFile(hFile, name, strlen(name), &bytesWriten, 0);
    const char* closeQuotes = "\":";
    WriteFile(hFile, closeQuotes, strlen(closeQuotes), &bytesWriten, 0);
}

void JsonObject::SaveValue(void* hFile)
{
    DWORD bytesWriten = 0;
    if (firstValue && (firstValue->next || firstValue->type == VALUE_OBJECT))
    {
        const char* openBrack = "[";
        WriteFile(hFile, openBrack, strlen(openBrack), &bytesWriten, 0);
    }

    JsonValue* value = firstValue;
    while (value != nullptr)
    {
        switch (value->type)
        {
            case VALUE_BOOL:
            {
                const char* trueString = "true";
                const char* falseString = "false";
                if (value->valueBool) 
                {
                    WriteFile(hFile, trueString, strlen(trueString), &bytesWriten, 0);
                }
                else
                {
                    WriteFile(hFile, falseString, strlen(falseString), &bytesWriten, 0);
                }
                
            } break;
            case VALUE_NULL:
            {
                const char* null = "null";
                WriteFile(hFile, null, strlen(null), &bytesWriten, 0);
            } break;
            case VALUE_CHARACTER:
            {
                const char* openQuotes = "\"";
                WriteFile(hFile, openQuotes, strlen(openQuotes), &bytesWriten, 0);

                WriteFile(hFile, value->valueChar, strlen(value->valueChar), &bytesWriten, 0);

                const char* closeQuotes = "\"";
                WriteFile(hFile, closeQuotes, strlen(closeQuotes), &bytesWriten, 0);
            } break;
            case VALUE_FLOAT:
            {
                static char buffer[100];
                sprintf_s(buffer, "%.*e\0", 16, value->valueFloat);
                WriteFile(hFile, buffer, strlen(buffer), &bytesWriten, 0);
            } break;
            case VALUE_OBJECT:
            {
                value->valueObject->SaveChild(hFile);

            } break;
        }
        if (value->next)
        {
            const char* comma = ",";
            WriteFile(hFile, comma, strlen(comma), &bytesWriten, 0);
        }
        value = value->next;
    }

    if (firstValue && (firstValue->next || firstValue->type == VALUE_OBJECT))
    {
        const char* closeBrack = "]";
        WriteFile(hFile, closeBrack, strlen(closeBrack), &bytesWriten, 0);
    }
}

void JsonObject::SaveSibling(void* hFile)
{
    DWORD bytesWriten = 0;

    const char* comma = ",";
    WriteFile(hFile, comma, strlen(comma), &bytesWriten, 0);

    if (name) SaveName(hFile);
    if (firstValue) SaveValue(hFile);
    if (firstChild) firstChild->SaveChild(hFile);
    if (firstSibling) firstSibling->SaveSibling(hFile);

}

void JsonObject::SaveChild(void* hFile)
{
    DWORD bytesWriten = 0;

    const char* openBrack = "{";
    WriteFile(hFile, openBrack, strlen(openBrack), &bytesWriten, 0);


    if (name) SaveName(hFile);
    if (firstValue) SaveValue(hFile);
    if (firstChild) firstChild->SaveChild(hFile);
    if (firstSibling) firstSibling->SaveSibling(hFile);

    const char* closeBrack = "}";
    WriteFile(hFile, closeBrack, strlen(closeBrack), &bytesWriten, 0);

}

void JsonObject::SaveRoot(void* hFile)
{
    if (name) SaveName(hFile);
    if (firstValue) SaveValue(hFile);
    if (firstChild) firstChild->SaveChild(hFile);
    if (firstSibling) firstSibling->SaveSibling(hFile);

}

bool JsonObject::SaveToFile(const char* filepath)
{
    HANDLE hFile = CreateFileA(filepath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (FAILED(hFile))
    {
        printf("Error: JsonParser::SaveToFile(%s) failed to open/create the file\n", filepath);
        return false;
    }

    SaveRoot(hFile);

    CloseHandle(hFile);

    return true;
}