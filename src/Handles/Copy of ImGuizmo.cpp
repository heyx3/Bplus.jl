//Source: https://github.com/CedricGuillemet/ImGuizmo

#if 1 //Header


// https://github.com/CedricGuillemet/ImGuizmo
// v 1.89 WIP
//
// The MIT License(MIT)
//
// Copyright(c) 2021 Cedric Guillemet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -------------------------------------------------------------------------------------------
// History :
// 2019/11/03 View gizmo
// 2016/09/11 Behind camera culling. Scaling Delta matrix not multiplied by source matrix scales. local/world rotation and translation fixed. Display message is incorrect (X: ... Y:...) in local mode.
// 2016/09/09 Hatched negative axis. Snapping. Documentation update.
// 2016/09/04 Axis switch and translation plan autohiding. Scale transform stability improved
// 2016/09/01 Mogwai changed to Manipulate. Draw debug cube. Fixed inverted scale. Mixing scale and translation/rotation gives bad results.
// 2016/08/31 First version
//
// -------------------------------------------------------------------------------------------
// Future (no order):
//
// - Multi view
// - display rotation/translation/scale infos in local/world space and not only local
// - finish local/world matrix application
// - OPERATION as bitmask
// 
// -------------------------------------------------------------------------------------------
// Example 
#if 0
void EditTransform(const Camera& camera, matrix_t& matrix)
{
   static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
   static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
   if (ImGui::IsKeyPressed(90))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
   if (ImGui::IsKeyPressed(69))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
   if (ImGui::IsKeyPressed(82)) // r Key
      mCurrentGizmoOperation = ImGuizmo::SCALE;
   if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
   ImGui::SameLine();
   if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
   ImGui::SameLine();
   if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
      mCurrentGizmoOperation = ImGuizmo::SCALE;
   float matrixTranslation[3], matrixRotation[3], matrixScale[3];
   ImGuizmo::DecomposeMatrixToComponents(matrix.m16, matrixTranslation, matrixRotation, matrixScale);
   ImGui::InputFloat3("Tr", matrixTranslation, 3);
   ImGui::InputFloat3("Rt", matrixRotation, 3);
   ImGui::InputFloat3("Sc", matrixScale, 3);
   ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix.m16);

   if (mCurrentGizmoOperation != ImGuizmo::SCALE)
   {
      if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
         mCurrentGizmoMode = ImGuizmo::LOCAL;
      ImGui::SameLine();
      if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
         mCurrentGizmoMode = ImGuizmo::WORLD;
   }
   static bool useSnap(false);
   if (ImGui::IsKeyPressed(83))
      useSnap = !useSnap;
   ImGui::Checkbox("", &useSnap);
   ImGui::SameLine();
   vec_t snap;
   switch (mCurrentGizmoOperation)
   {
   case ImGuizmo::TRANSLATE:
      snap = config.mSnapTranslation;
      ImGui::InputFloat3("Snap", &snap.x);
      break;
   case ImGuizmo::ROTATE:
      snap = config.mSnapRotation;
      ImGui::InputFloat("Angle Snap", &snap.x);
      break;
   case ImGuizmo::SCALE:
      snap = config.mSnapScale;
      ImGui::InputFloat("Scale Snap", &snap.x);
      break;
   }
   ImGuiIO& io = ImGui::GetIO();
   ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
   ImGuizmo::Manipulate(camera.mView.m16, camera.mProjection.m16, mCurrentGizmoOperation, mCurrentGizmoMode, matrix.m16, NULL, useSnap ? &snap.x : NULL);
}
#endif
#pragma once

#ifdef USE_IMGUI_API
#include "imconfig.h"
#endif
#ifndef IMGUI_API
#define IMGUI_API
#endif

#ifndef IMGUIZMO_NAMESPACE
#define IMGUIZMO_NAMESPACE ImGuizmo
#endif

namespace IMGUIZMO_NAMESPACE
{
   // call inside your own window and before Manipulate() in order to draw gizmo to that window.
   // Or pass a specific ImDrawList to draw to (e.g. ImGui::GetForegroundDrawList()).
   IMGUI_API void SetDrawlist(ImDrawList* drawlist = nullptr);

   // call BeginFrame right after ImGui_XXXX_NewFrame();
   IMGUI_API void BeginFrame();

   // this is necessary because when imguizmo is compiled into a dll, and imgui into another
   // globals are not shared between them.
   // More details at https://stackoverflow.com/questions/19373061/what-happens-to-global-and-static-variables-in-a-shared-library-when-it-is-dynam
   // expose method to set imgui context
   IMGUI_API void SetImGuiContext(ImGuiContext* ctx);

   // return true if mouse cursor is over any gizmo control (axis, plan or screen component)
   IMGUI_API bool IsOver();

   // return true if mouse IsOver or if the gizmo is in moving state
   IMGUI_API bool IsUsing();

   // return true if any gizmo is in moving state
   IMGUI_API bool IsUsingAny();

   // enable/disable the gizmo. Stay in the state until next call to Enable.
   // gizmo is rendered with gray half transparent color when disabled
   IMGUI_API void Enable(bool enable);

   // helper functions for manualy editing translation/rotation/scale with an input float
   // translation, rotation and scale float points to 3 floats each
   // Angles are in degrees (more suitable for human editing)
   // example:
   // float matrixTranslation[3], matrixRotation[3], matrixScale[3];
   // ImGuizmo::DecomposeMatrixToComponents(gizmoMatrix.m16, matrixTranslation, matrixRotation, matrixScale);
   // ImGui::InputFloat3("Tr", matrixTranslation, 3);
   // ImGui::InputFloat3("Rt", matrixRotation, 3);
   // ImGui::InputFloat3("Sc", matrixScale, 3);
   // ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, gizmoMatrix.m16);
   //
   // These functions have some numerical stability issues for now. Use with caution.
   IMGUI_API void DecomposeMatrixToComponents(const float* matrix, float* translation, float* rotation, float* scale);
   IMGUI_API void RecomposeMatrixFromComponents(const float* translation, const float* rotation, const float* scale, float* matrix);

   IMGUI_API void SetRect(float x, float y, float width, float height);
   // default is false
   IMGUI_API void SetOrthographic(bool isOrthographic);

   // Render a cube with face color corresponding to face normal. Usefull for debug/tests
   IMGUI_API void DrawCubes(const float* view, const float* projection, const float* matrices, int matrixCount);
   IMGUI_API void DrawGrid(const float* view, const float* projection, const float* matrix, const float gridSize);

   // call it when you want a gizmo
   // Needs view and projection matrices. 
   // matrix parameter is the source matrix (where will be gizmo be drawn) and might be transformed by the function. Return deltaMatrix is optional
   // translation is applied in world space
   enum OPERATION
   {
      TRANSLATE_X      = (1u << 0),
      TRANSLATE_Y      = (1u << 1),
      TRANSLATE_Z      = (1u << 2),
      ROTATE_X         = (1u << 3),
      ROTATE_Y         = (1u << 4),
      ROTATE_Z         = (1u << 5),
      ROTATE_SCREEN    = (1u << 6),
      SCALE_X          = (1u << 7),
      SCALE_Y          = (1u << 8),
      SCALE_Z          = (1u << 9),
      BOUNDS           = (1u << 10),
      SCALE_XU         = (1u << 11),
      SCALE_YU         = (1u << 12),
      SCALE_ZU         = (1u << 13),

      TRANSLATE = TRANSLATE_X | TRANSLATE_Y | TRANSLATE_Z,
      ROTATE = ROTATE_X | ROTATE_Y | ROTATE_Z | ROTATE_SCREEN,
      SCALE = SCALE_X | SCALE_Y | SCALE_Z,
      SCALEU = SCALE_XU | SCALE_YU | SCALE_ZU, // universal
      UNIVERSAL = TRANSLATE | ROTATE | SCALEU
   };

   enum MODE
   {
      LOCAL,
      WORLD
   };

   IMGUI_API bool Manipulate(const float* view, const float* projection, OPERATION operation, MODE mode, float* matrix, float* deltaMatrix = NULL, const float* snap = NULL, const float* localBounds = NULL, const float* boundsSnap = NULL);
   //
   // Please note that this cubeview is patented by Autodesk : https://patents.google.com/patent/US7782319B2/en
   // It seems to be a defensive patent in the US. I don't think it will bring troubles using it as
   // other software are using the same mechanics. But just in case, you are now warned!
   //
   IMGUI_API void ViewManipulate(float* view, float length, ImVec2 position, ImVec2 size, ImU32 backgroundColor);

   // use this version if you did not call Manipulate before and you are just using ViewManipulate
   IMGUI_API void ViewManipulate(float* view, const float* projection, OPERATION operation, MODE mode, float* matrix, float length, ImVec2 position, ImVec2 size, ImU32 backgroundColor);

   IMGUI_API void SetID(int id);

   // return true if the cursor is over the operation's gizmo
   IMGUI_API bool IsOver(OPERATION op);
   IMGUI_API void SetGizmoSizeClipSpace(float value);

   // Allow axis to flip
   // When true (default), the guizmo axis flip for better visibility
   // When false, they always stay along the positive world/local axis
   IMGUI_API void AllowAxisFlip(bool value);

   // Configure the limit where axis are hidden
   IMGUI_API void SetAxisLimit(float value);
   // Configure the limit where planes are hiden
   IMGUI_API void SetPlaneLimit(float value);

   enum COLOR
   {
      DIRECTION_X,      // directionColor[0]
      DIRECTION_Y,      // directionColor[1]
      DIRECTION_Z,      // directionColor[2]
      PLANE_X,          // planeColor[0]
      PLANE_Y,          // planeColor[1]
      PLANE_Z,          // planeColor[2]
      SELECTION,        // selectionColor
      INACTIVE,         // inactiveColor
      TRANSLATION_LINE, // translationLineColor
      SCALE_LINE,
      ROTATION_USING_BORDER,
      ROTATION_USING_FILL,
      HATCHED_AXIS_LINES,
      TEXT,
      TEXT_SHADOW,
      COUNT
   };

   struct Style
   {
      IMGUI_API Style();

      float TranslationLineThickness;   // Thickness of lines for translation gizmo
      float TranslationLineArrowSize;   // Size of arrow at the end of lines for translation gizmo
      float RotationLineThickness;      // Thickness of lines for rotation gizmo
      float RotationOuterLineThickness; // Thickness of line surrounding the rotation gizmo
      float ScaleLineThickness;         // Thickness of lines for scale gizmo
      float ScaleLineCircleSize;        // Size of circle at the end of lines for scale gizmo
      float HatchedAxisLineThickness;   // Thickness of hatched axis lines
      float CenterCircleSize;           // Size of circle at the center of the translate/scale gizmo

      ImVec4 Colors[COLOR::COUNT];
   };

   IMGUI_API Style& GetStyle();
}


#endif //Header

#if 1 //Source


// https://github.com/CedricGuillemet/ImGuizmo
// v 1.89 WIP
//
// The MIT License(MIT)
//
// Copyright(c) 2021 Cedric Guillemet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h>
#endif
#if !defined(_MSC_VER) && !defined(__MINGW64_VERSION_MAJOR)
#define _malloca(x) alloca(x)
#define _freea(x)
#endif

// includes patches for multiview from
// https://github.com/CedricGuillemet/ImGuizmo/issues/15

namespace IMGUIZMO_NAMESPACE
{
   static const float RAD_TO_DEG = (180.f / PI);
   static const float DEG_TO_RAD = (PI / 180.f);
   const float screenRotateSize = 0.06f;
   // scale a bit so translate axis do not touch when in universal
   const float rotationDisplayFactor = 1.2f;

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // utility and math

   //Multiply `a*b`, store the result in `r`
   void MatrixMultiply(const float* a, const float* b, float* r);

   //Projection matrix with arbitrary center in view-space 
   void Perspective(float left, float right, float bottom, float top, float znear, float zfar, float* m16)
   {
      float temp, temp2, temp3, temp4;
      temp = 2.0f * znear;
      temp2 = right - left;
      temp3 = top - bottom;
      temp4 = zfar - znear;
      m16[0] = temp / temp2;
      m16[1] = 0.0;
      m16[2] = 0.0;
      m16[3] = 0.0;
      m16[4] = 0.0;
      m16[5] = temp / temp3;
      m16[6] = 0.0;
      m16[7] = 0.0;
      m16[8] = (right + left) / temp2;
      m16[9] = (top + bottom) / temp3;
      m16[10] = (-zfar - znear) / temp4;
      m16[11] = -1.0f;
      m16[12] = 0.0;
      m16[13] = 0.0;
      m16[14] = (-temp * zfar) / temp4;
      m16[15] = 0.0;
   }

   //Projection matrix
   void Perspective(float fovyInDegrees, float aspectRatio, float znear, float zfar, float* m16)
   {
      float ymax, xmax;
      ymax = znear * tanf(fovyInDegrees * DEG_TO_RAD);
      xmax = ymax * aspectRatio;
      Perspective(-xmax, xmax, -ymax, ymax, znear, zfar, m16);
   }

   void LookAt(const float* eye, const float* at, const float* up, float* m16);

   template <typename T> bool IsWithin(T x, T a, T b) { return (x >= a) && (x <= b); }

    using vec_t = v4f;
    using matrix_t = fmat4;

