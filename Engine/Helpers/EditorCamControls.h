#pragma once

#include "../Utils.h"
#include "../Dependencies.h"

namespace Bplus::Helpers
{
    //Different ways the camera can behave in regards to preserving its upward vector.
    BETTER_ENUM(CameraUpModes, uint8_t,
        //Always keep the camera's Up vector at its current value.
        //Prevent rotations from reaching that value.
        KeepUpright,
        //Allow the camera to rotate freely, but snap back to a +Z up vector once rotation stops.
        ResetZUp,
        //Allow turns and rolls without limitations.
        Free
    );

    //Controls for a 3D editor camera.
    class BP_API EditorCamControls
    {
    public:

        EditorCamControls(glm::fvec3 pos = { 0, 0, 0 },
                          CameraUpModes upMode = CameraUpModes::KeepUpright,
                          glm::fvec3 forward = { 1, 0, 0 },
                          glm::fvec3 up = { 0, 0, 1 })
            : Position(pos), Forward(forward), Up(up), UpMode(upMode) { }


        glm::fvec3 Position;

        glm::fvec3 Forward, Up;

        glm::fvec3 GetRight() const { return glm::normalize(glm::cross(Forward, Up)); }
        glm::quat GetRotation() const { return glm::quatLookAt(Forward, Up); }
        glm::fmat4 GetViewMat() const { return glm::lookAt(Position, Position + Forward, Up); }


        //Movement speed per second:
        float MoveSpeed = 20;
        //The scale in movement speed when holding the "speed" button:
        float MoveSpeedBoostMultiplier = 3;
        //The scale in movement speed due to the "ChangeSpeed" input:
        float MoveSpeedScale = 1.25f,
              MoveSpeedScaledMin = 0.01f,
              MoveSpeedScaledMax = std::numeric_limits<float>::max();

        //Turn speed, in degrees per second:
        float TurnSpeedDegrees = 1;
        //Whether/how much the camera preserves its up axis.
        CameraUpModes UpMode = CameraUpModes::KeepUpright;


        //Camera rotation inputs only work when this input is on.
        bool EnableRotation = false;
        glm::fvec2 InputCamYawPitch = { 0, 0 };

        float InputMoveForward = 0,
              InputMoveRight = 0,
              InputMoveUp = 0;

        //When this input is on, the camera moves faster.
        bool InputSpeedBoost = false;
        //When this input is changed, the camera's speed will increase or decrease.
        //The value is reset to 0 as soon as it gets applied.
        float InputSpeedChange = 0;


        void Update(float deltaT);
    };
}