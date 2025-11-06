#pragma once
#include <memory>
#include "SDL3/SDL_gamepad.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_scancode.h"
#include "../constants.hpp"
#include "input.hpp"
#include "inputBuffer.hpp"



namespace input
{

    class IDevice
    {
    public:
        inline virtual const char* getName() const = 0;
        void poll();
        PlayerInput getInput(int64_t tick) const;

    protected:
        InputBuffer _inputBuffer;

        inline virtual PlayerInput _poll() const = 0;
    };


    class Gamepad : public IDevice
    {
    public:
        inline Gamepad(SDL_Gamepad* gamepad) :
            _id(SDL_GetGamepadID(gamepad)),
            _pGamepad(gamepad, SDL_CloseGamepad)
        {}

        inline SDL_JoystickID getID() const
            { return _id; }

        inline const char* getName() const
            { return SDL_GetGamepadName(_pGamepad.get()); }

    protected:
        inline PlayerInput _poll() const
        {            
            PlayerInput input = 0;
            
            // get action buttons
            set::start( input, SDL_GetGamepadButton(_pGamepad.get(), SDL_GamepadButton::SDL_GAMEPAD_BUTTON_START));
            set::shoot( input, SDL_GetGamepadButton(_pGamepad.get(), SDL_GamepadButton::SDL_GAMEPAD_BUTTON_WEST));
            set::jump(  input, SDL_GetGamepadButton(_pGamepad.get(), SDL_GamepadButton::SDL_GAMEPAD_BUTTON_SOUTH));
            set::toggle(input, SDL_GetGamepadButton(_pGamepad.get(), SDL_GamepadButton::SDL_GAMEPAD_BUTTON_NORTH));
            set::cancel(input, SDL_GetGamepadButton(_pGamepad.get(), SDL_GamepadButton::SDL_GAMEPAD_BUTTON_EAST));

            // count dash inputs
            uint16_t count = 0;
            count += SDL_GetGamepadButton(_pGamepad.get(), SDL_GamepadButton::SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
            count += SDL_GetGamepadButton(_pGamepad.get(), SDL_GamepadButton::SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
            count += (SDL_GetGamepadAxis(_pGamepad.get(), SDL_GamepadAxis::SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 1000);
            count += (SDL_GetGamepadAxis(_pGamepad.get(), SDL_GamepadAxis::SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 1000);
            set::dashCount(input, count);

            // get axis inputs
            set::horizontalAxis(input, SDL_GetGamepadAxis(_pGamepad.get(), SDL_GamepadAxis::SDL_GAMEPAD_AXIS_LEFTX));
            set::verticalAxis(input, SDL_GetGamepadAxis(_pGamepad.get(), SDL_GamepadAxis::SDL_GAMEPAD_AXIS_LEFTY));

            return input;
        }

    private:
        const SDL_JoystickID _id;
        const std::unique_ptr<SDL_Gamepad, decltype(&SDL_CloseGamepad)> _pGamepad;
    };


    class Keyboard : public IDevice
    {
    public:
        inline const char* getName() const
            { return "keyboard"; }

    protected:
        inline PlayerInput _poll() const
        {
            PlayerInput input = 0;

            const bool* keysPressed = SDL_GetKeyboardState(nullptr);

            // get actions
            set::start( input, keysPressed[SDL_Scancode::SDL_SCANCODE_2]);
            set::shoot( input, keysPressed[SDL_Scancode::SDL_SCANCODE_3]);
            set::jump(  input, keysPressed[SDL_Scancode::SDL_SCANCODE_SPACE]);
            set::toggle(input, keysPressed[SDL_Scancode::SDL_SCANCODE_4]);
            set::cancel(input, keysPressed[SDL_Scancode::SDL_SCANCODE_5]);

            // count dash inputs
            uint16_t count = 0;
            count += keysPressed[SDL_Scancode::SDL_SCANCODE_LSHIFT];
            count += keysPressed[SDL_Scancode::SDL_SCANCODE_6];
            set::dashCount(input, count);

            // get axis inputs
            set::horizontalAxis(input, (keysPressed[SDL_Scancode::SDL_SCANCODE_D] - keysPressed[SDL_Scancode::SDL_SCANCODE_A]) * 32767);
            set::verticalAxis(input, (keysPressed[SDL_Scancode::SDL_SCANCODE_S] - keysPressed[SDL_Scancode::SDL_SCANCODE_W]) * 32767);
        
            return input;
        }
    };

};  // end namespace input