   //I think this packs info about a handle's editing plane and position into a v4f.
   vec_t BuildPlan(const vec_t& p_point1, const vec_t& p_normal)
   {
      vec_t normal, res;
      normal.Normalize(p_normal);
      res.w = normal.Dot(p_point1);
      res.x = normal.x;
      res.y = normal.y;
      res.z = normal.z;
      return res;
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //

   enum MOVETYPE
   {
      MT_NONE,
      MT_MOVE_X,
      MT_MOVE_Y,
      MT_MOVE_Z,
      MT_MOVE_YZ,
      MT_MOVE_ZX,
      MT_MOVE_XY,
      MT_MOVE_SCREEN,
      MT_ROTATE_X,
      MT_ROTATE_Y,
      MT_ROTATE_Z,
      MT_ROTATE_SCREEN,
      MT_SCALE_X,
      MT_SCALE_Y,
      MT_SCALE_Z,
      MT_SCALE_XYZ
   };

   static bool IsTranslateType(int type)
   {
     return type >= MT_MOVE_X && type <= MT_MOVE_SCREEN;
   }

   static bool IsRotateType(int type)
   {
     return type >= MT_ROTATE_X && type <= MT_ROTATE_SCREEN;
   }

   static bool IsScaleType(int type)
   {
     return type >= MT_SCALE_X && type <= MT_SCALE_XYZ;
   }

   // Matches MT_MOVE_AB order
   static const OPERATION TRANSLATE_PLANS[3] = { TRANSLATE_Y | TRANSLATE_Z, TRANSLATE_X | TRANSLATE_Z, TRANSLATE_X | TRANSLATE_Y };

   Style::Style()
   {
      // default values
      TranslationLineThickness   = 3.0f;
      TranslationLineArrowSize   = 6.0f;
      RotationLineThickness      = 2.0f;
      RotationOuterLineThickness = 3.0f;
      ScaleLineThickness         = 3.0f;
      ScaleLineCircleSize        = 6.0f;
      HatchedAxisLineThickness   = 6.0f;
      CenterCircleSize           = 6.0f;

      // initialize default colors
      Colors[DIRECTION_X]           = ImVec4(0.666f, 0.000f, 0.000f, 1.000f);
      Colors[DIRECTION_Y]           = ImVec4(0.000f, 0.666f, 0.000f, 1.000f);
      Colors[DIRECTION_Z]           = ImVec4(0.000f, 0.000f, 0.666f, 1.000f);
      Colors[PLANE_X]               = ImVec4(0.666f, 0.000f, 0.000f, 0.380f);
      Colors[PLANE_Y]               = ImVec4(0.000f, 0.666f, 0.000f, 0.380f);
      Colors[PLANE_Z]               = ImVec4(0.000f, 0.000f, 0.666f, 0.380f);
      Colors[SELECTION]             = ImVec4(1.000f, 0.500f, 0.062f, 0.541f);
      Colors[INACTIVE]              = ImVec4(0.600f, 0.600f, 0.600f, 0.600f);
      Colors[TRANSLATION_LINE]      = ImVec4(0.666f, 0.666f, 0.666f, 0.666f);
      Colors[SCALE_LINE]            = ImVec4(0.250f, 0.250f, 0.250f, 1.000f);
      Colors[ROTATION_USING_BORDER] = ImVec4(1.000f, 0.500f, 0.062f, 1.000f);
      Colors[ROTATION_USING_FILL]   = ImVec4(1.000f, 0.500f, 0.062f, 0.500f);
      Colors[HATCHED_AXIS_LINES]    = ImVec4(0.000f, 0.000f, 0.000f, 0.500f);
      Colors[TEXT]                  = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
      Colors[TEXT_SHADOW]           = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
   }

   struct Context
   {
      Context() : mbUsing(false), mbEnable(true), mbUsingBounds(false)
      {
      }

      ImDrawList* mDrawList;
      Style mStyle;

      MODE mHandleSpace;
      matrix_t mViewMat;
      matrix_t mProjectionMat;
      matrix_t mModel;
      matrix_t mModelLocal; // orthonormalized model
      matrix_t mModelInverse;
      matrix_t mModelSource;
      matrix_t mModelSourceInverse;
      matrix_t mMVP;
      matrix_t mMVPLocal; // MVP with full model matrix whereas mMVP's model matrix might only be translation in case of World space edition
      matrix_t mViewProjection;

      vec_t mModelScaleOrigin;
      vec_t mCameraPos;
      vec_t mCameraRight;
      vec_t mCameraForward;
      vec_t mCameraUp;
      vec_t mRayOrigin;
      vec_t mRayVector;

      float  mRadiusSquareCenter;
      ImVec2 mScreenSquareCenter;
      ImVec2 mScreenSquareMin;
      ImVec2 mScreenSquareMax;

      float mScreenFactor;
      vec_t mRelativeOrigin;

      bool mbUsing;
      bool mbEnable;
      bool mbMouseOver;
      bool mIsProjMatReversed; // reversed projection matrix

      // translation
      vec_t mTranslationPlan;
      vec_t mTranslationPlanOrigin;
      vec_t mMatrixOrigin;
      vec_t mTranslationLastDelta;

      // rotation
      vec_t mRotationVectorSource;
      float mRotationAngle;
      float mRotationAngleOrigin;
      //vec_t mWorldToLocalAxis;

      // scale
      vec_t mScale;
      vec_t mScaleValueOrigin;
      vec_t mScaleLast;
      float mSavedMousePosX;

      // save axis factor when using gizmo
      bool mBelowAxisLimit[3];
      bool mBelowPlaneLimit[3];
      float mAxisFactor[3];

      float mAxisLimit=0.0025f;
      float mPlaneLimit=0.02f;

      // bounds stretching
      vec_t mBoundsPivot;
      vec_t mBoundsAnchor;
      vec_t mBoundsPlan;
      vec_t mBoundsLocalPivot;
      int mBoundsBestAxis;
      int mBoundsAxis[2];
      bool mbUsingBounds;
      matrix_t mBoundsMatrix;

      //
      int mCurrentOperation;

      float mX = 0.f;
      float mY = 0.f;
      float mWidth = 0.f;
      float mHeight = 0.f;
      float mXMax = 0.f;
      float mYMax = 0.f;
      float mDisplayRatio = 1.f;

      bool mIsOrthographic = false;

      int mActualID = -1;
      int mEditingID = -1;
      OPERATION mOperation = OPERATION(-1);

      bool mAllowAxisFlip = true;
      float mGizmoSizeClipSpace = 0.1f;
   };

   static Context gContext;

   static const vec_t directionUnary[3] = { makeVect(1.f, 0.f, 0.f), makeVect(0.f, 1.f, 0.f), makeVect(0.f, 0.f, 1.f) };
   static const char* translationInfoMask[] = { "X : %5.3f", "Y : %5.3f", "Z : %5.3f",
      "Y : %5.3f Z : %5.3f", "X : %5.3f Z : %5.3f", "X : %5.3f Y : %5.3f",
      "X : %5.3f Y : %5.3f Z : %5.3f" };
   static const char* scaleInfoMask[] = { "X : %5.2f", "Y : %5.2f", "Z : %5.2f", "XYZ : %5.2f" };
   static const char* rotationInfoMask[] = { "X : %5.2f deg %5.2f rad", "Y : %5.2f deg %5.2f rad", "Z : %5.2f deg %5.2f rad", "Screen : %5.2f deg %5.2f rad" };
   static const int translationInfoIndex[] = { 0,0,0, 1,0,0, 2,0,0, 1,2,0, 0,2,0, 0,1,0, 0,1,2 };
   static const float quadMin = 0.5f;
   static const float quadMax = 0.8f;
   static const float quadUV[8] = { quadMin, quadMin, quadMin, quadMax, quadMax, quadMax, quadMax, quadMin };
   static const int halfCircleSegmentCount = 64;
   static const float snapTension = 0.5f;

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //
   static int GetMoveType(OPERATION op, vec_t* gizmoHitProportion);
   static int GetRotateType(OPERATION op);
   static int GetScaleType(OPERATION op);

   Style& GetStyle()
   {
      return gContext.mStyle;
   }

   static ImU32 GetColorU32(int idx)
   {
      IM_ASSERT(idx < COLOR::COUNT);
      return ImGui::ColorConvertFloat4ToU32(gContext.mStyle.Colors[idx]);
   }

   static ImVec2 worldToPos(const vec_t& worldPos, const matrix_t& mat, ImVec2 position = ImVec2(gContext.mX, gContext.mY), ImVec2 size = ImVec2(gContext.mWidth, gContext.mHeight))
   {
      vec_t trans;
      trans.TransformPoint(worldPos, mat);
      trans *= 0.5f / trans.w;
      trans += makeVect(0.5f, 0.5f);
      trans.y = 1.f - trans.y;
      trans.x *= size.x;
      trans.y *= size.y;
      trans.x += position.x;
      trans.y += position.y;
      return ImVec2(trans.x, trans.y);
   }

   static void ComputeCameraRay(vec_t& rayOrigin, vec_t& rayDir, ImVec2 position = ImVec2(gContext.mX, gContext.mY), ImVec2 size = ImVec2(gContext.mWidth, gContext.mHeight))
   {
      ImGuiIO& io = ImGui::GetIO();

      matrix_t mViewProjInverse;
      mViewProjInverse.Inverse(gContext.mViewMat * gContext.mProjectionMat);

      const float mox = ((io.MousePos.x - position.x) / size.x) * 2.f - 1.f;
      const float moy = (1.f - ((io.MousePos.y - position.y) / size.y)) * 2.f - 1.f;

      const float zNear = gContext.mIsProjMatReversed ? (1.f - FLT_EPSILON) : 0.f;
      const float zFar = gContext.mIsProjMatReversed ? 0.f : (1.f - FLT_EPSILON);

      rayOrigin.Transform(makeVect(mox, moy, zNear, 1.f), mViewProjInverse);
      rayOrigin *= 1.f / rayOrigin.w;
      vec_t rayEnd;
      rayEnd.Transform(makeVect(mox, moy, zFar, 1.f), mViewProjInverse);
      rayEnd *= 1.f / rayEnd.w;
      rayDir = Normalized(rayEnd - rayOrigin);
   }

   static float GetSegmentLengthClipSpace(const vec_t& start, const vec_t& end, const bool localCoordinates = false)
   {
      vec_t startOfSegment = start;
      const matrix_t& mvp = localCoordinates ? gContext.mMVPLocal : gContext.mMVP;
      startOfSegment.TransformPoint(mvp);
      if (fabsf(startOfSegment.w) > FLT_EPSILON) // check for axis aligned with camera direction
      {
         startOfSegment *= 1.f / startOfSegment.w;
      }

      vec_t endOfSegment = end;
      endOfSegment.TransformPoint(mvp);
      if (fabsf(endOfSegment.w) > FLT_EPSILON) // check for axis aligned with camera direction
      {
         endOfSegment *= 1.f / endOfSegment.w;
      }

      vec_t clipSpaceAxis = endOfSegment - startOfSegment;
      if (gContext.mDisplayRatio < 1.0)
         clipSpaceAxis.x *= gContext.mDisplayRatio;
      else
         clipSpaceAxis.y /= gContext.mDisplayRatio;
      float segmentLengthInClipSpace = sqrtf(clipSpaceAxis.x * clipSpaceAxis.x + clipSpaceAxis.y * clipSpaceAxis.y);
      return segmentLengthInClipSpace;
   }

   static float GetParallelogram(const vec_t& ptO, const vec_t& ptA, const vec_t& ptB)
   {
      vec_t pts[] = { ptO, ptA, ptB };
      for (unsigned int i = 0; i < 3; i++)
      {
         pts[i].TransformPoint(gContext.mMVP);
         if (fabsf(pts[i].w) > FLT_EPSILON) // check for axis aligned with camera direction
         {
            pts[i] *= 1.f / pts[i].w;
         }
      }
      vec_t segA = pts[1] - pts[0];
      vec_t segB = pts[2] - pts[0];
      segA.y /= gContext.mDisplayRatio;
      segB.y /= gContext.mDisplayRatio;
      vec_t segAOrtho = makeVect(-segA.y, segA.x);
      segAOrtho.Normalize();
      float dt = segAOrtho.Dot3(segB);
      float surface = sqrtf(segA.x * segA.x + segA.y * segA.y) * fabsf(dt);
      return surface;
   }

   inline vec_t PointOnSegment(const vec_t& point, const vec_t& vertPos1, const vec_t& vertPos2)
   {
      vec_t c = point - vertPos1;
      vec_t V;

      V.Normalize(vertPos2 - vertPos1);
      float d = (vertPos2 - vertPos1).Length();
      float t = V.Dot3(c);

      if (t < 0.f)
      {
         return vertPos1;
      }

      if (t > d)
      {
         return vertPos2;
      }

      return vertPos1 + V * t;
   }

   static float IntersectRayPlane(const vec_t& rOrigin, const vec_t& rVector, const vec_t& plan)
   {
      const float numer = plan.Dot3(rOrigin) - plan.w;
      const float denom = plan.Dot3(rVector);

      if (fabsf(denom) < FLT_EPSILON)  // normal is orthogonal to vector, cant intersect
      {
         return -1.0f;
      }

      return -(numer / denom);
   }

   static float DistanceToPlane(const vec_t& point, const vec_t& plan)
   {
      return plan.Dot3(point) + plan.w;
   }

   static bool IsInContextRect(ImVec2 p)
   {
      return IsWithin(p.x, gContext.mX, gContext.mXMax) && IsWithin(p.y, gContext.mY, gContext.mYMax);
   }

   static bool IsHoveringWindow()
   {
      ImGuiContext& g = *ImGui::GetCurrentContext();
      ImGuiWindow* window = ImGui::FindWindowByName(gContext.mDrawList->_OwnerName);
      if (g.HoveredWindow == window)   // Mouse hovering drawlist window
         return true;
      if (g.HoveredWindow != NULL)     // Any other window is hovered
         return false;
      if (ImGui::IsMouseHoveringRect(window->InnerRect.Min, window->InnerRect.Max, false))   // Hovering drawlist window rect, while no other window is hovered (for _NoInputs windows)
         return true;
      return false;
   }

   void SetRect(float x, float y, float width, float height)
   {
      gContext.mX = x;
      gContext.mY = y;
      gContext.mWidth = width;
      gContext.mHeight = height;
      gContext.mXMax = gContext.mX + gContext.mWidth;
      gContext.mYMax = gContext.mY + gContext.mXMax;
      gContext.mDisplayRatio = width / height;
   }

   void SetOrthographic(bool isOrthographic)
   {
      gContext.mIsOrthographic = isOrthographic;
   }

   void SetDrawlist(ImDrawList* drawlist)
   {
      gContext.mDrawList = drawlist ? drawlist : ImGui::GetWindowDrawList();
   }

   void SetImGuiContext(ImGuiContext* ctx)
   {
      ImGui::SetCurrentContext(ctx);
   }

   void BeginFrame()
   {
      const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;

#ifdef IMGUI_HAS_VIEWPORT
      ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
      ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
#else
      ImGuiIO& io = ImGui::GetIO();
      ImGui::SetNextWindowSize(io.DisplaySize);
      ImGui::SetNextWindowPos(ImVec2(0, 0));
#endif

      ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
      ImGui::PushStyleColor(ImGuiCol_Border, 0);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

      ImGui::Begin("gizmo", NULL, flags);
      gContext.mDrawList = ImGui::GetWindowDrawList();
      ImGui::End();
      ImGui::PopStyleVar();
      ImGui::PopStyleColor(2);
   }

   bool IsUsing()
   {
      return (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID)) || gContext.mbUsingBounds;
   }
   
