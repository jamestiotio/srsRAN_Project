/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#pragma once

#include "srsran/adt/span.h"
#include "srsran/adt/static_vector.h"
#include "srsran/ofh/ofh_constants.h"
#include "srsran/ran/frame_types.h"
#include "srsran/ran/slot_point.h"
#include <mutex>

namespace srsran {
namespace ether {

/// Length of Ethernet Jumbo frame.
constexpr unsigned MAX_ETH_FRAME_LENGTH = 9600;

/// Minimal Ethernet frame length.
constexpr unsigned MIN_ETH_FRAME_LENGTH = 64;

/// Storage for one Ethernet frame buffer.
class frame_buffer
{
  using buffer_type = std::array<uint8_t, MAX_ETH_FRAME_LENGTH>;

  size_t      sz     = 0;
  buffer_type buffer = {};

public:
  constexpr bool empty() const noexcept { return sz == 0; }

  constexpr size_t size() const noexcept { return sz; }

  constexpr void set_size(size_t new_size) noexcept
  {
    srsran_assert(new_size <= MAX_ETH_FRAME_LENGTH, "Can't set_size: new size if out-of-bounds");
    unsigned written_size = (new_size < MIN_ETH_FRAME_LENGTH) ? MIN_ETH_FRAME_LENGTH : new_size;
    if (new_size < MIN_ETH_FRAME_LENGTH) {
      std::fill(buffer.begin() + new_size, buffer.begin() + MIN_ETH_FRAME_LENGTH, 0);
    }
    sz = written_size;
  }

  constexpr void clear() noexcept { sz = 0; }

  span<uint8_t>       data() noexcept { return {buffer}; }
  span<const uint8_t> data() const noexcept { return {buffer.data(), sz}; }
};

/// Class encapsulating \c ethernet_frame_buffer buffers used in a circular manner. It is used by \c eth_frame_pool
/// class to manage Ethernet frame buffers for each slot symbol. Internally it keeps track of current write and read
/// positions.
class frame_buffer_array
{
  /// Maximum number of ethernet frames of each OFH type stored for each slot symbol.
  /// Every read-write operation retrieves either CP_PACKETS_PER_SYMBOL or UP_PACKETS_PER_SYMBOL buffers.
  /// \note Minimum value of stored frames is 2, which allows differentiation the written/read frame counters.
  static constexpr size_t MAX_ETH_FRAMES_PER_SYMBOL =
      std::max(2 * ofh::MAX_CP_MESSAGES_PER_SYMBOL, 2 * ofh::MAX_UP_MESSAGES_PER_SYMBOL);

  /// Maximum number of \c ethernet_frame_buffer arrays with prepared ethernet frames.
  static constexpr size_t MAX_NOF_PREPARED_FRAME_BATCHES = 2;

  /// Type of the buffer used in the pool to store Ethernet frames with a specific OFH packet.
  using storage_array_type = std::array<frame_buffer, MAX_ETH_FRAMES_PER_SYMBOL>;
  using ready_frames_type  = static_vector<span<const frame_buffer>, MAX_NOF_PREPARED_FRAME_BATCHES>;

  /// Structure used to count number of written and read elements in the circular array.
  struct rd_wr_counter {
    unsigned count    = 0;
    unsigned boundary = 0;
    // Constructor receives upper boundary.
    explicit rd_wr_counter(unsigned b) : boundary(b) {}
    // Increment by N positions and wrap-around if needed
    void increment(unsigned n = 1) { count += n; }
    // Return counter value.
    unsigned value() const { return count % boundary; }
  };

public:
  // Receives number of packets stored/read at a time, reserves storage to accommodate (2 * n_packets) packets.
  explicit frame_buffer_array(unsigned n_packets) : write_position(2 * n_packets), increment_quant(n_packets) {}

  // Returns view over increment_quant buffers for writing. Unread buffers might be overwritten
  span<frame_buffer> get_wr_buffers()
  {
    span<frame_buffer> wr_buffers(&buf_array[write_position.value()], increment_quant);
    write_position.increment(increment_quant);
    return wr_buffers;
  }

  void push_buffers(span<frame_buffer> prepared_buffers)
  {
    // Overwrite old data.
    if (ready_frames.size() == MAX_NOF_PREPARED_FRAME_BATCHES) {
      clear_buffers();
    }
    ready_frames.push_back(prepared_buffers);
  }

  void clear_buffers() { ready_frames.clear(); }

  // Returns a view over the written buffers for reading; returns an empty span if no buffers were written.
  span<const span<const frame_buffer>> get_rd_buffers() const
  {
    if (ready_frames.empty()) {
      return {};
    }
    return ready_frames;
  }

private:
  // Data buffers.
  storage_array_type buf_array;
  // Vector of spans of prepared ethernet packets.
  ready_frames_type ready_frames;
  // Keeps track of the current write position.
  rd_wr_counter write_position;
  // Number of buffers accessed at a time.
  unsigned increment_quant;
};

/// Pool of Ethernet frames pre-allocated for each slot symbol.
class eth_frame_pool
{
  /// Maximum number of entries contained by the pool, one entry per OFDM symbol, sized to accommodate 20 slots.
  static constexpr size_t NUM_ENTRIES = NOF_OFDM_SYM_PER_SLOT_NORMAL_CP * 20L;

