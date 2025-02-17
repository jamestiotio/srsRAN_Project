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

#include "gtpu_tunnel_base_rx.h"
#include "srsran/psup/psup_packing.h"
#include "srsran/ran/cu_types.h"

namespace srsran {

/// Class used for receiving GTP-U bearers.
class gtpu_tunnel_ngu_rx : public gtpu_tunnel_base_rx
{
public:
  gtpu_tunnel_ngu_rx(uint32_t                                 ue_index,
                     gtpu_config::gtpu_rx_config              cfg_,
                     gtpu_tunnel_ngu_rx_lower_layer_notifier& rx_lower_) :
    gtpu_tunnel_base_rx(ue_index, cfg_), psup_packer(logger.get_basic_logger()), lower_dn(rx_lower_)
  {
    // Validate configuration
    logger.log_info("GTPU NGU configured. {}", cfg);
  }
  ~gtpu_tunnel_ngu_rx() = default;

protected:
  // domain-specific PDU handler
  void handle_pdu(byte_buffer buf, const gtpu_header& hdr) final
  {
    psup_dl_pdu_session_information pdu_session_info      = {};
    bool                            have_pdu_session_info = false;
    for (auto ext_hdr : hdr.ext_list) {
      switch (ext_hdr.extension_header_type) {
        case gtpu_extension_header_type::pdu_session_container:
          if (!have_pdu_session_info) {
            have_pdu_session_info = psup_packer.unpack(pdu_session_info, ext_hdr.container);
            if (!have_pdu_session_info) {
              logger.log_error("Failed to unpack PDU session container.");
            }
          } else {
            logger.log_warning("Ignoring multiple PDU session container.");
          }
          break;
        default:
          logger.log_warning("Ignoring unexpected extension header at NG-U interface. type={}",
                             ext_hdr.extension_header_type);
      }
    }
    if (!have_pdu_session_info) {
      logger.log_warning(
          "Incomplete PDU at NG-U interface: missing or invalid PDU session container. sdu_len={} teid={:#x}",
          buf.length(),
          hdr.teid);
      // As per TS 29.281 Sec. 5.2.2.7 the (...) PDU Session Container (...) shall be transmitted in a G-PDU over the N3
      // and N9 user plane interfaces (...).
      return;
    }

    logger.log_info(buf.begin(),
                    buf.end(),
                    "RX SDU. sdu_len={} teid={:#x} qos_flow={}",
                    buf.length(),
                    hdr.teid,
                    pdu_session_info.qos_flow_id);
    lower_dn.on_new_sdu(std::move(buf), pdu_session_info.qos_flow_id);
  }

private:
  psup_packing                             psup_packer;
  gtpu_tunnel_ngu_rx_lower_layer_notifier& lower_dn;
};
} // namespace srsran
