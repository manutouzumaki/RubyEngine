#pragma once

#include <windows.h>

namespace Ruby
{
    struct Button
    {
        bool wasPress;
        bool isPress;

    };

    struct InputState
    {
        Button keys[349];
        Button mouseButtons[3];
        int mouseX;
        int mouseY;
    };

    class Input
    {
    public:

        InputState mCurrent;
        InputState mLast;

    public:

        Input() {}
        ~Input() {}

        bool KeyIsDown(UINT vkCode);
        bool KeyJustDown(UINT vkCode);
        bool KeyIsUp(UINT vkCode);
        bool KeyJustUp(UINT vkCode);

        bool MouseButtonIsDown(UINT button);
        bool MouseButtonJustDown(UINT button);
        bool MouseButtonIsUp(UINT button);
        bool MouseButtonJustUp(UINT button);

        int MousePosX();
        int MousePosY();

        int MouseLastPosX();
        int MouseLastPosY();

    };
}