   bool IsUsingAny()
   {
      return gContext.mbUsing || gContext.mbUsingBounds;
   }

   bool IsOver()
   {
      return (Intersects(gContext.mOperation, TRANSLATE) && GetMoveType(gContext.mOperation, NULL) != MT_NONE) ||
         (Intersects(gContext.mOperation, ROTATE) && GetRotateType(gContext.mOperation) != MT_NONE) ||
         (Intersects(gContext.mOperation, SCALE) && GetScaleType(gContext.mOperation) != MT_NONE) || IsUsing();
   }

   bool IsOver(OPERATION op)
   {
      if(IsUsing())
      {
         return true;
      }
      if(Intersects(op, SCALE) && GetScaleType(op) != MT_NONE)
      {
         return true;
      }
      if(Intersects(op, ROTATE) && GetRotateType(op) != MT_NONE)
      {
         return true;
      }
      if(Intersects(op, TRANSLATE) && GetMoveType(op, NULL) != MT_NONE)
      {
         return true;
      }
      return false;
   }

   void Enable(bool enable)
   {
      gContext.mbEnable = enable;
      if (!enable)
      {
         gContext.mbUsing = false;
         gContext.mbUsingBounds = false;
      }
   }

   static void ComputeContext(const float* view, const float* projection, float* matrix, MODE mode)
   {
      gContext.mHandleSpace = mode;
      gContext.mViewMat = *(matrix_t*)view;
      gContext.mProjectionMat = *(matrix_t*)projection;
      gContext.mbMouseOver = IsHoveringWindow();

      gContext.mModelLocal = *(matrix_t*)matrix;
      gContext.mModelLocal.OrthoNormalize();

      if (mode == LOCAL)
      {
         gContext.mModel = gContext.mModelLocal;
      }
      else
      {
         gContext.mModel.Translation(((matrix_t*)matrix)->v.position);
      }
      gContext.mModelSource = *(matrix_t*)matrix;
      gContext.mModelScaleOrigin.Set(gContext.mModelSource.v.right.Length(), gContext.mModelSource.v.up.Length(), gContext.mModelSource.v.dir.Length());

      gContext.mModelInverse.Inverse(gContext.mModel);
      gContext.mModelSourceInverse.Inverse(gContext.mModelSource);
      gContext.mViewProjection = gContext.mViewMat * gContext.mProjectionMat;
      gContext.mMVP = gContext.mModel * gContext.mViewProjection;
      gContext.mMVPLocal = gContext.mModelLocal * gContext.mViewProjection;

      matrix_t viewInverse;
      viewInverse.Inverse(gContext.mViewMat);
      gContext.mCameraForward = viewInverse.v.dir;
      gContext.mCameraPos = viewInverse.v.position;
      gContext.mCameraRight = viewInverse.v.right;
      gContext.mCameraUp = viewInverse.v.up;

      // projection reverse
       vec_t nearPos, farPos;
       nearPos.Transform(makeVect(0, 0, 1.f, 1.f), gContext.mProjectionMat);
       farPos.Transform(makeVect(0, 0, 2.f, 1.f), gContext.mProjectionMat);

       gContext.mIsProjMatReversed = (nearPos.z/nearPos.w) > (farPos.z / farPos.w);

      // compute scale from the size of camera right vector projected on screen at the matrix position
      vec_t pointRight = viewInverse.v.right;
      pointRight.TransformPoint(gContext.mViewProjection);
      gContext.mScreenFactor = gContext.mGizmoSizeClipSpace / (pointRight.x / pointRight.w - gContext.mMVP.v.position.x / gContext.mMVP.v.position.w);

      vec_t rightViewInverse = viewInverse.v.right;
      rightViewInverse.TransformVector(gContext.mModelInverse);
      float rightLength = GetSegmentLengthClipSpace(makeVect(0.f, 0.f), rightViewInverse);
      gContext.mScreenFactor = gContext.mGizmoSizeClipSpace / rightLength;

      ImVec2 centerSSpace = worldToPos(makeVect(0.f, 0.f), gContext.mMVP);
      gContext.mScreenSquareCenter = centerSSpace;
      gContext.mScreenSquareMin = ImVec2(centerSSpace.x - 10.f, centerSSpace.y - 10.f);
      gContext.mScreenSquareMax = ImVec2(centerSSpace.x + 10.f, centerSSpace.y + 10.f);

      ComputeCameraRay(gContext.mRayOrigin, gContext.mRayVector);
   }

   static void ComputeColors(ImU32* colors, int type, OPERATION operation)
   {
      if (gContext.mbEnable)
      {
         ImU32 selectionColor = GetColorU32(SELECTION);

         switch (operation)
         {
         case TRANSLATE:
            colors[0] = (type == MT_MOVE_SCREEN) ? selectionColor : IM_COL32_WHITE;
            for (int i = 0; i < 3; i++)
            {
               colors[i + 1] = (type == (int)(MT_MOVE_X + i)) ? selectionColor : GetColorU32(DIRECTION_X + i);
               colors[i + 4] = (type == (int)(MT_MOVE_YZ + i)) ? selectionColor : GetColorU32(PLANE_X + i);
               colors[i + 4] = (type == MT_MOVE_SCREEN) ? selectionColor : colors[i + 4];
            }
            break;
         case ROTATE:
            colors[0] = (type == MT_ROTATE_SCREEN) ? selectionColor : IM_COL32_WHITE;
            for (int i = 0; i < 3; i++)
            {
               colors[i + 1] = (type == (int)(MT_ROTATE_X + i)) ? selectionColor : GetColorU32(DIRECTION_X + i);
            }
            break;
         case SCALEU:
         case SCALE:
            colors[0] = (type == MT_SCALE_XYZ) ? selectionColor : IM_COL32_WHITE;
            for (int i = 0; i < 3; i++)
            {
               colors[i + 1] = (type == (int)(MT_SCALE_X + i)) ? selectionColor : GetColorU32(DIRECTION_X + i);
            }
            break;
         // note: this internal function is only called with three possible values for operation
         default:
            break;
         }
      }
      else
      {
         ImU32 inactiveColor = GetColorU32(INACTIVE);
         for (int i = 0; i < 7; i++)
         {
            colors[i] = inactiveColor;
         }
      }
   }

