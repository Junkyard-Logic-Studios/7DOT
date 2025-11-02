#pragma once
#include <inttypes.h>
#include <memory>
#include "SDL3/SDL_gamepad.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_scancode.h"



namespace input
{
    
    using PlayerInput = uint16_t;


    enum BitMask : uint16_t
    {
        DASH                  = 0b0000'0000'0000'0111,
        START                 = 0b0000'0000'0000'1000,
        SHOOT                 = 0b0000'0000'0001'0000,
        JUMP                  = 0b0000'0000'0010'0000,
        TOGGLE                = 0b0000'0000'0100'0000,
        CANCEL                = 0b0000'0000'1000'0000,
        HORIZONTAL_AXIS_SIGN  = 0b0000'1000'0000'0000,
        HORIZONTAL_AXIS_VALUE = 0b0000'0111'0000'0000,
        HORIZONTAL_AXIS       = HORIZONTAL_AXIS_SIGN | HORIZONTAL_AXIS_VALUE,
        VERTICAL_AXIS_SIGN    = 0b1000'0000'0000'0000,
        VERTICAL_AXIS_VALUE   = 0b0111'0000'0000'0000,
        VERTICAL_AXIS         = VERTICAL_AXIS_SIGN | VERTICAL_AXIS_VALUE
    };


    namespace set
    {

        inline void dashCount (PlayerInput& input, unsigned int value) 
            { input = (input & ~BitMask::DASH) | value & BitMask::DASH; }

        inline void start(PlayerInput& input, bool value)  
            { input = (input & ~BitMask::START) | (value * BitMask::START); }

        inline void shoot(PlayerInput& input, bool value)  
            { input = (input & ~BitMask::SHOOT) | (value * BitMask::SHOOT); }

        inline void jump(PlayerInput& input, bool value)   
            { input = (input & ~BitMask::JUMP) | (value * BitMask::JUMP); }

        inline void toggle(PlayerInput& input, bool value) 
            { input = (input & ~BitMask::TOGGLE) | (value * BitMask::TOGGLE); }

        inline void cancel(PlayerInput& input, bool value) 
            { input = (input & ~BitMask::CANCEL) | (value * BitMask::CANCEL); }
        
        inline void horizontalAxis(PlayerInput& input, Sint16 value)
        {
            uint16_t sign = value < 0;
            value = (value ^ -sign) >> 4;     // get one's complement representation
            input = (input & ~BitMask::HORIZONTAL_AXIS) 
                  | (value & BitMask::HORIZONTAL_AXIS_VALUE) 
                  | (sign * BitMask::HORIZONTAL_AXIS_SIGN);
        }

        inline void verticalAxis(PlayerInput& input, Sint16 value)
        {
            uint16_t sign = value < 0;
            value = (value ^ -sign);          // get one's complement representation
            input = (input & ~BitMask::VERTICAL_AXIS) 
                  | (value & BitMask::VERTICAL_AXIS_VALUE) 
                  | (sign * BitMask::VERTICAL_AXIS_SIGN);
        }

    };  // end namespace set


    namespace get
    {

        inline unsigned int dashCount (const PlayerInput input) 
            { return input & BitMask::DASH; }
        
        inline bool start(const PlayerInput input)  
            { return input & BitMask::START; }

        inline bool shoot(const PlayerInput input)  
            { return input & BitMask::SHOOT; }

        inline bool jump(const PlayerInput input)   
            { return input & BitMask::JUMP; }

        inline bool toggle(const PlayerInput input) 
            { return input & BitMask::TOGGLE; }

        inline bool cancel(const PlayerInput input) 
            { return input & BitMask::CANCEL; }
                
        inline float horizontalAxis(const PlayerInput input) 
        {
            int16_t sign = (input & BitMask::HORIZONTAL_AXIS_SIGN) >> 11;
            int16_t value = (input & BitMask::HORIZONTAL_AXIS_VALUE) >> 8;
            return float((value ^ -sign) + sign) / 7.0f;
        }

        inline float verticalAxis(const PlayerInput input)
        {
            int16_t sign = (input & BitMask::VERTICAL_AXIS_SIGN) >> 15;
            int16_t value = (input & BitMask::VERTICAL_AXIS_VALUE) >> 12;
            return float((value ^ -sign) + sign) / 7.0f;
        }

    };  // end namespace get


    class IDevice
    {
    public:
        inline virtual const char* getName() const = 0;
        inline virtual PlayerInput poll() const = 0;
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

        inline PlayerInput poll() const
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

        inline PlayerInput poll() const
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
