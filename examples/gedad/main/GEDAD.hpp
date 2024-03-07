
#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <forward_list>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

namespace ad {

using namespace std;

enum class AnomalyType { Unknown, Normal, Anomaly };

template <typename DataType, typename DistType = float, size_t Channels = 3u> class GEDAD final {
   public:
    GEDAD(size_t buffer_size)
    noexcept
        : _buffer_flag(false),
          _buffer_cidx(0),
          _buffer_size(buffer_size),
          _window_start(0),
          _window_size(buffer_size),
          _view_size(0),
          _shift_dist(1),
          _minimal_n(0),
          _bind_channels(false),
          _buffer(),
          _window(),
          _euclidean_dist_thresh() {
        // pre-allocate buffer
        for (auto& c : _buffer) {
            c.resize(buffer_size);
        }
    }

    ~GEDAD() = default;

   public:
    void printBuffer() const {
        size_t i = 0;
        cout << "buffer:\n" << fixed;
        for (const auto& c : _buffer) {
            cout << "  " << i++ << ": ";
            for (const auto& d : c) {
                cout << d << " ";
            }
            cout << endl;
        }
    }

    void printWindow() const {
        size_t i = 0;
        cout << "window:\n" << fixed;
        for (const auto& c : _window) {
            cout << "  " << i++ << ": ";
            for (const auto& d : c) {
                cout << d << " ";
            }
            cout << endl;
        }
    }

    inline bool pushToBuffer(DataType channel_data[Channels]) {
        if (_buffer_flag) {
            return false;
        }
        _buffer_flag = true;
        for (size_t i = 0; i < Channels; ++i) {
            _buffer[i][_buffer_cidx % _buffer[i].size()] = channel_data[i];
        }
        _buffer_cidx = _buffer_cidx + 1;
        _buffer_flag = false;
        return true;
    }

    void calEuclideanDistThresh(size_t window_start,
                                size_t window_size,
                                size_t sample_start,
                                size_t sample_size,
                                size_t view_size,
                                size_t batch_size,
                                size_t shift_dist    = 1,
                                size_t minimal_n     = 10,
                                bool   bind_channels = false) {
        // verify and assign parameters to member variables
        {
            assert(window_size >= 1);
            assert(sample_size >= 1);
            assert(view_size >= 1);
            assert(batch_size >= 1);
            assert(shift_dist >= 1);
            assert(window_start + window_size <= _buffer_size);
            assert(sample_start + sample_size <= _buffer_size);
            assert(view_size * batch_size - (view_size - shift_dist) * (batch_size - 1) <= sample_size);
            _window_start  = window_start;
            _window_size   = window_size;
            _view_size     = view_size;
            _shift_dist    = shift_dist;
            _minimal_n     = minimal_n;
            _bind_channels = bind_channels;
        }

        // release the window buffer
        {
            for (auto& c : _window) {
                c.clear();
                c.shrink_to_fit();
            }
        }

        // from range sample_start to (sample_start + sample_size - view_size)
        // randomly select batch_size views, each view has view_size elements
        vector<size_t> view_index = geneRandViewIndex(sample_start, sample_start + sample_size - view_size, batch_size);
        {
            // debug print view_index
            cout << "view index:\n  " << fixed;
            for (const auto& i : view_index) {
                cout << "(" << i << "," << i + view_size << ") ";
            }
            cout << endl;
        }

        // calculate the euclidean distance for each channel
        // TODO: support bind_channels
        array<vector<DistType>, Channels> euclidean_dist_per_channel;
        {
            size_t window_end        = window_start + window_size - view_size;
            size_t views_per_channel = (window_end - window_start) / shift_dist;
            size_t buffer_size       = _buffer.size();
            for (size_t i = 0; i < buffer_size; ++i) {
                // for each view, sum the total euclidean distance
                // by sliding the view with shift_dist on the channel buffer
                const auto       buffer_i = _buffer[i];
                vector<DistType> euclidean_dist_per_view;
                euclidean_dist_per_view.reserve(view_index.size());
                for (const auto& j : view_index) {
                    vector<DistType> euclidean_dist_per_element;
                    euclidean_dist_per_element.reserve(views_per_channel);
                    // slide view on the channel buffer
                    for (size_t k = window_start; k < window_end; k += shift_dist) {
                        // calculate the euclidean distance
                        DistType euclidean_dist{};
                        for (size_t l = 0; l < view_size; ++l) {
                            euclidean_dist += pow(buffer_i[j + l] - buffer_i[k + l], 2);
                        }
                        euclidean_dist_per_element.emplace_back(euclidean_dist);  // skip sqrt for performance
                    }
                    // sort and take minimal_n euclidean distances
                    sort(begin(euclidean_dist_per_element), end(euclidean_dist_per_element), less<DistType>{});
                    euclidean_dist_per_element.resize(minimal_n);
                    euclidean_dist_per_element.shrink_to_fit();
                    // calculate the average of the minimal_n euclidean distances
                    euclidean_dist_per_view.emplace_back(
                      accumulate(begin(euclidean_dist_per_element), end(euclidean_dist_per_element), DistType{}) /
                      static_cast<DistType>(minimal_n));
                }
                euclidean_dist_per_channel[i] = move(euclidean_dist_per_view);
            }

            // debug print euclidean_dist_per_channel
            size_t i = 0;
            cout << "euclidean distance per channel: " << endl;
            for (const auto& dists : euclidean_dist_per_channel) {
                cout << "  " << ++i << ": ";
                for (const auto& d : dists) {
                    cout << d << " ";
                }
                cout << endl;
            }
        }

        // calculate and assign the final euclidean distance threshold
        {
            for (size_t i = 0; i < Channels; ++i) {
                _euclidean_dist_thresh[i] =
                  accumulate(begin(euclidean_dist_per_channel[i]), end(euclidean_dist_per_channel[i]), DistType{}) /
                  static_cast<DistType>(batch_size);

                _euclidean_dist_thresh[i] *= 1.5f;
            }

            // debug print euclidean_dist_thresh
            cout << "euclidean distance threshold:\n";
            for (size_t i = 0; i < _euclidean_dist_thresh.size(); ++i) {
                cout << "  " << i << ": " << _euclidean_dist_thresh[i] << "\n";
            }
        }

        // assign the window buffer again
        {
            for (size_t i = 0; i < _buffer.size(); ++i) {
                _window[i].resize(_window_size);
                auto it = begin(_buffer[i]) + _window_start;
                copy(it, it + _window_size, begin(_window[i]));
            }

            // debug print window buffer
            printWindow();
        }
    }

    inline AnomalyType isLastViewAnomaly() const {
        // take last view_size elements from the _buffer (ring buffer)
        // copy them to the _cached_view
        if (_buffer_flag) {
            return AnomalyType::Unknown;
        }
        _buffer_flag          = true;
        size_t view_start_idx = (_buffer_cidx - _view_size) % _buffer_size;
        for (size_t i = 0; i < Channels; ++i) {
            auto& view_per_channel_i = _cached_view[i];
            // check cache buffer size, resize if necessary
            if (view_per_channel_i.size() != _view_size) [[unlikely]] {
                view_per_channel_i.resize(_view_size);
                view_per_channel_i.shrink_to_fit();
            }
            for (size_t j = 0; j < _view_size; ++j) {
                view_per_channel_i[j] = _buffer[i][(view_start_idx + j) % _buffer_size];
            }
        }
        _buffer_flag = false;

        // debug print cached view
        // cout << "cached view:\n";
        // for (size_t i = 0; i < Channels; ++i) {
        //     cout << "  " << i << ": ";
        //     for (const auto& d : _cached_view[i]) {
        //         cout << d << " ";
        //     }
        //     cout << endl;
        // }

        // calculate the euclidean distance for each channel
        // slide the view with shift_dist on the window buffer
        // TODO: support bind_channels
        size_t verified_channels = 0;
        for (size_t i = 0; i < Channels; ++i) {
            const auto& window_i   = _window[i];
            const auto& view_i     = _cached_view[i];
            auto        thresh_i   = _euclidean_dist_thresh[i];
            size_t      window_end = window_i.size() - _view_size;
            for (size_t j = 0; j < window_end; j += _shift_dist) {
                DistType euclidean_dist{};
                for (size_t k = 0; k < _view_size; ++k) {
                    euclidean_dist += pow(window_i[j + k] - view_i[k], 2);
                }
                if (euclidean_dist < thresh_i) {
                    ++verified_channels;
                    break;
                }
            }
        }

        return verified_channels != Channels ? AnomalyType::Anomaly : AnomalyType::Normal;
    }

    inline vector<size_t> geneRandViewIndex(size_t range_start, size_t range_end, size_t n) const {
        vector<size_t> view_index(n);

        random_device                    rd;
        mt19937                          gen(rd());
        uniform_int_distribution<size_t> dis(range_start, range_end);
        size_t                           rand_index;
        for (auto it = begin(view_index); it != end(view_index); ++it) {
            rand_index = dis(gen);
            if (find(begin(view_index), it, rand_index) != it) {
                continue;  // skip if the index is already in the view_index
            }
            *it = rand_index;
        }

        return view_index;
    }

   private:
    mutable volatile bool _buffer_flag;
    volatile size_t       _buffer_cidx;
    size_t                _buffer_size;

    size_t _window_start;
    size_t _window_size;

    size_t _view_size;
    size_t _shift_dist;
    size_t _minimal_n;
    bool   _bind_channels;

    array<vector<DataType>, Channels> _buffer;
    array<vector<DataType>, Channels> _window;

    array<DistType, Channels> _euclidean_dist_thresh;

    // cache buffer, eliminate memory allocation
    mutable array<vector<DataType>, Channels> _cached_view;
};

}  // namespace ad