   static void ComputeTripodAxisAndVisibility(const int axisIndex, vec_t& dirAxis, vec_t& dirPlaneX, vec_t& dirPlaneY, bool& belowAxisLimit, bool& belowPlaneLimit, const bool localCoordinates = false)
   {
      dirAxis = directionUnary[axisIndex];
      dirPlaneX = directionUnary[(axisIndex + 1) % 3];
      dirPlaneY = directionUnary[(axisIndex + 2) % 3];

      if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID))
      {
         // when using, use stored factors so the gizmo doesn't flip when we translate
         belowAxisLimit = gContext.mBelowAxisLimit[axisIndex];
         belowPlaneLimit = gContext.mBelowPlaneLimit[axisIndex];

         dirAxis *= gContext.mAxisFactor[axisIndex];
         dirPlaneX *= gContext.mAxisFactor[(axisIndex + 1) % 3];
         dirPlaneY *= gContext.mAxisFactor[(axisIndex + 2) % 3];
      }
      else
      {
         // new method
         float lenDir = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirAxis, localCoordinates);
         float lenDirMinus = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirAxis, localCoordinates);

         float lenDirPlaneX = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirPlaneX, localCoordinates);
         float lenDirMinusPlaneX = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirPlaneX, localCoordinates);

         float lenDirPlaneY = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirPlaneY, localCoordinates);
         float lenDirMinusPlaneY = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirPlaneY, localCoordinates);

         // For readability
         bool & allowFlip = gContext.mAllowAxisFlip;
         float mulAxis = (allowFlip && lenDir < lenDirMinus&& fabsf(lenDir - lenDirMinus) > FLT_EPSILON) ? -1.f : 1.f;
         float mulAxisX = (allowFlip && lenDirPlaneX < lenDirMinusPlaneX&& fabsf(lenDirPlaneX - lenDirMinusPlaneX) > FLT_EPSILON) ? -1.f : 1.f;
         float mulAxisY = (allowFlip && lenDirPlaneY < lenDirMinusPlaneY&& fabsf(lenDirPlaneY - lenDirMinusPlaneY) > FLT_EPSILON) ? -1.f : 1.f;
         dirAxis *= mulAxis;
         dirPlaneX *= mulAxisX;
         dirPlaneY *= mulAxisY;

         // for axis
         float axisLengthInClipSpace = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirAxis * gContext.mScreenFactor, localCoordinates);

         float paraSurf = GetParallelogram(makeVect(0.f, 0.f, 0.f), dirPlaneX * gContext.mScreenFactor, dirPlaneY * gContext.mScreenFactor);
         belowPlaneLimit = (paraSurf > gContext.mAxisLimit);
         belowAxisLimit = (axisLengthInClipSpace > gContext.mPlaneLimit);

         // and store values
         gContext.mAxisFactor[axisIndex] = mulAxis;
         gContext.mAxisFactor[(axisIndex + 1) % 3] = mulAxisX;
         gContext.mAxisFactor[(axisIndex + 2) % 3] = mulAxisY;
         gContext.mBelowAxisLimit[axisIndex] = belowAxisLimit;
         gContext.mBelowPlaneLimit[axisIndex] = belowPlaneLimit;
      }
   }

   static void ComputeSnap(float* value, float snap)
   {
      if (snap <= FLT_EPSILON)
      {
         return;
      }

      float modulo = fmodf(*value, snap);
      float moduloRatio = fabsf(modulo) / snap;
      if (moduloRatio < snapTension)
      {
         *value -= modulo;
      }
      else if (moduloRatio > (1.f - snapTension))
      {
         *value = *value - modulo + snap * ((*value < 0.f) ? -1.f : 1.f);
      }
   }
   static void ComputeSnap(vec_t& value, const float* snap)
   {
      for (int i = 0; i < 3; i++)
      {
         ComputeSnap(&value[i], snap[i]);
      }
   }

   static float ComputeAngleOnPlan()
   {
      const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
      vec_t localPos = Normalized(gContext.mRayOrigin + gContext.mRayVector * len - gContext.mModel.v.position);

      vec_t perpendicularVector;
      perpendicularVector.Cross(gContext.mRotationVectorSource, gContext.mTranslationPlan);
      perpendicularVector.Normalize();
      float acosAngle = Clamp(Dot(localPos, gContext.mRotationVectorSource), -1.f, 1.f);
      float angle = acosf(acosAngle);
      angle *= (Dot(localPos, perpendicularVector) < 0.f) ? 1.f : -1.f;
      return angle;
   }

   static void DrawRotationGizmo(OPERATION op, int type)
   {
      if(!Intersects(op, ROTATE))
      {
         return;
      }
      ImDrawList* drawList = gContext.mDrawList;

      // colors
      ImU32 colors[7];
      ComputeColors(colors, type, ROTATE);

      vec_t cameraToModelNormalized;
      if (gContext.mIsOrthographic)
      {
         matrix_t viewInverse;
         viewInverse.Inverse(*(matrix_t*)&gContext.mViewMat);
         cameraToModelNormalized = -viewInverse.v.dir;
      }
      else
      {
         cameraToModelNormalized = Normalized(gContext.mModel.v.position - gContext.mCameraPos);
      }

      cameraToModelNormalized.TransformVector(gContext.mModelInverse);

      gContext.mRadiusSquareCenter = screenRotateSize * gContext.mHeight;

      bool hasRSC = Intersects(op, ROTATE_SCREEN);
      for (int axis = 0; axis < 3; axis++)
      {
         if(!Intersects(op, static_cast<OPERATION>(ROTATE_Z >> axis)))
         {
            continue;
         }
         const bool usingAxis = (gContext.mbUsing && type == MT_ROTATE_Z - axis);
         const int circleMul = (hasRSC && !usingAxis ) ? 1 : 2;

         ImVec2* circlePos = (ImVec2*)alloca(sizeof(ImVec2) * (circleMul * halfCircleSegmentCount + 1));

         float angleStart = atan2f(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + PI * 0.5f;

         for (int i = 0; i < circleMul * halfCircleSegmentCount + 1; i++)
         {
            float ng = angleStart + (float)circleMul * PI * ((float)i / (float)(circleMul * halfCircleSegmentCount));
            vec_t axisPos = makeVect(cosf(ng), sinf(ng), 0.f);
            vec_t pos = makeVect(axisPos[axis], axisPos[(axis + 1) % 3], axisPos[(axis + 2) % 3]) * gContext.mScreenFactor * rotationDisplayFactor;
            circlePos[i] = worldToPos(pos, gContext.mMVP);
         }
         if (!gContext.mbUsing || usingAxis)
         {
            drawList->AddPolyline(circlePos, circleMul* halfCircleSegmentCount + 1, colors[3 - axis], false, gContext.mStyle.RotationLineThickness);
         }

         float radiusAxis = sqrtf((ImLengthSqr(worldToPos(gContext.mModel.v.position, gContext.mViewProjection) - circlePos[0])));
         if (radiusAxis > gContext.mRadiusSquareCenter)
         {
            gContext.mRadiusSquareCenter = radiusAxis;
         }
      }
      if(hasRSC && (!gContext.mbUsing || type == MT_ROTATE_SCREEN))
      {
         drawList->AddCircle(worldToPos(gContext.mModel.v.position, gContext.mViewProjection), gContext.mRadiusSquareCenter, colors[0], 64, gContext.mStyle.RotationOuterLineThickness);
      }

      if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID) && IsRotateType(type))
      {
         ImVec2 circlePos[halfCircleSegmentCount + 1];

         circlePos[0] = worldToPos(gContext.mModel.v.position, gContext.mViewProjection);
         for (unsigned int i = 1; i < halfCircleSegmentCount + 1; i++)
         {
            float ng = gContext.mRotationAngle * ((float)(i - 1) / (float)(halfCircleSegmentCount - 1));
            matrix_t rotateVectorMatrix;
            rotateVectorMatrix.RotationAxis(gContext.mTranslationPlan, ng);
            vec_t pos;
            pos.TransformPoint(gContext.mRotationVectorSource, rotateVectorMatrix);
            pos *= gContext.mScreenFactor * rotationDisplayFactor;
            circlePos[i] = worldToPos(pos + gContext.mModel.v.position, gContext.mViewProjection);
         }
         drawList->AddConvexPolyFilled(circlePos, halfCircleSegmentCount + 1, GetColorU32(ROTATION_USING_FILL));
         drawList->AddPolyline(circlePos, halfCircleSegmentCount + 1, GetColorU32(ROTATION_USING_BORDER), true, gContext.mStyle.RotationLineThickness);

         ImVec2 destinationPosOnScreen = circlePos[1];
         char tmps[512];
         ImFormatString(tmps, sizeof(tmps), rotationInfoMask[type - MT_ROTATE_X], (gContext.mRotationAngle / PI) * 180.f, gContext.mRotationAngle);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), GetColorU32(TEXT_SHADOW), tmps);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), GetColorU32(TEXT), tmps);
      }
   }

   static void DrawHatchedAxis(const vec_t& axis)
   {
      if (gContext.mStyle.HatchedAxisLineThickness <= 0.0f)
      {
         return;
      }

      for (int j = 1; j < 10; j++)
      {
         ImVec2 baseSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2) * gContext.mScreenFactor, gContext.mMVP);
         ImVec2 worldDirSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2 + 1) * gContext.mScreenFactor, gContext.mMVP);
         gContext.mDrawList->AddLine(baseSSpace2, worldDirSSpace2, GetColorU32(HATCHED_AXIS_LINES), gContext.mStyle.HatchedAxisLineThickness);
      }
   }

   static void DrawScaleGizmo(OPERATION op, int type)
   {
      ImDrawList* drawList = gContext.mDrawList;

      if(!Intersects(op, SCALE))
      {
        return;
      }

      // colors
      ImU32 colors[7];
      ComputeColors(colors, type, SCALE);

      // draw
      vec_t scaleDisplay = { 1.f, 1.f, 1.f, 1.f };

      if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID))
      {
         scaleDisplay = gContext.mScale;
      }

      for (int i = 0; i < 3; i++)
      {
         if(!Intersects(op, static_cast<OPERATION>(SCALE_X << i)))
         {
            continue;
         }
         const bool usingAxis = (gContext.mbUsing && type == MT_SCALE_X + i);
         if (!gContext.mbUsing || usingAxis)
         {
            vec_t dirPlaneX, dirPlaneY, dirAxis;
            bool belowAxisLimit, belowPlaneLimit;
            ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);

            // draw axis
            if (belowAxisLimit)
            {
               bool hasTranslateOnAxis = Contains(op, static_cast<OPERATION>(TRANSLATE_X << i));
               float markerScale = hasTranslateOnAxis ? 1.4f : 1.0f;
               ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gContext.mScreenFactor, gContext.mMVP);
               ImVec2 worldDirSSpaceNoScale = worldToPos(dirAxis * markerScale * gContext.mScreenFactor, gContext.mMVP);
               ImVec2 worldDirSSpace = worldToPos((dirAxis * markerScale * scaleDisplay[i]) * gContext.mScreenFactor, gContext.mMVP);

               if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID))
               {
                  ImU32 scaleLineColor = GetColorU32(SCALE_LINE);
                  drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, scaleLineColor, gContext.mStyle.ScaleLineThickness);
                  drawList->AddCircleFilled(worldDirSSpaceNoScale, gContext.mStyle.ScaleLineCircleSize, scaleLineColor);
               }

               if (!hasTranslateOnAxis || gContext.mbUsing)
               {
                  drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], gContext.mStyle.ScaleLineThickness);
               }
               drawList->AddCircleFilled(worldDirSSpace, gContext.mStyle.ScaleLineCircleSize, colors[i + 1]);

               if (gContext.mAxisFactor[i] < 0.f)
               {
                  DrawHatchedAxis(dirAxis * scaleDisplay[i]);
               }
            }
         }
      }

      // draw screen cirle
      drawList->AddCircleFilled(gContext.mScreenSquareCenter, gContext.mStyle.CenterCircleSize, colors[0], 32);

      if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID) && IsScaleType(type))
      {
         //ImVec2 sourcePosOnScreen = worldToPos(gContext.mMatrixOrigin, gContext.mViewProjection);
         ImVec2 destinationPosOnScreen = worldToPos(gContext.mModel.v.position, gContext.mViewProjection);
         /*vec_t dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y);
         dif.Normalize();
         dif *= 5.f;
         drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
         drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
         drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);
         */
         char tmps[512];
         //vec_t deltaInfo = gContext.mModel.v.position - gContext.mMatrixOrigin;
         int componentInfoIndex = (type - MT_SCALE_X) * 3;
         ImFormatString(tmps, sizeof(tmps), scaleInfoMask[type - MT_SCALE_X], scaleDisplay[translationInfoIndex[componentInfoIndex]]);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), GetColorU32(TEXT_SHADOW), tmps);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), GetColorU32(TEXT), tmps);
      }
   }


   static void DrawScaleUniveralGizmo(OPERATION op, int type)
   {
      ImDrawList* drawList = gContext.mDrawList;

      if (!Intersects(op, SCALEU))
      {
         return;
      }

      // colors
      ImU32 colors[7];
      ComputeColors(colors, type, SCALEU);

      // draw
      vec_t scaleDisplay = { 1.f, 1.f, 1.f, 1.f };

      if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID))
      {
         scaleDisplay = gContext.mScale;
      }

      for (int i = 0; i < 3; i++)
      {
         if (!Intersects(op, static_cast<OPERATION>(SCALE_XU << i)))
         {
            continue;
         }
         const bool usingAxis = (gContext.mbUsing && type == MT_SCALE_X + i);
         if (!gContext.mbUsing || usingAxis)
         {
            vec_t dirPlaneX, dirPlaneY, dirAxis;
            bool belowAxisLimit, belowPlaneLimit;
            ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);

            // draw axis
            if (belowAxisLimit)
            {
               bool hasTranslateOnAxis = Contains(op, static_cast<OPERATION>(TRANSLATE_X << i));
               float markerScale = hasTranslateOnAxis ? 1.4f : 1.0f;
               //ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gContext.mScreenFactor, gContext.mMVPLocal);
               //ImVec2 worldDirSSpaceNoScale = worldToPos(dirAxis * markerScale * gContext.mScreenFactor, gContext.mMVP);
               ImVec2 worldDirSSpace = worldToPos((dirAxis * markerScale * scaleDisplay[i]) * gContext.mScreenFactor, gContext.mMVPLocal);

#if 0
               if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID))
               {
                  drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, IM_COL32(0x40, 0x40, 0x40, 0xFF), 3.f);
                  drawList->AddCircleFilled(worldDirSSpaceNoScale, 6.f, IM_COL32(0x40, 0x40, 0x40, 0xFF));
               }
               /*
               if (!hasTranslateOnAxis || gContext.mbUsing)
               {
                  drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 3.f);
               }
               */
