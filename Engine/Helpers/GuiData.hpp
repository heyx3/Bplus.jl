#pragma once

#include "../Dependencies.h"
#include "../Math/Math.hpp"


//Defines data structures that are useful for building a GUI,
//    like sliders and slider/textboxes.
//Also provides Dear IMGUI helpers to use these structures.

namespace Bplus::GuiData
{
    //The different ways that a slider can be combined with a textbox.
    BETTER_ENUM(SliderTextboxModes, uint8_t,
        Off = 0,
        ClampToSlider = 1,
        Unbounded = 2
    );

    //Data for a slider.
    template<typename Number_t, typename Float_t = double> //Not much harm in using maximum precision
                                                           //  for GUI sliders that do so much exponential math.
    struct SliderRange
    {
        Number_t Min = (Number_t)0,
                 Max = (Number_t)1;

        //An exponent which curves the slider range to provide finer control
        //    within a certain part of the range between Min and Max.
        Float_t Power = Float_t{1.0},
                //From 0 - 1, this provides the focal point on the slider
                //   that the Power field is centered around.
                PowerMidpoint = Float_t{0};

        SliderTextboxModes TextboxMode = SliderTextboxModes::Off;


        //Gets the 'T' value from 0 to 1, representing this number's slider position.
        inline Float_t GetT(Number_t value) const
        {
            //Get the un-biased interpolant first.
            Float_t t = Math::InverseLerp<Number_t, Float_t>(Min, Max, value);

            //Get how far away it is from the power "midpoint", from 0 to 1.
            Float_t powerMidDistance = t - PowerMidpoint;
            auto signToPowerMid = (int_fast8_t)glm::sign(powerMidDistance);
            if (signToPowerMid == 0)
                signToPowerMid = 1;
            Float_t maxPowerDistance = (signToPowerMid > 0) ?
                                           (Float_t{1} - PowerMidpoint) :
                                           PowerMidpoint;
            Float_t powerMidDistanceT = glm::abs(powerMidDistance) / maxPowerDistance;

            //Scale the 0-1 distance from the "midpoint" with the "Power" exponent.
            //Then do the math in reverse to get the new scaled 't' value.
            Float_t scaledPowerMidDistanceT = glm::pow(powerMidDistanceT, Power);
            t = PowerMidpoint + (scaledPowerMidDistanceT * maxPowerDistance *
                                 signToPowerMid);
            
            return t;
        }
        //Get the value for this slider at the given position (from 0 to 1).
        inline Number_t GetValue(Float_t t) const
        {
            Float_t distFromMidpoint = t - PowerMidpoint;
            
            //Calculte the direction from t to PowerMidpoint,
            //    as well as the largest-possible magnitude it could have.
            auto signToPowerMid = (int_fast8_t)glm::sign(powerMidDistance);
            if (signToPowerMid == 0)
                signToPowerMid = 1;
            Float_t maxPowerDistance = (signToPowerMid > 0) ?
                                           (Float_t{1} - PowerMidpoint) :
                                           PowerMidpoint;
            Float_t powerMidDistanceT = glm::abs(distFromMidpoint) / maxPowerDistance;

            //Un-scale 't' using the inverse of 'Power'.
            Float_t unscaledPowerMidDistanceT = glm::pow(powerMidDistanceT,
                                                         Float_t{1} / Power);
            t = PowerMidpoint + (unscaledPowerMidDistanceT * maxPowerDistance *
                                 signToPowerMid);

            //Calculate the slider's value.
            Float_t valF = glm::lerp((Float_t)Min, (Float_t)Max, t);
            if constexpr (std::is_integral_v<Number_t>)
            {
                valF = glm::round(valF);
            }
            return (Number_t)valF;
        }
    };
    

    //Data for any kind of clamped number/vector value.
    template<typename Number_t>
    struct ValueRange
    {
        std::optional<Number_b> Min = 0,
                                Max = std::nullopt;

        inline Number_t Apply(Number_t input) const
        {
            if (Min.has_value())
                input = glm::max(*Min, input);
            if (Max.has_value())
                input = glm::min(*Max, input);

            return input;
        }
    };

