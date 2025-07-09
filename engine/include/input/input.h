#pragma once
#include <utility>

namespace MongooseVK {

    class Input
    {
    public:
        Input() = default;
        virtual ~Input() = default;

        static bool IsKeyPressed(int keycode) { return s_Instance->IsKeyPressedImpl(keycode); }
        static bool IsMousePressed(int button) { return s_Instance->IsMousePressedImpl(button); }

        static float GetMousePosX() { return s_Instance->GetMousePosXImpl(); }
        static float GetMousePosY() { return s_Instance->GetMousePosYImpl(); }
        static std::pair<float, float> GetMousePos() { return s_Instance->GetMousePosImpl(); }

    protected:
        virtual bool IsKeyPressedImpl(int keycode);
        virtual bool IsMousePressedImpl(int button);

        virtual float GetMousePosXImpl();
        virtual float GetMousePosYImpl();
        virtual std::pair<float, float> GetMousePosImpl();

    private:
        static Input* s_Instance;
    };

}
