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

#include "srsran/asn1/rrc_nr/rrc_nr.h"
#include "srsran/cu_cp/up_resource_manager.h"
#include "srsran/rrc/rrc_cell_context.h"
#include "srsran/rrc/rrc_ue.h"
#include "srsran/rrc/rrc_ue_config.h"

namespace srsran {

namespace srs_cu_cp {

/// RRC states (3GPP 38.331 v15.5.1 Sec 4.2.1)
enum class rrc_state { idle = 0, connected, connected_inactive };

/// Holds the RRC UE context used by the UE object and all its procedures.
class rrc_ue_context_t
{
public:
  rrc_ue_context_t(const ue_index_t        ue_index_,
                   const rnti_t            c_rnti_,
                   const rrc_cell_context& cell_,
                   const rrc_ue_cfg_t&     cfg_) :
    ue_index(ue_index_), c_rnti(c_rnti_), cell(cell_), cfg(cfg_), up_mng(create_up_resource_manager(cfg_.up_cfg))
  {
  }

  up_resource_manager& get_up_manager() { return *up_mng; }

  const ue_index_t                       ue_index; // UE index assigned by the DU processor
  const rnti_t                           c_rnti;   // current C-RNTI
  const rrc_cell_context                 cell;     // current cell
  const rrc_ue_cfg_t                     cfg;
  rrc_state                              state = rrc_state::idle;
  std::unique_ptr<up_resource_manager>   up_mng;
  optional<uint32_t>                     five_g_tmsi;
  uint64_t                               setup_ue_id;
  asn1::rrc_nr::establishment_cause_opts connection_cause;
  security::security_context             sec_context;
  optional<asn1::rrc_nr::ue_nr_cap_s>    capabilities;
};

} // namespace srs_cu_cp

} // namespace srsran
