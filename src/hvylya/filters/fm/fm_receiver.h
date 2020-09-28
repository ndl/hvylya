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

#include <hvylya/filters/composite_filter_generic.h>
#include <hvylya/filters/fm/rds_decoding_stats.h>
#include <hvylya/filters/fm/rds_state.h>

namespace hvylya {
namespace filters {

struct RdsState;
struct RdsDecodingStats;

namespace fm {

template <typename T>
class FmReceiver:
    public CompositeFilterGeneric<
        core::TypeList<std::complex<T>>,
        core::TypeList<T, T>
    >
{
  public:
    FmReceiver();

    virtual ~FmReceiver() override;

    virtual Channel inputChannel(std::size_t input_index) override;

    virtual Channel outputChannel(std::size_t output_index) override;

    std::size_t inputSamplingRate() const;

    std::size_t outputAudioSamplingRate() const;

    void setStereoWeight(T stereo_weight);

    RdsState rdsState() const;

    RdsDecodingStats rdsDecodingStats() const;

    T pilotSnr() const;

    T monoSnr() const;

    T stereoSnr() const;

    T rdsSnr() const;

  private:
    struct FmReceiverImpl;
    std::unique_ptr<FmReceiverImpl> pimpl_;
};

} // namespace fm
} // namespace filters
} // namespace hvylya