  /// Pool entry storing circular arrays of ethernet frame buffers.
  struct pool_entry {
    /// Pool entry stores \c POOL_ENTRY_SIZE circular arrays for every OFH type.
    static constexpr size_t POOL_ENTRY_SIZE = (size_t)ofh::message_type::num_ofh_types;

    /// Circular arrays of Ethernet frame buffers for each OFH type.
    std::array<frame_buffer_array, POOL_ENTRY_SIZE> buffers = {frame_buffer_array{ofh::MAX_CP_MESSAGES_PER_SYMBOL},
                                                               frame_buffer_array{ofh::MAX_UP_MESSAGES_PER_SYMBOL}};

    /// Returns an entry_buffer given the OFH type.
    frame_buffer_array& get_ofh_type_buffers(ofh::message_type type) { return buffers[static_cast<unsigned>(type)]; }
    const frame_buffer_array& get_ofh_type_buffers(ofh::message_type type) const
    {
      return buffers[static_cast<unsigned>(type)];
    }

    /// Returns a view over next free frame buffers for a given OFH type, or an empty span if there is no free space.
    span<frame_buffer> get_write_buffers(ofh::message_type type)
    {
      frame_buffer_array& entry_buf = get_ofh_type_buffers(type);
      span<frame_buffer>  buffs     = entry_buf.get_wr_buffers();
      // Reset size of buffers before returning.
      for (auto& buf : buffs) {
        buf.clear();
      }
      return buffs;
    }

    /// Push span of ready buffers to the array associated with the given OFH type.
    void push_buffers(ofh::message_type type, span<frame_buffer> prepared_buffers)
    {
      frame_buffer_array& entry_buf = get_ofh_type_buffers(type);
      entry_buf.push_buffers(prepared_buffers);
    }

    void clear_buffers(ofh::message_type type)
    {
      frame_buffer_array& entry_buf = get_ofh_type_buffers(type);
      entry_buf.clear_buffers();
    }

    /// Returns a view over a next stored frame buffer for a given OFH type.
    span<const span<const frame_buffer>> get_read_buffers(ofh::message_type type) const
    {
      const frame_buffer_array& entry_buf = get_ofh_type_buffers(type);
      return entry_buf.get_rd_buffers();
    }
  }; // end of pool_entry

  /// Returns pool entry for the given slot and symbol.
  const pool_entry& get_pool_entry(slot_point slot_point, unsigned symbol) const
  {
    unsigned slot_index = slot_point.slot_index();
    unsigned sym_index  = (slot_index * NOF_OFDM_SYM_PER_SLOT_NORMAL_CP + symbol) % NUM_ENTRIES;
    return pool[sym_index];
  }
  /// Returns pool entry for the given slot and symbol.
  pool_entry& get_pool_entry(slot_point slot_point, unsigned symbol)
  {
    return const_cast<pool_entry&>(const_cast<const eth_frame_pool*>(this)->get_pool_entry(slot_point, symbol));
  }

public:
  /// Returns data buffer from the pool for the given slot and symbol.
  span<frame_buffer> get_frame_buffers(slot_point slot_point, unsigned symbol, ofh::message_type type)
  {
    pool_entry& p_entry = get_pool_entry(slot_point, symbol);
    // Acquire lock before accessing pool entry.
    std::lock_guard<std::mutex> lock(mutex);
    return p_entry.get_write_buffers(type);
  }

  /// Increments number of prepared ethernet frames in the given slot symbol.
  void
  eth_frames_ready(slot_point slot_point, unsigned symbol, ofh::message_type type, span<frame_buffer> prepared_buffers)
  {
    pool_entry& p_entry = get_pool_entry(slot_point, symbol);
    // Lock and update the pool entry.
    std::lock_guard<std::mutex> lock(mutex);
    p_entry.push_buffers(type, prepared_buffers);
  }

  /// Returns data buffer from the pool to a consumer thread.
  span<const span<const frame_buffer>>
  read_frame_buffers(slot_point slot_point, unsigned symbol, ofh::message_type type) const
  {
    const pool_entry& p_entry = get_pool_entry(slot_point, symbol);
    // Acquire lock before accessing pool entry.
    std::lock_guard<std::mutex> lock(mutex);
    return p_entry.get_read_buffers(type);
  }

  /// Clear prepared ethernet frames in the given symbol once it was sent to a gateway (thread-safe).
  void eth_frames_sent(slot_point slot_point, unsigned symbol, ofh::message_type type)
  {
    pool_entry& p_entry = get_pool_entry(slot_point, symbol);
    // Lock and increment number of read ethernet frames.
    std::lock_guard<std::mutex> lock(mutex);
    p_entry.clear_buffers(type);
  }

private:
  /// Buffer pool.
  std::array<pool_entry, NUM_ENTRIES> pool;
  /// Mutex protecting buffers read/write counters.
  mutable std::mutex mutex;
};

} // namespace ether
} // namespace srsran
