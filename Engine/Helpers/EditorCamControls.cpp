#include "EditorCamControls.h"

using namespace Bplus;
using namespace Bplus::Helpers;


void EditorCamControls::Update(float deltaT)
{
    //Handle motion.
    float speed = deltaT * MoveSpeed * (InputSpeedBoost ? MoveSpeedBoostMultiplier : 1);
    Position += speed * ((Forward * InputMoveForward) +
                         (Up * InputMoveUp) +
                         (GetRight() * InputMoveRight));

    //Handle rotation.
    if (EnableRotation)
    {
        speed = deltaT * TurnSpeedDegrees;
        Forward = glm::normalize(Forward * glm::angleAxis(speed * InputCamYawPitch.x, Up));

        auto forwardPrePitch = Forward;
        auto pitchRot = glm::angleAxis(speed * InputCamYawPitch.y, GetRight());

        Forward = glm::normalize(Forward * pitchRot);
        switch (UpMode)
        {
            case CameraUpModes::Free:
            case CameraUpModes::ResetZUp:
                Up = glm::normalize(Up * pitchRot);
            break;

            case CameraUpModes::KeepUpright:
                //Leave the Up alone, and prevent Forward from passing through it.
                auto oldRight = glm::cross(forwardPrePitch, Up);
                auto oldTrueForward = glm::cross(Up, oldRight);
                auto newTrueForward = glm::cross(Up, GetRight());
                if (glm::dot(oldTrueForward, newTrueForward) < 0)
                    Forward = oldTrueForward;
            break;

            default:
                std::string errorMsg = "Unhandled CameraUpModes::";
                errorMsg += UpMode._to_string();
                BPAssert(false, errorMsg.c_str());
            break;
        }
    }
    //If the user is no longer controlling the camera,
    //    we may want to snap the camera back upright.
    else if (UpMode == +CameraUpModes::ResetZUp)
    {
        Up = { 0, 0, 1 };
    }

    //Handle other inputs.
    if (InputSpeedChange != 0)
    {
        float scaleFactor = std::powf(MoveSpeedScale, InputSpeedChange);
        MoveSpeed = std::clamp(MoveSpeed * scaleFactor,
                               MoveSpeedScaledMin, MoveSpeedScaledMax);
    }
}