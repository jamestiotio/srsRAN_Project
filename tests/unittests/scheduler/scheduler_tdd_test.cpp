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

/// \file
/// \brief Unit test for scheduler using different TDD patterns.

#include "test_utils/config_generators.h"
#include "test_utils/indication_generators.h"
#include "test_utils/result_test_helpers.h"
#include "test_utils/scheduler_test_bench.h"
#include "srsran/ran/prach/prach_helper.h"
#include "srsran/ran/tdd/tdd_ul_dl_config_formatters.h"
#include "srsran/support/test_utils.h"
#include <gtest/gtest.h>
#include <ostream>

using namespace srsran;

class base_scheduler_tdd_tester : public scheduler_test_bench
{
protected:
  base_scheduler_tdd_tester(
      const tdd_ul_dl_config_common& tdd_cfg = config_helpers::make_default_tdd_ul_dl_config_common()) :
    scheduler_test_bench(4, tdd_cfg.ref_scs)
  {
    // Add Cell.
    this->add_cell([this, &tdd_cfg]() {
      params.scs_common       = tdd_cfg.ref_scs;
      params.dl_arfcn         = 520002;
      params.band             = nr_band::n41;
      params.channel_bw_mhz   = bs_channel_bandwidth_fr1::MHz20;
      const unsigned nof_crbs = band_helper::get_n_rbs_from_bw(
          params.channel_bw_mhz, params.scs_common, band_helper::get_freq_range(*params.band));
      static const uint8_t                              ss0_idx      = 0;
      optional<band_helper::ssb_coreset0_freq_location> ssb_freq_loc = band_helper::get_ssb_coreset0_freq_location(
          params.dl_arfcn, *params.band, nof_crbs, params.scs_common, params.scs_common, ss0_idx);
      if (!ssb_freq_loc.has_value()) {
        report_error("Unable to derive a valid SSB pointA and k_SSB for cell id ({}).\n", params.pci);
      }
      params.offset_to_point_a = (*ssb_freq_loc).offset_to_point_A;
      params.k_ssb             = (*ssb_freq_loc).k_ssb;
      params.coreset0_index    = (*ssb_freq_loc).coreset0_idx;

      sched_cell_configuration_request_message sched_cfg =
          test_helpers::make_default_sched_cell_configuration_request(params);
      // TDD params.
      sched_cfg.tdd_ul_dl_cfg_common = tdd_cfg;

      // RACH config
      optional<uint8_t> chosen_prach_cfg_idx =
          prach_helper::find_valid_prach_config_index(sched_cfg.ul_cfg_common.init_ul_bwp.generic_params.scs,
                                                      sched_cfg.ul_cfg_common.init_ul_bwp.generic_params.cp_extended,
                                                      *sched_cfg.tdd_ul_dl_cfg_common);
      sched_cfg.ul_cfg_common.init_ul_bwp.rach_cfg_common->rach_cfg_generic.prach_config_index = *chosen_prach_cfg_idx;

      return sched_cfg;
    }());

    // Add UE
    auto ue_cfg     = test_helpers::create_default_sched_ue_creation_request(params, {ue_drb_lcid});
    ue_cfg.ue_index = ue_idx;
    ue_cfg.crnti    = ue_rnti;
    this->add_ue(ue_cfg);
  }

  const du_ue_index_t ue_idx      = to_du_ue_index(0);
  const rnti_t        ue_rnti     = to_rnti(0x4601);
  const lcid_t        ue_drb_lcid = LCID_MIN_DRB;

  cell_config_builder_params params;
};

using test_params = tdd_ul_dl_config_common;

namespace srsran {

/// Formatter for test params.
void PrintTo(const test_params& value, ::std::ostream* os)
{
  *os << fmt::format("{}", value);
}

} // namespace srsran

class scheduler_dl_tdd_tester : public base_scheduler_tdd_tester, public ::testing::TestWithParam<test_params>
{
public:
  scheduler_dl_tdd_tester() : base_scheduler_tdd_tester(GetParam()) {}
};

