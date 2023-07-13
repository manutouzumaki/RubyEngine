#include "RubyInput.h"

namespace Ruby
{
    bool Input::KeyIsDown(UINT vkCode)
    {
        return mCurrent.keys[vkCode].isPress;
    }

    bool Input::KeyJustDown(UINT vkCode)
    {
        if (mCurrent.keys[vkCode].isPress != mCurrent.keys[vkCode].wasPress)
        {
            return mCurrent.keys[vkCode].isPress;
        }
        return false;
    }

    bool Input::KeyIsUp(UINT vkCode)
    {
        return !mCurrent.keys[vkCode].isPress;
    }

    bool Input::KeyJustUp(UINT vkCode)
    {
        if (mCurrent.keys[vkCode].isPress != mCurrent.keys[vkCode].wasPress)
        {
            return mCurrent.keys[vkCode].wasPress;
        }
        return false;
    }

    bool Input::MouseButtonIsDown(UINT button)
    {
        return mCurrent.mouseButtons[button].isPress;

    }

    bool Input::MouseButtonJustDown(UINT button)
    {
        if (mCurrent.mouseButtons[button].isPress != mCurrent.mouseButtons[button].wasPress)
        {
            return mCurrent.mouseButtons[button].isPress;
        }
        return false;
    }

    bool Input::MouseButtonIsUp(UINT button)
    {
        return !mCurrent.mouseButtons[button].isPress;
    }

    bool Input::MouseButtonJustUp(UINT button)
    {
        if (mCurrent.mouseButtons[button].isPress != mCurrent.mouseButtons[button].wasPress)
        {
            return mCurrent.mouseButtons[button].wasPress;
        }
        return false;
    }

    int Input::MousePosX()
    {
        return mCurrent.mouseX;
    }

    int Input::MousePosY()
    {
        return mCurrent.mouseY;
    }

    int Input::MouseLastPosX()
    {
        return mLast.mouseX;
    }

    int Input::MouseLastPosY()
    {
        return mLast.mouseY;
    }

}