#endif
               drawList->AddCircleFilled(worldDirSSpace, 12.f, colors[i + 1]);
            }
         }
      }

      // draw screen cirle
      drawList->AddCircle(gContext.mScreenSquareCenter, 20.f, colors[0], 32, gContext.mStyle.CenterCircleSize);

      if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID) && IsScaleType(type))
      {
         //ImVec2 sourcePosOnScreen = worldToPos(gContext.mMatrixOrigin, gContext.mViewProjection);
         ImVec2 destinationPosOnScreen = worldToPos(gContext.mModel.v.position, gContext.mViewProjection);
         /*vec_t dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y);
         dif.Normalize();
         dif *= 5.f;
         drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
         drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
         drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);
         */
         char tmps[512];
         //vec_t deltaInfo = gContext.mModel.v.position - gContext.mMatrixOrigin;
         int componentInfoIndex = (type - MT_SCALE_X) * 3;
         ImFormatString(tmps, sizeof(tmps), scaleInfoMask[type - MT_SCALE_X], scaleDisplay[translationInfoIndex[componentInfoIndex]]);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), GetColorU32(TEXT_SHADOW), tmps);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), GetColorU32(TEXT), tmps);
      }
   }

   static void DrawTranslationGizmo(OPERATION op, int type)
   {
      ImDrawList* drawList = gContext.mDrawList;
      if (!drawList)
      {
         return;
      }

      if(!Intersects(op, TRANSLATE))
      {
         return;
      }

      // colors
      ImU32 colors[7];
      ComputeColors(colors, type, TRANSLATE);

      const ImVec2 origin = worldToPos(gContext.mModel.v.position, gContext.mViewProjection);

      // draw
      bool belowAxisLimit = false;
      bool belowPlaneLimit = false;
      for (int i = 0; i < 3; ++i)
      {
         vec_t dirPlaneX, dirPlaneY, dirAxis;
         ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

         if (!gContext.mbUsing || (gContext.mbUsing && type == MT_MOVE_X + i))
         {
            // draw axis
            if (belowAxisLimit && Intersects(op, static_cast<OPERATION>(TRANSLATE_X << i)))
            {
               ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gContext.mScreenFactor, gContext.mMVP);
               ImVec2 worldDirSSpace = worldToPos(dirAxis * gContext.mScreenFactor, gContext.mMVP);

               drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], gContext.mStyle.TranslationLineThickness);

               // Arrow head begin
               ImVec2 dir(origin - worldDirSSpace);

               float d = sqrtf(ImLengthSqr(dir));
               dir /= d; // Normalize
               dir *= gContext.mStyle.TranslationLineArrowSize;

               ImVec2 ortogonalDir(dir.y, -dir.x); // Perpendicular vector
               ImVec2 a(worldDirSSpace + dir);
               drawList->AddTriangleFilled(worldDirSSpace - dir, a + ortogonalDir, a - ortogonalDir, colors[i + 1]);
               // Arrow head end

               if (gContext.mAxisFactor[i] < 0.f)
               {
                  DrawHatchedAxis(dirAxis);
               }
            }
         }
         // draw plane
         if (!gContext.mbUsing || (gContext.mbUsing && type == MT_MOVE_YZ + i))
         {
            if (belowPlaneLimit && Contains(op, TRANSLATE_PLANS[i]))
            {
               ImVec2 screenQuadPts[4];
               for (int j = 0; j < 4; ++j)
               {
                  vec_t cornerWorldPos = (dirPlaneX * quadUV[j * 2] + dirPlaneY * quadUV[j * 2 + 1]) * gContext.mScreenFactor;
                  screenQuadPts[j] = worldToPos(cornerWorldPos, gContext.mMVP);
               }
               drawList->AddPolyline(screenQuadPts, 4, GetColorU32(DIRECTION_X + i), true, 1.0f);
               drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[i + 4]);
            }
         }
      }

      drawList->AddCircleFilled(gContext.mScreenSquareCenter, gContext.mStyle.CenterCircleSize, colors[0], 32);

      if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID) && IsTranslateType(type))
      {
         ImU32 translationLineColor = GetColorU32(TRANSLATION_LINE);

         ImVec2 sourcePosOnScreen = worldToPos(gContext.mMatrixOrigin, gContext.mViewProjection);
         ImVec2 destinationPosOnScreen = worldToPos(gContext.mModel.v.position, gContext.mViewProjection);
         vec_t dif = { destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y, 0.f, 0.f };
         dif.Normalize();
         dif *= 5.f;
         drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
         drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
         drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);

         char tmps[512];
         vec_t deltaInfo = gContext.mModel.v.position - gContext.mMatrixOrigin;
         int componentInfoIndex = (type - MT_MOVE_X) * 3;
         ImFormatString(tmps, sizeof(tmps), translationInfoMask[type - MT_MOVE_X], deltaInfo[translationInfoIndex[componentInfoIndex]], deltaInfo[translationInfoIndex[componentInfoIndex + 1]], deltaInfo[translationInfoIndex[componentInfoIndex + 2]]);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), GetColorU32(TEXT_SHADOW), tmps);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), GetColorU32(TEXT), tmps);
      }
   }

   static bool CanActivate()
   {
      if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive())
      {
         return true;
      }
      return false;
   }

   static void HandleAndDrawLocalBounds(const float* bounds, matrix_t* matrix, const float* snapValues, OPERATION operation)
   {
      ImGuiIO& io = ImGui::GetIO();
      ImDrawList* drawList = gContext.mDrawList;

      // compute best projection axis
      vec_t axesWorldDirections[3];
      vec_t bestAxisWorldDirection = { 0.0f, 0.0f, 0.0f, 0.0f };
      int axes[3];
      unsigned int numAxes = 1;
      axes[0] = gContext.mBoundsBestAxis;
      int bestAxis = axes[0];
      if (!gContext.mbUsingBounds)
      {
         numAxes = 0;
         float bestDot = 0.f;
         for (int i = 0; i < 3; i++)
         {
            vec_t dirPlaneNormalWorld;
            dirPlaneNormalWorld.TransformVector(directionUnary[i], gContext.mModelSource);
            dirPlaneNormalWorld.Normalize();

            float dt = fabsf(Dot(Normalized(gContext.mCameraPos - gContext.mModelSource.v.position), dirPlaneNormalWorld));
            if (dt >= bestDot)
            {
               bestDot = dt;
               bestAxis = i;
               bestAxisWorldDirection = dirPlaneNormalWorld;
            }

            if (dt >= 0.1f)
            {
               axes[numAxes] = i;
               axesWorldDirections[numAxes] = dirPlaneNormalWorld;
               ++numAxes;
            }
         }
      }

      if (numAxes == 0)
      {
         axes[0] = bestAxis;
         axesWorldDirections[0] = bestAxisWorldDirection;
         numAxes = 1;
      }

      else if (bestAxis != axes[0])
      {
         unsigned int bestIndex = 0;
         for (unsigned int i = 0; i < numAxes; i++)
         {
            if (axes[i] == bestAxis)
            {
               bestIndex = i;
               break;
            }
         }
         int tempAxis = axes[0];
         axes[0] = axes[bestIndex];
         axes[bestIndex] = tempAxis;
         vec_t tempDirection = axesWorldDirections[0];
         axesWorldDirections[0] = axesWorldDirections[bestIndex];
         axesWorldDirections[bestIndex] = tempDirection;
      }

      for (unsigned int axisIndex = 0; axisIndex < numAxes; ++axisIndex)
      {
         bestAxis = axes[axisIndex];
         bestAxisWorldDirection = axesWorldDirections[axisIndex];

         // corners
         vec_t aabb[4];

         int secondAxis = (bestAxis + 1) % 3;
         int thirdAxis = (bestAxis + 2) % 3;

         for (int i = 0; i < 4; i++)
         {
            aabb[i][3] = aabb[i][bestAxis] = 0.f;
            aabb[i][secondAxis] = bounds[secondAxis + 3 * (i >> 1)];
            aabb[i][thirdAxis] = bounds[thirdAxis + 3 * ((i >> 1) ^ (i & 1))];
         }

         // draw bounds
         unsigned int anchorAlpha = gContext.mbEnable ? IM_COL32_BLACK : IM_COL32(0, 0, 0, 0x80);

         matrix_t boundsMVP = gContext.mModelSource * gContext.mViewProjection;
         for (int i = 0; i < 4; i++)
         {
            ImVec2 worldBound1 = worldToPos(aabb[i], boundsMVP);
            ImVec2 worldBound2 = worldToPos(aabb[(i + 1) % 4], boundsMVP);
            if (!IsInContextRect(worldBound1) || !IsInContextRect(worldBound2))
            {
               continue;
            }
            float boundDistance = sqrtf(ImLengthSqr(worldBound1 - worldBound2));
            int stepCount = (int)(boundDistance / 10.f);
            stepCount = min(stepCount, 1000);
            for (int j = 0; j < stepCount; j++)
            {
               float stepLength = 1.f / (float)stepCount;
               float t1 = (float)j * stepLength;
               float t2 = (float)j * stepLength + stepLength * 0.5f;
               ImVec2 worldBoundSS1 = ImLerp(worldBound1, worldBound2, ImVec2(t1, t1));
               ImVec2 worldBoundSS2 = ImLerp(worldBound1, worldBound2, ImVec2(t2, t2));
               //drawList->AddLine(worldBoundSS1, worldBoundSS2, IM_COL32(0, 0, 0, 0) + anchorAlpha, 3.f);
               drawList->AddLine(worldBoundSS1, worldBoundSS2, IM_COL32(0xAA, 0xAA, 0xAA, 0) + anchorAlpha, 2.f);
            }
            vec_t midPoint = (aabb[i] + aabb[(i + 1) % 4]) * 0.5f;
            ImVec2 midBound = worldToPos(midPoint, boundsMVP);
            static const float AnchorBigRadius = 8.f;
            static const float AnchorSmallRadius = 6.f;
            bool overBigAnchor = ImLengthSqr(worldBound1 - io.MousePos) <= (AnchorBigRadius * AnchorBigRadius);
            bool overSmallAnchor = ImLengthSqr(midBound - io.MousePos) <= (AnchorBigRadius * AnchorBigRadius);

            int type = MT_NONE;
            vec_t gizmoHitProportion;

            if(Intersects(operation, TRANSLATE))
            {
               type = GetMoveType(operation, &gizmoHitProportion);
            }
            if(Intersects(operation, ROTATE) && type == MT_NONE)
            {
               type = GetRotateType(operation);
            }
            if(Intersects(operation, SCALE) && type == MT_NONE)
            {
               type = GetScaleType(operation);
            }

            if (type != MT_NONE)
            {
               overBigAnchor = false;
               overSmallAnchor = false;
            }

            ImU32 selectionColor = GetColorU32(SELECTION);

            unsigned int bigAnchorColor = overBigAnchor ? selectionColor : (IM_COL32(0xAA, 0xAA, 0xAA, 0) + anchorAlpha);
            unsigned int smallAnchorColor = overSmallAnchor ? selectionColor : (IM_COL32(0xAA, 0xAA, 0xAA, 0) + anchorAlpha);

            drawList->AddCircleFilled(worldBound1, AnchorBigRadius, IM_COL32_BLACK);
            drawList->AddCircleFilled(worldBound1, AnchorBigRadius - 1.2f, bigAnchorColor);

            drawList->AddCircleFilled(midBound, AnchorSmallRadius, IM_COL32_BLACK);
            drawList->AddCircleFilled(midBound, AnchorSmallRadius - 1.2f, smallAnchorColor);
            int oppositeIndex = (i + 2) % 4;
            // big anchor on corners
            if (!gContext.mbUsingBounds && gContext.mbEnable && overBigAnchor && CanActivate())
            {
               gContext.mBoundsPivot.TransformPoint(aabb[(i + 2) % 4], gContext.mModelSource);
               gContext.mBoundsAnchor.TransformPoint(aabb[i], gContext.mModelSource);
               gContext.mBoundsPlan = BuildPlan(gContext.mBoundsAnchor, bestAxisWorldDirection);
               gContext.mBoundsBestAxis = bestAxis;
               gContext.mBoundsAxis[0] = secondAxis;
               gContext.mBoundsAxis[1] = thirdAxis;

               gContext.mBoundsLocalPivot.Set(0.f);
               gContext.mBoundsLocalPivot[secondAxis] = aabb[oppositeIndex][secondAxis];
               gContext.mBoundsLocalPivot[thirdAxis] = aabb[oppositeIndex][thirdAxis];

               gContext.mbUsingBounds = true;
               gContext.mEditingID = gContext.mActualID;
               gContext.mBoundsMatrix = gContext.mModelSource;
            }
            // small anchor on middle of segment
            if (!gContext.mbUsingBounds && gContext.mbEnable && overSmallAnchor && CanActivate())
            {
               vec_t midPointOpposite = (aabb[(i + 2) % 4] + aabb[(i + 3) % 4]) * 0.5f;
               gContext.mBoundsPivot.TransformPoint(midPointOpposite, gContext.mModelSource);
               gContext.mBoundsAnchor.TransformPoint(midPoint, gContext.mModelSource);
               gContext.mBoundsPlan = BuildPlan(gContext.mBoundsAnchor, bestAxisWorldDirection);
               gContext.mBoundsBestAxis = bestAxis;
               int indices[] = { secondAxis , thirdAxis };
               gContext.mBoundsAxis[0] = indices[i % 2];
               gContext.mBoundsAxis[1] = -1;

               gContext.mBoundsLocalPivot.Set(0.f);
               gContext.mBoundsLocalPivot[gContext.mBoundsAxis[0]] = aabb[oppositeIndex][indices[i % 2]];// bounds[gContext.mBoundsAxis[0]] * (((i + 1) & 2) ? 1.f : -1.f);

               gContext.mbUsingBounds = true;
               gContext.mEditingID = gContext.mActualID;
               gContext.mBoundsMatrix = gContext.mModelSource;
            }
         }

         if (gContext.mbUsingBounds && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID))
         {
            matrix_t scale;
            scale.SetToIdentity();

            // compute projected mouse position on plan
            const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mBoundsPlan);
            vec_t newPos = gContext.mRayOrigin + gContext.mRayVector * len;

            // compute a reference and delta vectors base on mouse move
            vec_t deltaVector = (newPos - gContext.mBoundsPivot).Abs();
            vec_t referenceVector = (gContext.mBoundsAnchor - gContext.mBoundsPivot).Abs();

            // for 1 or 2 axes, compute a ratio that's used for scale and snap it based on resulting length
            for (int i = 0; i < 2; i++)
            {
               int axisIndex1 = gContext.mBoundsAxis[i];
               if (axisIndex1 == -1)
               {
                  continue;
               }

               float ratioAxis = 1.f;
               vec_t axisDir = gContext.mBoundsMatrix.component[axisIndex1].Abs();

               float dtAxis = axisDir.Dot(referenceVector);
               float boundSize = bounds[axisIndex1 + 3] - bounds[axisIndex1];
               if (dtAxis > FLT_EPSILON)
               {
                  ratioAxis = axisDir.Dot(deltaVector) / dtAxis;
               }

               if (snapValues)
               {
                  float length = boundSize * ratioAxis;
                  ComputeSnap(&length, snapValues[axisIndex1]);
                  if (boundSize > FLT_EPSILON)
                  {
                     ratioAxis = length / boundSize;
                  }
               }
               scale.component[axisIndex1] *= ratioAxis;
            }

            // transform matrix
            matrix_t preScale, postScale;
            preScale.Translation(-gContext.mBoundsLocalPivot);
            postScale.Translation(gContext.mBoundsLocalPivot);
            matrix_t res = preScale * scale * postScale * gContext.mBoundsMatrix;
            *matrix = res;

            // info text
            char tmps[512];
            ImVec2 destinationPosOnScreen = worldToPos(gContext.mModel.v.position, gContext.mViewProjection);
            ImFormatString(tmps, sizeof(tmps), "X: %.2f Y: %.2f Z: %.2f"
               , (bounds[3] - bounds[0]) * gContext.mBoundsMatrix.component[0].Length() * scale.component[0].Length()
               , (bounds[4] - bounds[1]) * gContext.mBoundsMatrix.component[1].Length() * scale.component[1].Length()
               , (bounds[5] - bounds[2]) * gContext.mBoundsMatrix.component[2].Length() * scale.component[2].Length()
            );
            drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), GetColorU32(TEXT_SHADOW), tmps);
            drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), GetColorU32(TEXT), tmps);
         }

         if (!io.MouseDown[0]) {
            gContext.mbUsingBounds = false;
            gContext.mEditingID = -1;
         }
         if (gContext.mbUsingBounds)
         {
            break;
         }
      }
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //

   static int GetScaleType(OPERATION op)
   {
      if (gContext.mbUsing)
      {
         return MT_NONE;
      }
      ImGuiIO& io = ImGui::GetIO();
      int type = MT_NONE;

      // screen
      if (io.MousePos.x >= gContext.mScreenSquareMin.x && io.MousePos.x <= gContext.mScreenSquareMax.x &&
         io.MousePos.y >= gContext.mScreenSquareMin.y && io.MousePos.y <= gContext.mScreenSquareMax.y &&
         Contains(op, SCALE))
      {
         type = MT_SCALE_XYZ;
      }

      // compute
      for (int i = 0; i < 3 && type == MT_NONE; i++)
      {
         if(!Intersects(op, static_cast<OPERATION>(SCALE_X << i)))
         {
            continue;
         }
         vec_t dirPlaneX, dirPlaneY, dirAxis;
         bool belowAxisLimit, belowPlaneLimit;
         ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);
         dirAxis.TransformVector(gContext.mModelLocal);
         dirPlaneX.TransformVector(gContext.mModelLocal);
         dirPlaneY.TransformVector(gContext.mModelLocal);

         const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, BuildPlan(gContext.mModelLocal.v.position, dirAxis));
         vec_t posOnPlan = gContext.mRayOrigin + gContext.mRayVector * len;

         const float startOffset = Contains(op, static_cast<OPERATION>(TRANSLATE_X << i)) ? 1.0f : 0.1f;
         const float endOffset = Contains(op, static_cast<OPERATION>(TRANSLATE_X << i)) ? 1.4f : 1.0f;
         const ImVec2 posOnPlanScreen = worldToPos(posOnPlan, gContext.mViewProjection);
         const ImVec2 axisStartOnScreen = worldToPos(gContext.mModelLocal.v.position + dirAxis * gContext.mScreenFactor * startOffset, gContext.mViewProjection);
         const ImVec2 axisEndOnScreen = worldToPos(gContext.mModelLocal.v.position + dirAxis * gContext.mScreenFactor * endOffset, gContext.mViewProjection);

         vec_t closestPointOnAxis = PointOnSegment(makeVect(posOnPlanScreen), makeVect(axisStartOnScreen), makeVect(axisEndOnScreen));

         if ((closestPointOnAxis - makeVect(posOnPlanScreen)).Length() < 12.f) // pixel size
         {
            type = MT_SCALE_X + i;
         }
      }

      // universal

      vec_t deltaScreen = { io.MousePos.x - gContext.mScreenSquareCenter.x, io.MousePos.y - gContext.mScreenSquareCenter.y, 0.f, 0.f };
      float dist = deltaScreen.Length();
      if (Contains(op, SCALEU) && dist >= 17.0f && dist < 23.0f)
      {
         type = MT_SCALE_XYZ;
      }

      for (int i = 0; i < 3 && type == MT_NONE; i++)
      {
         if (!Intersects(op, static_cast<OPERATION>(SCALE_XU << i)))
         {
            continue;
         }

         vec_t dirPlaneX, dirPlaneY, dirAxis;
         bool belowAxisLimit, belowPlaneLimit;
         ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);

         // draw axis
         if (belowAxisLimit)
         {
            bool hasTranslateOnAxis = Contains(op, static_cast<OPERATION>(TRANSLATE_X << i));
            float markerScale = hasTranslateOnAxis ? 1.4f : 1.0f;
            //ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gContext.mScreenFactor, gContext.mMVPLocal);
            //ImVec2 worldDirSSpaceNoScale = worldToPos(dirAxis * markerScale * gContext.mScreenFactor, gContext.mMVP);
            ImVec2 worldDirSSpace = worldToPos((dirAxis * markerScale) * gContext.mScreenFactor, gContext.mMVPLocal);

            float distance = sqrtf(ImLengthSqr(worldDirSSpace - io.MousePos));
            if (distance < 12.f)
            {
               type = MT_SCALE_X + i;
            }
         }
      }
      return type;
   }

   static int GetRotateType(OPERATION op)
   {
      if (gContext.mbUsing)
      {
         return MT_NONE;
      }
      ImGuiIO& io = ImGui::GetIO();
      int type = MT_NONE;

      vec_t deltaScreen = { io.MousePos.x - gContext.mScreenSquareCenter.x, io.MousePos.y - gContext.mScreenSquareCenter.y, 0.f, 0.f };
      float dist = deltaScreen.Length();
      if (Intersects(op, ROTATE_SCREEN) && dist >= (gContext.mRadiusSquareCenter - 4.0f) && dist < (gContext.mRadiusSquareCenter + 4.0f))
      {
         type = MT_ROTATE_SCREEN;
      }

      const vec_t planNormals[] = { gContext.mModel.v.right, gContext.mModel.v.up, gContext.mModel.v.dir };

      vec_t modelViewPos;
      modelViewPos.TransformPoint(gContext.mModel.v.position, gContext.mViewMat);

      for (int i = 0; i < 3 && type == MT_NONE; i++)
      {
         if(!Intersects(op, static_cast<OPERATION>(ROTATE_X << i)))
         {
            continue;
         }
         // pickup plan
         vec_t pickupPlan = BuildPlan(gContext.mModel.v.position, planNormals[i]);

         const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, pickupPlan);
         const vec_t intersectWorldPos = gContext.mRayOrigin + gContext.mRayVector * len;
         vec_t intersectViewPos;
         intersectViewPos.TransformPoint(intersectWorldPos, gContext.mViewMat);

         if (ImAbs(modelViewPos.z) - ImAbs(intersectViewPos.z) < -FLT_EPSILON)
         {
            continue;
         }

         const vec_t localPos = intersectWorldPos - gContext.mModel.v.position;
         vec_t idealPosOnCircle = Normalized(localPos);
         idealPosOnCircle.TransformVector(gContext.mModelInverse);
         const ImVec2 idealPosOnCircleScreen = worldToPos(idealPosOnCircle * rotationDisplayFactor * gContext.mScreenFactor, gContext.mMVP);

         //gContext.mDrawList->AddCircle(idealPosOnCircleScreen, 5.f, IM_COL32_WHITE);
         const ImVec2 distanceOnScreen = idealPosOnCircleScreen - io.MousePos;

         const float distance = makeVect(distanceOnScreen).Length();
         if (distance < 8.f) // pixel size
         {
            type = MT_ROTATE_X + i;
         }
      }

      return type;
   }

   static int GetMoveType(OPERATION op, vec_t* gizmoHitProportion)
   {
      if(!Intersects(op, TRANSLATE) || gContext.mbUsing || !gContext.mbMouseOver)
      {
        return MT_NONE;
      }
      ImGuiIO& io = ImGui::GetIO();
      int type = MT_NONE;

      // screen
      if (io.MousePos.x >= gContext.mScreenSquareMin.x && io.MousePos.x <= gContext.mScreenSquareMax.x &&
         io.MousePos.y >= gContext.mScreenSquareMin.y && io.MousePos.y <= gContext.mScreenSquareMax.y &&
         Contains(op, TRANSLATE))
      {
         type = MT_MOVE_SCREEN;
      }

      const vec_t screenCoord = makeVect(io.MousePos - ImVec2(gContext.mX, gContext.mY));

      // compute
      for (int i = 0; i < 3 && type == MT_NONE; i++)
      {
         vec_t dirPlaneX, dirPlaneY, dirAxis;
         bool belowAxisLimit, belowPlaneLimit;
         ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
         dirAxis.TransformVector(gContext.mModel);
         dirPlaneX.TransformVector(gContext.mModel);
         dirPlaneY.TransformVector(gContext.mModel);

         const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, BuildPlan(gContext.mModel.v.position, dirAxis));
         vec_t posOnPlan = gContext.mRayOrigin + gContext.mRayVector * len;

         const ImVec2 axisStartOnScreen = worldToPos(gContext.mModel.v.position + dirAxis * gContext.mScreenFactor * 0.1f, gContext.mViewProjection) - ImVec2(gContext.mX, gContext.mY);
         const ImVec2 axisEndOnScreen = worldToPos(gContext.mModel.v.position + dirAxis * gContext.mScreenFactor, gContext.mViewProjection) - ImVec2(gContext.mX, gContext.mY);

         vec_t closestPointOnAxis = PointOnSegment(screenCoord, makeVect(axisStartOnScreen), makeVect(axisEndOnScreen));
         if ((closestPointOnAxis - screenCoord).Length() < 12.f && Intersects(op, static_cast<OPERATION>(TRANSLATE_X << i))) // pixel size
         {
            type = MT_MOVE_X + i;
         }

         const float dx = dirPlaneX.Dot3((posOnPlan - gContext.mModel.v.position) * (1.f / gContext.mScreenFactor));
         const float dy = dirPlaneY.Dot3((posOnPlan - gContext.mModel.v.position) * (1.f / gContext.mScreenFactor));
         if (belowPlaneLimit && dx >= quadUV[0] && dx <= quadUV[4] && dy >= quadUV[1] && dy <= quadUV[3] && Contains(op, TRANSLATE_PLANS[i]))
         {
            type = MT_MOVE_YZ + i;
         }

         if (gizmoHitProportion)
         {
            *gizmoHitProportion = makeVect(dx, dy, 0.f);
         }
      }
      return type;
   }

   static bool HandleTranslation(float* matrix, float* deltaMatrix, OPERATION op, int& type, const float* snap)
   {
      if(!Intersects(op, TRANSLATE) || type != MT_NONE)
      {
        return false;
      }
      const ImGuiIO& io = ImGui::GetIO();
      const bool applyRotationLocaly = gContext.mHandleSpace == LOCAL || type == MT_MOVE_SCREEN;
      bool modified = false;

      // move
      if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID) && IsTranslateType(gContext.mCurrentOperation))
      {
#if IMGUI_VERSION_NUM >= 18723
         ImGui::SetNextFrameWantCaptureMouse(true);
#else
         ImGui::CaptureMouseFromApp();
#endif
         const float signedLength = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
         const float len = fabsf(signedLength); // near plan
         const vec_t newPos = gContext.mRayOrigin + gContext.mRayVector * len;

         // compute delta
         const vec_t newOrigin = newPos - gContext.mRelativeOrigin * gContext.mScreenFactor;
         vec_t delta = newOrigin - gContext.mModel.v.position;

         // 1 axis constraint
         if (gContext.mCurrentOperation >= MT_MOVE_X && gContext.mCurrentOperation <= MT_MOVE_Z)
         {
            const int axisIndex = gContext.mCurrentOperation - MT_MOVE_X;
            const vec_t& axisValue = *(vec_t*)&gContext.mModel.m[axisIndex];
            const float lengthOnAxis = Dot(axisValue, delta);
            delta = axisValue * lengthOnAxis;
         }

         // snap
         if (snap)
         {
            vec_t cumulativeDelta = gContext.mModel.v.position + delta - gContext.mMatrixOrigin;
            if (applyRotationLocaly)
            {
               matrix_t modelSourceNormalized = gContext.mModelSource;
               modelSourceNormalized.OrthoNormalize();
               matrix_t modelSourceNormalizedInverse;
               modelSourceNormalizedInverse.Inverse(modelSourceNormalized);
               cumulativeDelta.TransformVector(modelSourceNormalizedInverse);
               ComputeSnap(cumulativeDelta, snap);
               cumulativeDelta.TransformVector(modelSourceNormalized);
            }
            else
            {
               ComputeSnap(cumulativeDelta, snap);
            }
            delta = gContext.mMatrixOrigin + cumulativeDelta - gContext.mModel.v.position;

         }

         if (delta != gContext.mTranslationLastDelta)
         {
            modified = true;
         }
         gContext.mTranslationLastDelta = delta;

         // compute matrix & delta
         matrix_t deltaMatrixTranslation;
         deltaMatrixTranslation.Translation(delta);
         if (deltaMatrix)
         {
            memcpy(deltaMatrix, deltaMatrixTranslation.m16, sizeof(float) * 16);
         }

         const matrix_t res = gContext.mModelSource * deltaMatrixTranslation;
         *(matrix_t*)matrix = res;

         if (!io.MouseDown[0])
         {
            gContext.mbUsing = false;
         }

         type = gContext.mCurrentOperation;
      }
      else
      {
         // find new possible way to move
         vec_t gizmoHitProportion;
         type = GetMoveType(op, &gizmoHitProportion);
         if (type != MT_NONE)
         {
#if IMGUI_VERSION_NUM >= 18723
            ImGui::SetNextFrameWantCaptureMouse(true);
#else
            ImGui::CaptureMouseFromApp();
#endif
         }
         if (CanActivate() && type != MT_NONE)
         {
            gContext.mbUsing = true;
            gContext.mEditingID = gContext.mActualID;
            gContext.mCurrentOperation = type;
            vec_t movePlanNormal[] = { gContext.mModel.v.right, gContext.mModel.v.up, gContext.mModel.v.dir,
               gContext.mModel.v.right, gContext.mModel.v.up, gContext.mModel.v.dir,
               -gContext.mCameraForward };

            vec_t cameraToModelNormalized = Normalized(gContext.mModel.v.position - gContext.mCameraPos);
            for (unsigned int i = 0; i < 3; i++)
            {
               vec_t orthoVector = Cross(movePlanNormal[i], cameraToModelNormalized);
               movePlanNormal[i].Cross(orthoVector);
               movePlanNormal[i].Normalize();
            }
            // pickup plan
            gContext.mTranslationPlan = BuildPlan(gContext.mModel.v.position, movePlanNormal[type - MT_MOVE_X]);
            const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
            gContext.mTranslationPlanOrigin = gContext.mRayOrigin + gContext.mRayVector * len;
            gContext.mMatrixOrigin = gContext.mModel.v.position;

            gContext.mRelativeOrigin = (gContext.mTranslationPlanOrigin - gContext.mModel.v.position) * (1.f / gContext.mScreenFactor);
         }
      }
      return modified;
   }

   static bool HandleScale(float* matrix, float* deltaMatrix, OPERATION op, int& type, const float* snap)
   {
      if((!Intersects(op, SCALE) && !Intersects(op, SCALEU)) || type != MT_NONE || !gContext.mbMouseOver)
      {
         return false;
      }
      ImGuiIO& io = ImGui::GetIO();
      bool modified = false;

      if (!gContext.mbUsing)
      {
         // find new possible way to scale
         type = GetScaleType(op);
         if (type != MT_NONE)
         {
#if IMGUI_VERSION_NUM >= 18723
            ImGui::SetNextFrameWantCaptureMouse(true);
#else
            ImGui::CaptureMouseFromApp();
#endif
         }
         if (CanActivate() && type != MT_NONE)
         {
            gContext.mbUsing = true;
            gContext.mEditingID = gContext.mActualID;
            gContext.mCurrentOperation = type;
            const vec_t movePlanNormal[] = { gContext.mModel.v.up, gContext.mModel.v.dir, gContext.mModel.v.right, gContext.mModel.v.dir, gContext.mModel.v.up, gContext.mModel.v.right, -gContext.mCameraForward };
            // pickup plan

            gContext.mTranslationPlan = BuildPlan(gContext.mModel.v.position, movePlanNormal[type - MT_SCALE_X]);
            const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
            gContext.mTranslationPlanOrigin = gContext.mRayOrigin + gContext.mRayVector * len;
            gContext.mMatrixOrigin = gContext.mModel.v.position;
            gContext.mScale.Set(1.f, 1.f, 1.f);
            gContext.mRelativeOrigin = (gContext.mTranslationPlanOrigin - gContext.mModel.v.position) * (1.f / gContext.mScreenFactor);
            gContext.mScaleValueOrigin = makeVect(gContext.mModelSource.v.right.Length(), gContext.mModelSource.v.up.Length(), gContext.mModelSource.v.dir.Length());
            gContext.mSavedMousePosX = io.MousePos.x;
         }
      }
      // scale
      if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID) && IsScaleType(gContext.mCurrentOperation))
      {
#if IMGUI_VERSION_NUM >= 18723
         ImGui::SetNextFrameWantCaptureMouse(true);
#else
         ImGui::CaptureMouseFromApp();
#endif
         const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
         vec_t newPos = gContext.mRayOrigin + gContext.mRayVector * len;
         vec_t newOrigin = newPos - gContext.mRelativeOrigin * gContext.mScreenFactor;
         vec_t delta = newOrigin - gContext.mModelLocal.v.position;

         // 1 axis constraint
         if (gContext.mCurrentOperation >= MT_SCALE_X && gContext.mCurrentOperation <= MT_SCALE_Z)
         {
            int axisIndex = gContext.mCurrentOperation - MT_SCALE_X;
            const vec_t& axisValue = *(vec_t*)&gContext.mModelLocal.m[axisIndex];
            float lengthOnAxis = Dot(axisValue, delta);
            delta = axisValue * lengthOnAxis;

            vec_t baseVector = gContext.mTranslationPlanOrigin - gContext.mModelLocal.v.position;
            float ratio = Dot(axisValue, baseVector + delta) / Dot(axisValue, baseVector);

            gContext.mScale[axisIndex] = max(ratio, 0.001f);
         }
         else
         {
            float scaleDelta = (io.MousePos.x - gContext.mSavedMousePosX) * 0.01f;
            gContext.mScale.Set(max(1.f + scaleDelta, 0.001f));
         }

         // snap
         if (snap)
         {
            float scaleSnap[] = { snap[0], snap[0], snap[0] };
            ComputeSnap(gContext.mScale, scaleSnap);
         }

         // no 0 allowed
         for (int i = 0; i < 3; i++)
            gContext.mScale[i] = max(gContext.mScale[i], 0.001f);

         if (gContext.mScaleLast != gContext.mScale)
         {
            modified = true;
         }
         gContext.mScaleLast = gContext.mScale;

         // compute matrix & delta
         matrix_t deltaMatrixScale;
         deltaMatrixScale.Scale(gContext.mScale * gContext.mScaleValueOrigin);

         matrix_t res = deltaMatrixScale * gContext.mModelLocal;
         *(matrix_t*)matrix = res;

         if (deltaMatrix)
         {
            vec_t deltaScale = gContext.mScale * gContext.mScaleValueOrigin;

            vec_t originalScaleDivider;
            originalScaleDivider.x = 1 / gContext.mModelScaleOrigin.x;
            originalScaleDivider.y = 1 / gContext.mModelScaleOrigin.y;
            originalScaleDivider.z = 1 / gContext.mModelScaleOrigin.z;

            deltaScale = deltaScale * originalScaleDivider;

            deltaMatrixScale.Scale(deltaScale);
            memcpy(deltaMatrix, deltaMatrixScale.m16, sizeof(float) * 16);
         }

         if (!io.MouseDown[0])
         {
            gContext.mbUsing = false;
            gContext.mScale.Set(1.f, 1.f, 1.f);
         }

         type = gContext.mCurrentOperation;
      }
      return modified;
   }

   static bool HandleRotation(float* matrix, float* deltaMatrix, OPERATION op, int& type, const float* snap)
   {
      if(!Intersects(op, ROTATE) || type != MT_NONE || !gContext.mbMouseOver)
      {
        return false;
      }
      ImGuiIO& io = ImGui::GetIO();
      bool applyRotationLocaly = gContext.mHandleSpace == LOCAL;
      bool modified = false;

      if (!gContext.mbUsing)
      {
         type = GetRotateType(op);

         if (type != MT_NONE)
         {
#if IMGUI_VERSION_NUM >= 18723
            ImGui::SetNextFrameWantCaptureMouse(true);
#else
            ImGui::CaptureMouseFromApp();
#endif
         }

         if (type == MT_ROTATE_SCREEN)
         {
            applyRotationLocaly = true;
         }

         if (CanActivate() && type != MT_NONE)
         {
            gContext.mbUsing = true;
            gContext.mEditingID = gContext.mActualID;
            gContext.mCurrentOperation = type;
            const vec_t rotatePlanNormal[] = { gContext.mModel.v.right, gContext.mModel.v.up, gContext.mModel.v.dir, -gContext.mCameraForward };
            // pickup plan
            if (applyRotationLocaly)
            {
               gContext.mTranslationPlan = BuildPlan(gContext.mModel.v.position, rotatePlanNormal[type - MT_ROTATE_X]);
            }
            else
            {
               gContext.mTranslationPlan = BuildPlan(gContext.mModelSource.v.position, directionUnary[type - MT_ROTATE_X]);
            }

            const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
            vec_t localPos = gContext.mRayOrigin + gContext.mRayVector * len - gContext.mModel.v.position;
            gContext.mRotationVectorSource = Normalized(localPos);
            gContext.mRotationAngleOrigin = ComputeAngleOnPlan();
         }
      }

      // rotation
      if (gContext.mbUsing && (gContext.mActualID == -1 || gContext.mActualID == gContext.mEditingID) && IsRotateType(gContext.mCurrentOperation))
      {
#if IMGUI_VERSION_NUM >= 18723
         ImGui::SetNextFrameWantCaptureMouse(true);
#else
         ImGui::CaptureMouseFromApp();
#endif
         gContext.mRotationAngle = ComputeAngleOnPlan();
         if (snap)
         {
            float snapInRadian = snap[0] * DEG_TO_RAD;
            ComputeSnap(&gContext.mRotationAngle, snapInRadian);
         }
         vec_t rotationAxisLocalSpace;

         rotationAxisLocalSpace.TransformVector(makeVect(gContext.mTranslationPlan.x, gContext.mTranslationPlan.y, gContext.mTranslationPlan.z, 0.f), gContext.mModelInverse);
         rotationAxisLocalSpace.Normalize();

         matrix_t deltaRotation;
         deltaRotation.RotationAxis(rotationAxisLocalSpace, gContext.mRotationAngle - gContext.mRotationAngleOrigin);
         if (gContext.mRotationAngle != gContext.mRotationAngleOrigin)
         {
            modified = true;
         }
         gContext.mRotationAngleOrigin = gContext.mRotationAngle;

         matrix_t scaleOrigin;
         scaleOrigin.Scale(gContext.mModelScaleOrigin);

         if (applyRotationLocaly)
         {
            *(matrix_t*)matrix = scaleOrigin * deltaRotation * gContext.mModelLocal;
         }
         else
         {
            matrix_t res = gContext.mModelSource;
            res.v.position.Set(0.f);

            *(matrix_t*)matrix = res * deltaRotation;
            ((matrix_t*)matrix)->v.position = gContext.mModelSource.v.position;
         }

         if (deltaMatrix)
         {
            *(matrix_t*)deltaMatrix = gContext.mModelInverse * deltaRotation * gContext.mModel;
         }

         if (!io.MouseDown[0])
         {
            gContext.mbUsing = false;
            gContext.mEditingID = -1;
         }
         type = gContext.mCurrentOperation;
      }
      return modified;
   }

   void DecomposeMatrixToComponents(const float* matrix, float* translation, float* rotation, float* scale)
   {
      matrix_t mat = *(matrix_t*)matrix;

      scale[0] = mat.v.right.Length();
      scale[1] = mat.v.up.Length();
      scale[2] = mat.v.dir.Length();

      mat.OrthoNormalize();

      rotation[0] = RAD_TO_DEG * atan2f(mat.m[1][2], mat.m[2][2]);
      rotation[1] = RAD_TO_DEG * atan2f(-mat.m[0][2], sqrtf(mat.m[1][2] * mat.m[1][2] + mat.m[2][2] * mat.m[2][2]));
      rotation[2] = RAD_TO_DEG * atan2f(mat.m[0][1], mat.m[0][0]);

      translation[0] = mat.v.position.x;
      translation[1] = mat.v.position.y;
      translation[2] = mat.v.position.z;
   }

   void RecomposeMatrixFromComponents(const float* translation, const float* rotation, const float* scale, float* matrix)
   {
      matrix_t& mat = *(matrix_t*)matrix;

      matrix_t rot[3];
      for (int i = 0; i < 3; i++)
      {
         rot[i].RotationAxis(directionUnary[i], rotation[i] * DEG_TO_RAD);
      }

      mat = rot[0] * rot[1] * rot[2];

      float validScale[3];
      for (int i = 0; i < 3; i++)
      {
         if (fabsf(scale[i]) < FLT_EPSILON)
         {
            validScale[i] = 0.001f;
         }
         else
         {
            validScale[i] = scale[i];
         }
      }
      mat.v.right *= validScale[0];
      mat.v.up *= validScale[1];
      mat.v.dir *= validScale[2];
      mat.v.position.Set(translation[0], translation[1], translation[2], 1.f);
   }

   void SetID(int id)
   {
      gContext.mActualID = id;
   }

   void AllowAxisFlip(bool value)
   {
     gContext.mAllowAxisFlip = value;
   }

   void SetAxisLimit(float value)
   {
     gContext.mAxisLimit=value;
   }

   void SetPlaneLimit(float value)
   {
     gContext.mPlaneLimit = value;
   }

   bool Manipulate(const float* view, const float* projection, OPERATION operation, MODE mode, float* matrix, float* deltaMatrix, const float* snap, const float* localBounds, const float* boundsSnap)
   {
      // Scale is always local or matrix will be skewed when applying world scale or oriented matrix
      ComputeContext(view, projection, matrix, (operation & SCALE) ? LOCAL : mode);

      // set delta to identity
      if (deltaMatrix)
      {
         ((matrix_t*)deltaMatrix)->SetToIdentity();
      }

      // behind camera
      vec_t camSpacePosition;
      camSpacePosition.TransformPoint(makeVect(0.f, 0.f, 0.f), gContext.mMVP);
      if (!gContext.mIsOrthographic && camSpacePosition.z < 0.001f)
      {
         return false;
      }

      // --
      int type = MT_NONE;
      bool manipulated = false;
      if (gContext.mbEnable)
      {
         if (!gContext.mbUsingBounds)
         {
            manipulated = HandleTranslation(matrix, deltaMatrix, operation, type, snap) ||
                          HandleScale(matrix, deltaMatrix, operation, type, snap) ||
                          HandleRotation(matrix, deltaMatrix, operation, type, snap);
         }
      }

      if (localBounds && !gContext.mbUsing)
      {
         HandleAndDrawLocalBounds(localBounds, (matrix_t*)matrix, boundsSnap, operation);
      }

      gContext.mOperation = operation;
      if (!gContext.mbUsingBounds)
      {
         DrawRotationGizmo(operation, type);
         DrawTranslationGizmo(operation, type);
         DrawScaleGizmo(operation, type);
         DrawScaleUniveralGizmo(operation, type);
      }
      return manipulated;
   }

   void SetGizmoSizeClipSpace(float value)
   {
      gContext.mGizmoSizeClipSpace = value;
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   void ComputeFrustumPlanes(vec_t* frustum, const float* clip)
   {
      frustum[0].x = clip[3] - clip[0];
      frustum[0].y = clip[7] - clip[4];
      frustum[0].z = clip[11] - clip[8];
      frustum[0].w = clip[15] - clip[12];

      frustum[1].x = clip[3] + clip[0];
      frustum[1].y = clip[7] + clip[4];
      frustum[1].z = clip[11] + clip[8];
      frustum[1].w = clip[15] + clip[12];

      frustum[2].x = clip[3] + clip[1];
      frustum[2].y = clip[7] + clip[5];
      frustum[2].z = clip[11] + clip[9];
      frustum[2].w = clip[15] + clip[13];

      frustum[3].x = clip[3] - clip[1];
      frustum[3].y = clip[7] - clip[5];
      frustum[3].z = clip[11] - clip[9];
      frustum[3].w = clip[15] - clip[13];

      frustum[4].x = clip[3] - clip[2];
      frustum[4].y = clip[7] - clip[6];
      frustum[4].z = clip[11] - clip[10];
      frustum[4].w = clip[15] - clip[14];

      frustum[5].x = clip[3] + clip[2];
      frustum[5].y = clip[7] + clip[6];
      frustum[5].z = clip[11] + clip[10];
      frustum[5].w = clip[15] + clip[14];

      for (int i = 0; i < 6; i++)
      {
         frustum[i].Normalize();
      }
   }

   void DrawCubes(const float* view, const float* projection, const float* matrices, int matrixCount)
   {
      matrix_t viewInverse;
      viewInverse.Inverse(*(matrix_t*)view);

      struct CubeFace
      {
         float z;
         ImVec2 faceCoordsScreen[4];
         ImU32 color;
      };
      CubeFace* faces = (CubeFace*)_malloca(sizeof(CubeFace) * matrixCount * 6);

      if (!faces)
      {
         return;
      }

      vec_t frustum[6];
      matrix_t viewProjection = *(matrix_t*)view * *(matrix_t*)projection;
      ComputeFrustumPlanes(frustum, viewProjection.m16);

      int cubeFaceCount = 0;
      for (int cube = 0; cube < matrixCount; cube++)
      {
         const float* matrix = &matrices[cube * 16];

         matrix_t res = *(matrix_t*)matrix * *(matrix_t*)view * *(matrix_t*)projection;

         for (int iFace = 0; iFace < 6; iFace++)
         {
            const int normalIndex = (iFace % 3);
            const int perpXIndex = (normalIndex + 1) % 3;
            const int perpYIndex = (normalIndex + 2) % 3;
            const float invert = (iFace > 2) ? -1.f : 1.f;

            const vec_t faceCoords[4] = { directionUnary[normalIndex] + directionUnary[perpXIndex] + directionUnary[perpYIndex],
               directionUnary[normalIndex] + directionUnary[perpXIndex] - directionUnary[perpYIndex],
               directionUnary[normalIndex] - directionUnary[perpXIndex] - directionUnary[perpYIndex],
               directionUnary[normalIndex] - directionUnary[perpXIndex] + directionUnary[perpYIndex],
            };

            // clipping
            /*
            bool skipFace = false;
            for (unsigned int iCoord = 0; iCoord < 4; iCoord++)
            {
               vec_t camSpacePosition;
               camSpacePosition.TransformPoint(faceCoords[iCoord] * 0.5f * invert, res);
               if (camSpacePosition.z < 0.001f)
               {
                  skipFace = true;
                  break;
               }
            }
            if (skipFace)
            {
               continue;
            }
            */
            vec_t centerPosition, centerPositionVP;
            centerPosition.TransformPoint(directionUnary[normalIndex] * 0.5f * invert, *(matrix_t*)matrix);
            centerPositionVP.TransformPoint(directionUnary[normalIndex] * 0.5f * invert, res);

            bool inFrustum = true;
            for (int iFrustum = 0; iFrustum < 6; iFrustum++)
            {
               float dist = DistanceToPlane(centerPosition, frustum[iFrustum]);
               if (dist < 0.f)
               {
                  inFrustum = false;
                  break;
               }
            }

            if (!inFrustum)
            {
               continue;
            }
            CubeFace& cubeFace = faces[cubeFaceCount];

            // 3D->2D
            //ImVec2 faceCoordsScreen[4];
            for (unsigned int iCoord = 0; iCoord < 4; iCoord++)
            {
               cubeFace.faceCoordsScreen[iCoord] = worldToPos(faceCoords[iCoord] * 0.5f * invert, res);
            }

            ImU32 directionColor = GetColorU32(DIRECTION_X + normalIndex);
            cubeFace.color = directionColor | IM_COL32(0x80, 0x80, 0x80, 0);

            cubeFace.z = centerPositionVP.z / centerPositionVP.w;
            cubeFaceCount++;
         }
      }
      qsort(faces, cubeFaceCount, sizeof(CubeFace), [](void const* _a, void const* _b) {
         CubeFace* a = (CubeFace*)_a;
         CubeFace* b = (CubeFace*)_b;
         if (a->z < b->z)
         {
            return 1;
         }
         return -1;
         });
      // draw face with lighter color
      for (int iFace = 0; iFace < cubeFaceCount; iFace++)
      {
         const CubeFace& cubeFace = faces[iFace];
         gContext.mDrawList->AddConvexPolyFilled(cubeFace.faceCoordsScreen, 4, cubeFace.color);
      }

      _freea(faces);
   }

   void DrawGrid(const float* view, const float* projection, const float* matrix, const float gridSize)
   {
      matrix_t viewProjection = *(matrix_t*)view * *(matrix_t*)projection;
      vec_t frustum[6];
      ComputeFrustumPlanes(frustum, viewProjection.m16);
      matrix_t res = *(matrix_t*)matrix * viewProjection;

      for (float f = -gridSize; f <= gridSize; f += 1.f)
      {
         for (int dir = 0; dir < 2; dir++)
         {
            vec_t ptA = makeVect(dir ? -gridSize : f, 0.f, dir ? f : -gridSize);
            vec_t ptB = makeVect(dir ? gridSize : f, 0.f, dir ? f : gridSize);
            bool visible = true;
            for (int i = 0; i < 6; i++)
            {
               float dA = DistanceToPlane(ptA, frustum[i]);
               float dB = DistanceToPlane(ptB, frustum[i]);
               if (dA < 0.f && dB < 0.f)
               {
                  visible = false;
                  break;
               }
               if (dA > 0.f && dB > 0.f)
               {
                  continue;
               }
               if (dA < 0.f)
               {
                  float len = fabsf(dA - dB);
                  float t = fabsf(dA) / len;
                  ptA.Lerp(ptB, t);
               }
               if (dB < 0.f)
               {
                  float len = fabsf(dB - dA);
                  float t = fabsf(dB) / len;
                  ptB.Lerp(ptA, t);
               }
            }
            if (visible)
            {
               ImU32 col = IM_COL32(0x80, 0x80, 0x80, 0xFF);
               col = (fmodf(fabsf(f), 10.f) < FLT_EPSILON) ? IM_COL32(0x90, 0x90, 0x90, 0xFF) : col;
               col = (fabsf(f) < FLT_EPSILON) ? IM_COL32(0x40, 0x40, 0x40, 0xFF): col;

               float thickness = 1.f;
               thickness = (fmodf(fabsf(f), 10.f) < FLT_EPSILON) ? 1.5f : thickness;
               thickness = (fabsf(f) < FLT_EPSILON) ? 2.3f : thickness;

               gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
            }
         }
      }
   }

   void ViewManipulate(float* view, const float* projection, OPERATION operation, MODE mode, float* matrix, float length, ImVec2 position, ImVec2 size, ImU32 backgroundColor)
   {
      // Scale is always local or matrix will be skewed when applying world scale or oriented matrix
      ComputeContext(view, projection, matrix, (operation & SCALE) ? LOCAL : mode);
      ViewManipulate(view, length, position, size, backgroundColor);
   }

   void ViewManipulate(float* view, float length, ImVec2 position, ImVec2 size, ImU32 backgroundColor)
   {
      static bool isDraging = false;
      static bool isClicking = false;
      static bool isInside = false;
      static vec_t interpolationUp;
      static vec_t interpolationDir;
      static int interpolationFrames = 0;
      const vec_t referenceUp = makeVect(0.f, 1.f, 0.f);

      matrix_t svgView, svgProjection;
      svgView = gContext.mViewMat;
      svgProjection = gContext.mProjectionMat;

      ImGuiIO& io = ImGui::GetIO();
      gContext.mDrawList->AddRectFilled(position, position + size, backgroundColor);
      matrix_t viewInverse;
      viewInverse.Inverse(*(matrix_t*)view);

      const vec_t camTarget = viewInverse.v.position - viewInverse.v.dir * length;

      // view/projection matrices
      const float distance = 3.f;
      matrix_t cubeProjection, cubeView;
      float fov = acosf(distance / (sqrtf(distance * distance + 3.f))) * RAD_TO_DEG;
      Perspective(fov / sqrtf(2.f), size.x / size.y, 0.01f, 1000.f, cubeProjection.m16);

      vec_t dir = makeVect(viewInverse.m[2][0], viewInverse.m[2][1], viewInverse.m[2][2]);
      vec_t up = makeVect(viewInverse.m[1][0], viewInverse.m[1][1], viewInverse.m[1][2]);
      vec_t eye = dir * distance;
      vec_t zero = makeVect(0.f, 0.f);
      LookAt(&eye.x, &zero.x, &up.x, cubeView.m16);

      // set context
      gContext.mViewMat = cubeView;
      gContext.mProjectionMat = cubeProjection;
      ComputeCameraRay(gContext.mRayOrigin, gContext.mRayVector, position, size);

      const matrix_t res = cubeView * cubeProjection;

      // panels
      static const ImVec2 panelPosition[9] = { ImVec2(0.75f,0.75f), ImVec2(0.25f, 0.75f), ImVec2(0.f, 0.75f),
         ImVec2(0.75f, 0.25f), ImVec2(0.25f, 0.25f), ImVec2(0.f, 0.25f),
         ImVec2(0.75f, 0.f), ImVec2(0.25f, 0.f), ImVec2(0.f, 0.f) };

      static const ImVec2 panelSize[9] = { ImVec2(0.25f,0.25f), ImVec2(0.5f, 0.25f), ImVec2(0.25f, 0.25f),
         ImVec2(0.25f, 0.5f), ImVec2(0.5f, 0.5f), ImVec2(0.25f, 0.5f),
         ImVec2(0.25f, 0.25f), ImVec2(0.5f, 0.25f), ImVec2(0.25f, 0.25f) };

      // tag faces
      bool boxes[27]{};
      static int overBox = -1;
      for (int iPass = 0; iPass < 2; iPass++)
      {
         for (int iFace = 0; iFace < 6; iFace++)
         {
            const int normalIndex = (iFace % 3);
            const int perpXIndex = (normalIndex + 1) % 3;
            const int perpYIndex = (normalIndex + 2) % 3;
            const float invert = (iFace > 2) ? -1.f : 1.f;
            const vec_t indexVectorX = directionUnary[perpXIndex] * invert;
            const vec_t indexVectorY = directionUnary[perpYIndex] * invert;
            const vec_t boxOrigin = directionUnary[normalIndex] * -invert - indexVectorX - indexVectorY;

            // plan local space
            const vec_t n = directionUnary[normalIndex] * invert;
            vec_t viewSpaceNormal = n;
            vec_t viewSpacePoint = n * 0.5f;
            viewSpaceNormal.TransformVector(cubeView);
            viewSpaceNormal.Normalize();
            viewSpacePoint.TransformPoint(cubeView);
            const vec_t viewSpaceFacePlan = BuildPlan(viewSpacePoint, viewSpaceNormal);

            // back face culling
            if (viewSpaceFacePlan.w > 0.f)
            {
               continue;
            }

            const vec_t facePlan = BuildPlan(n * 0.5f, n);

            const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, facePlan);
            vec_t posOnPlan = gContext.mRayOrigin + gContext.mRayVector * len - (n * 0.5f);

            float localx = Dot(directionUnary[perpXIndex], posOnPlan) * invert + 0.5f;
            float localy = Dot(directionUnary[perpYIndex], posOnPlan) * invert + 0.5f;

            // panels
            const vec_t dx = directionUnary[perpXIndex];
            const vec_t dy = directionUnary[perpYIndex];
            const vec_t origin = directionUnary[normalIndex] - dx - dy;
            for (int iPanel = 0; iPanel < 9; iPanel++)
            {
               vec_t boxCoord = boxOrigin + indexVectorX * float(iPanel % 3) + indexVectorY * float(iPanel / 3) + makeVect(1.f, 1.f, 1.f);
               const ImVec2 p = panelPosition[iPanel] * 2.f;
               const ImVec2 s = panelSize[iPanel] * 2.f;
               ImVec2 faceCoordsScreen[4];
               vec_t panelPos[4] = { dx * p.x + dy * p.y,
                                     dx * p.x + dy * (p.y + s.y),
                                     dx * (p.x + s.x) + dy * (p.y + s.y),
                                     dx * (p.x + s.x) + dy * p.y };

               for (unsigned int iCoord = 0; iCoord < 4; iCoord++)
               {
                  faceCoordsScreen[iCoord] = worldToPos((panelPos[iCoord] + origin) * 0.5f * invert, res, position, size);
               }

               const ImVec2 panelCorners[2] = { panelPosition[iPanel], panelPosition[iPanel] + panelSize[iPanel] };
               bool insidePanel = localx > panelCorners[0].x && localx < panelCorners[1].x && localy > panelCorners[0].y && localy < panelCorners[1].y;
               int boxCoordInt = int(boxCoord.x * 9.f + boxCoord.y * 3.f + boxCoord.z);
               IM_ASSERT(boxCoordInt < 27);
               boxes[boxCoordInt] |= insidePanel && (!isDraging) && gContext.mbMouseOver;

               // draw face with lighter color
               if (iPass)
               {
                  ImU32 directionColor = GetColorU32(DIRECTION_X + normalIndex);
                  gContext.mDrawList->AddConvexPolyFilled(faceCoordsScreen, 4, (directionColor | IM_COL32(0x80, 0x80, 0x80, 0x80)) | (isInside ? IM_COL32(0x08, 0x08, 0x08, 0) : 0));
                  if (boxes[boxCoordInt])
                  {
                     gContext.mDrawList->AddConvexPolyFilled(faceCoordsScreen, 4, IM_COL32(0xF0, 0xA0, 0x60, 0x80));

                     if (io.MouseDown[0] && !isClicking && !isDraging && GImGui->ActiveId == 0) {
                        overBox = boxCoordInt;
                        isClicking = true;
                        isDraging = true;
                     }
                  }
               }
            }
         }
      }
      if (interpolationFrames)
      {
         interpolationFrames--;
         vec_t newDir = viewInverse.v.dir;
         newDir.Lerp(interpolationDir, 0.2f);
         newDir.Normalize();

         vec_t newUp = viewInverse.v.up;
         newUp.Lerp(interpolationUp, 0.3f);
         newUp.Normalize();
         newUp = interpolationUp;
         vec_t newEye = camTarget + newDir * length;
         LookAt(&newEye.x, &camTarget.x, &newUp.x, view);
      }
      isInside = gContext.mbMouseOver && ImRect(position, position + size).Contains(io.MousePos);

      if (io.MouseDown[0] && (fabsf(io.MouseDelta[0]) || fabsf(io.MouseDelta[1])) && isClicking)
      {
         isClicking = false;
      }

      if (!io.MouseDown[0])
      {
         if (isClicking)
         {
            // apply new view direction
            int cx = overBox / 9;
            int cy = (overBox - cx * 9) / 3;
            int cz = overBox % 3;
            interpolationDir = makeVect(1.f - (float)cx, 1.f - (float)cy, 1.f - (float)cz);
            interpolationDir.Normalize();

            if (fabsf(Dot(interpolationDir, referenceUp)) > 1.0f - 0.01f)
            {
               vec_t right = viewInverse.v.right;
               if (fabsf(right.x) > fabsf(right.z))
               {
                  right.z = 0.f;
               }
               else
               {
                  right.x = 0.f;
               }
               right.Normalize();
               interpolationUp = Cross(interpolationDir, right);
               interpolationUp.Normalize();
            }
            else
            {
               interpolationUp = referenceUp;
            }
            interpolationFrames = 40;
            
         }
         isClicking = false;
         isDraging = false;
      }


      if (isDraging)
      {
         matrix_t rx, ry, roll;

         rx.RotationAxis(referenceUp, -io.MouseDelta.x * 0.01f);
         ry.RotationAxis(viewInverse.v.right, -io.MouseDelta.y * 0.01f);

         roll = rx * ry;

         vec_t newDir = viewInverse.v.dir;
         newDir.TransformVector(roll);
         newDir.Normalize();

         // clamp
         vec_t planDir = Cross(viewInverse.v.right, referenceUp);
         planDir.y = 0.f;
         planDir.Normalize();
         float dt = Dot(planDir, newDir);
         if (dt < 0.0f)
         {
            newDir += planDir * dt;
            newDir.Normalize();
         }

         vec_t newEye = camTarget + newDir * length;
         LookAt(&newEye.x, &camTarget.x, &referenceUp.x, view);
      }

      // restore view/projection because it was used to compute ray
      ComputeContext(svgView.m16, svgProjection.m16, gContext.mModelSource.m16, gContext.mHandleSpace);
   }
};



#endif //Source