TEST_P(scheduler_dl_tdd_tester, all_dl_slots_are_scheduled)
{
  // Enqueue enough bytes for continuous DL tx.
  dl_buffer_state_indication_message dl_buf_st{ue_idx, ue_drb_lcid, 10000000};
  this->push_dl_buffer_state(dl_buf_st);

  const unsigned MAX_COUNT = 1000;
  for (unsigned count = 0; count != MAX_COUNT; ++count) {
    this->run_slot();

    // For every DL slot.
    // Note: Skip special slots in test for now.
    if (cell_cfg_list[0].is_fully_dl_enabled(this->last_result_slot())) {
      // Ensure UE PDSCH allocations are made.
      ASSERT_FALSE(this->last_sched_res->dl.ue_grants.empty())
          << "The UE configuration is leading to some DL slots staying empty";
    }

    for (const pucch_info& pucch : this->last_sched_res->ul.pucchs) {
      if (pucch.format == pucch_format::FORMAT_1 and pucch.format_1.sr_bits != sr_nof_bits::no_sr) {
        // Skip SRs for this test.
        continue;
      }

      uci_indication uci_ind = create_uci_indication(this->last_result_slot(), ue_idx, pucch);
      this->sched->handle_uci_indication(uci_ind);
    }
  }
}

class scheduler_ul_tdd_tester : public base_scheduler_tdd_tester, public ::testing::TestWithParam<test_params>
{
public:
  scheduler_ul_tdd_tester() : base_scheduler_tdd_tester(GetParam()) {}
};

TEST_P(scheduler_ul_tdd_tester, all_ul_slots_are_scheduled)
{
  // Enqueue enough bytes for continuous UL tx.
  ul_bsr_indication_message bsr{
      to_du_cell_index(0), ue_idx, ue_rnti, bsr_format::SHORT_BSR, {ul_bsr_lcg_report{uint_to_lcg_id(0), 10000000}}};
  this->push_bsr(bsr);

  // Run some slots to ensure PDCCH can be scheduled.
  for (unsigned i = 0; i != cell_cfg_list[0].tdd_cfg_common->pattern1.nof_ul_symbols; ++i) {
    run_slot();
  }

  const unsigned MAX_COUNT = 1000;
  for (unsigned count = 0; count != MAX_COUNT; ++count) {
    this->run_slot();

    // For every UL slot.
    // Note: Skip special slots in test for now.
    if (cell_cfg_list[0].is_fully_ul_enabled(this->last_result_slot())) {
      // Ensure UE PUSCH allocations are made.
      ASSERT_FALSE(this->last_sched_res->ul.puschs.empty())
          << "The UE configuration is leading to some UL slots staying empty";
    }

    for (const ul_sched_info& pusch : this->last_sched_res->ul.puschs) {
      ul_crc_indication crc{};
      crc.cell_index = to_du_cell_index(0);
      crc.sl_rx      = this->last_result_slot();
      crc.crcs.resize(1);
      crc.crcs[0].ue_index       = ue_idx;
      crc.crcs[0].rnti           = ue_rnti;
      crc.crcs[0].harq_id        = to_harq_id(pusch.pusch_cfg.harq_id);
      crc.crcs[0].tb_crc_success = true;
      this->sched->handle_crc_indication(crc);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    scheduler_tdd_test,
    scheduler_dl_tdd_tester,
    testing::Values(
        // clang-format off
  // test_params{ref_scs, pattern1={slot_period, DL_slots, DL_symbols, UL_slots, UL_symbols}, pattern2={...}}
  test_params{subcarrier_spacing::kHz30, {10, 6, 4, 3, 4}, nullopt}));
  // TODO: Support more TDD patterns.
// clang-format on

INSTANTIATE_TEST_SUITE_P(
    scheduler_tdd_test,
    scheduler_ul_tdd_tester,
    testing::Values(
        // clang-format off
        // test_params{ref_scs, pattern1={slot_period, DL_slots, DL_symbols, UL_slots, UL_symbols}, pattern2={...}}
        test_params{subcarrier_spacing::kHz30, {10, 6, 4, 3, 4}, nullopt},
        test_params{subcarrier_spacing::kHz30, {10, 7, 4, 2, 4}, nullopt},
        test_params{subcarrier_spacing::kHz30, {6, 3, 4, 2, 4}, tdd_ul_dl_pattern{4, 4, 0, 0, 0}}));
// clang-format on