    //Represents the settings for a vector field,
    //    which may be nonexistent, per-component, or global.
    template<typename Number_t, typename Float_t = double>
    using VectorChannelDataRange = std::variant<std::nullopt_t,
                                                GuiData::SliderRange<Number_t, Float_t>,
                                                GuiData::ValueRange<Number_t>>;
    template<typename Number_t, typename Float_t = double>
    using VectorDataRange = std::variant<VectorChannelDataRange<Number_t, Float_t>,
                                         std::array<VectorChannelDataRange<Number_t, Float_t>,
                                                    4>>;

    template<glm::length_t D,  //D is the dimensionality of the data (1D to 4D).
             typename Channel_t, //Channel_t is the type of each "channel" in the data.
             typename Float_t = Math::AppropriateFloat_t<Channel_t>> //Float_t is the floating-point type used for internal math/interpolation.
    struct Curve
    {
        using Value_t = glm::vec<D, Channel_t>;

        struct Key
        {
            Float_t Pos;
            Value_t Value;
            //A tangent direction of 1 means to point towards the next value.
            //A tangent direction of 0 means to point towards "no change" at all.
            //A tangent direction of -1 means to point away from the next value.
            Value_t InTangentDir = 1,
                    OutTangentDir = 1;
            //Tangent strength affects the influence of the above "tangent direction" fields.
            Value_t InTangentStrength = 0.5f,
                    OutTangentStrength = 0.5f;
        };

        //Assumed to be in order by their position.
        std::vector<Key> Keys;
        //The default value if no Keys exist in this curve.
        Value_t Default = { 0 };

        Curve(Value_t constant)
            : Keys({ { 0, constant } })
        {

        }
        Curve(Value_t start, Value_t end, Float_t slope = 1, Float_t range = 1)
            : Keys({ { (Float_t)0, start, 0,                             slope * (end - start) / range,   0.5f, 0.5f },
                     { range,      end,   slope * (start - end) / range, 0,                               0.5f, 0.5f } })
        {

        }
        Curve(std::vector<Key>&& keys) : Keys(std::move(keys)) { }
        Curve(Key* keys, size_t nKeys) : Keys(keys, keys + nKeys) { }

        //Gets the range of this curve, based on its keys.
        //If there are no keys, returns an "empty" range.
        Math::Interval<Float_t> GetRange() const
        {
            if (Keys.size() > 0)
                return Math::Interval<Float_t>::MinMaxInclusive(Keys[0].Pos, Keys.back().Pos);
            else
                return Math::Interval<Float_t>();
        }

        //Gets the value of this curve at the given time.
        Value_t Get(Float_t t) const
        {
            if (Keys.size() > 0)
            {
                //Edge-case: the 't' value is behind the first key.
                if (t < Keys[0].Pos)
                    return Keys[0].Value; //TOOD: Optionally extrapolate backward.

                //Find the key immediately before this T value.
                size_t prevKey = 0;
                while (prevKey < Keys.size() - 1 && Keys[prevKey].Pos >= t)
                    prevKey += 1;

                //Edge-case: the 't' value is after the last key.
                if (prevKey >= Keys.size() - 1)
                    return Keys.back().Value; //TODO: Optionally extrapolate forward.

                const auto& key1 = Keys[prevKey];
                const auto& key2 = Keys[prevKey + 1];

                //Edge-case: the keys share the same position.
                if (key1.Pos == key2.Pos)
                    return Value_t{ (key1.Value + key2.Value) / 2 };

                Float_t keyT = Math::InverseLerp(key1.Pos, key2.Pos, t);
                //Model the movement between key1 and key2 using
                //    a (N+1)-dimensional Bezier curve.
                //TODO: Finish.
            }
            else
            {
                return Default;
            }
        }
    };
}

//TODO: Toml IO helpers.
//TODO: Unit tests for slider math.

//Make some of the above types hashable.
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GuiData::SliderTextboxModes);