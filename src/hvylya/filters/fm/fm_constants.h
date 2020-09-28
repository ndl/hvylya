// Hvylya - software-defined radio framework, see https://endl.ch/projects/hvylya
//
// Copyright (C) 2019 - 2020 Alexander Tsvyashchenko <sdr@endl.ch>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// THIS IS AUTO-GENERATED FILE, DO NOT EDIT!

#pragma once

#include <hvylya/core/common.h>

namespace hvylya {
namespace filters {
namespace fm {

static const std::size_t InputSamplingRate = 1000000;
static const std::size_t IntermediateSamplingRate = 250000;
static const std::size_t IntermediateAudioSamplingRate = 50000;
static const std::size_t OutputAudioSamplingRate = 48000;
static const double RdsSymbolRate = 1187.5;

static const std::size_t IntermediateDecimationRatio = 4;
static const std::size_t AudioDecimationRatio = 5;
static const std::size_t AudioResamplerInterpolationRatio = 24;
static const std::size_t AudioResamplerDecimationRatio = 25;

static const double StereoPilotFrequency = 19000;
static const double NoiseExtractorFrequency = 65000;

static const double FmChannelBandwidth = 120000;
static const double FmChannelStopband = 130000;
static const double StereoPilotBandwidth = 200;
static const double StereoPilotStopband = 2200;
static const double AudioBandwidth = 15000;
static const double AudioStopband = 17000;
static const double RdsBandwidth = 2400;
static const double RdsStopband = 3900;
static const double NoiseExtractorBandwidth = 2000;
static const double NoiseExtractorStopband = 5000;

static const std::size_t FmBasebandTapsCount = 473; 
extern const float FmBasebandTaps[FmBasebandTapsCount];

static const std::size_t FmDemodDecimatorTapsCount = 65; 
extern const float FmDemodDecimatorTaps[FmDemodDecimatorTapsCount];

static const std::size_t StereoPilotBandpassTapsCount = 401; 
extern const std::complex<float> StereoPilotBandpassTaps[StereoPilotBandpassTapsCount];

static const std::size_t StereoBandpassTapsCount = 433; 
extern const float StereoBandpassTaps[StereoBandpassTapsCount];

static const std::size_t MonoDecimatorTapsCount = 433; 
extern const float MonoDecimatorTaps[MonoDecimatorTapsCount];

static const std::size_t StereoDecimatorTapsCount = 97; 
extern const float StereoDecimatorTaps[StereoDecimatorTapsCount];

static const std::size_t AudioResamplerTapsCount = 457; 
extern const float AudioResamplerTaps[AudioResamplerTapsCount];

static const std::size_t RdsBandpassTapsCount = 593; 
extern const float RdsBandpassTaps[RdsBandpassTapsCount];

static const std::size_t RdsDemodulatedTapsCount = 593; 
extern const float RdsDemodulatedTaps[RdsDemodulatedTapsCount];

static const std::size_t RdsSymbolShapeTapsCount = 422; 
extern const float RdsSymbolShapeTaps[RdsSymbolShapeTapsCount];

static const std::size_t NoiseExtractorTapsCount = 305; 
extern const float NoiseExtractorTaps[NoiseExtractorTapsCount];


} // namespace fm
} // namespace filters
} // namespace hvylya
