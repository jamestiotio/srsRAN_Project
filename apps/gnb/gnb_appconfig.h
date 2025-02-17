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

#include "srsran/adt/variant.h"
#include "srsran/ran/band_helper.h"
#include "srsran/ran/bs_channel_bandwidth.h"
#include "srsran/ran/cyclic_prefix.h"
#include "srsran/ran/five_qi.h"
#include "srsran/ran/ntn.h"
#include "srsran/ran/pcch/pcch_configuration.h"
#include "srsran/ran/pci.h"
#include "srsran/ran/pdcch/search_space.h"
#include "srsran/ran/pdsch/pdsch_mcs.h"
#include "srsran/ran/pucch/pucch_configuration.h"
#include "srsran/ran/pusch/pusch_mcs.h"
#include "srsran/ran/rnti.h"
#include "srsran/ran/s_nssai.h"
#include "srsran/ran/subcarrier_spacing.h"
#include <string>
#include <thread>
#include <vector>

namespace srsran {

/// PRACH application configuration.
struct prach_appconfig {
  /// PRACH configuration index.
  unsigned prach_config_index = 1;
  /// PRACH root sequence index.
  unsigned prach_root_sequence_index = 1;
  /// Zero correlation zone
  unsigned zero_correlation_zone = 0;
  unsigned fixed_msg3_mcs        = 0;
  unsigned max_msg3_harq_retx    = 4;
  /// \c preambleReceivedTargetPower, target power at the network rx side, in dBm. Only values multiple of 2 are valid.
  int preamble_rx_target_pw = -100;
  /// Total number of PRACH preambles used for contention based and contention free 4-step or 2-step random access.
  optional<unsigned> total_nof_ra_preambles;
  /// Offset of lowest PRACH transmission occasion in frequency domain respective to PRB 0. To minimize interference
  /// with the PUCCH, the user should leave some guardband between the PUCCH CRBs and the PRACH PRBs.
  /// Possible values: {0,...,MAX_NOF_PRB - 1}.
  optional<unsigned> prach_frequency_start;
};

/// TDD pattern configuration. See TS 38.331, \c TDD-UL-DL-Pattern.
struct tdd_ul_dl_pattern_appconfig {
  /// Periodicity of the DL-UL pattern in slots. Values {2,...,80}.
  unsigned dl_ul_period_slots = 10;
  /// Values: {0,...,maxNrofSlots=80}.
  unsigned nof_dl_slots = 6;
  /// Values: {0,...,maxNrofSymbols-1=13}.
  unsigned nof_dl_symbols = 0;
  /// Values: {0,...,maxNrofSlots=80}.
  unsigned nof_ul_slots = 3;
  /// Values: {0,...,maxNrofSymbols-1=13}.
  unsigned nof_ul_symbols = 0;
};

/// TDD configuration. See TS 38.331, \c TDD-UL-DL-ConfigCommon.
struct tdd_ul_dl_appconfig {
  tdd_ul_dl_pattern_appconfig           pattern1;
  optional<tdd_ul_dl_pattern_appconfig> pattern2;
};

/// Paging related configuration. See TS 38.331, PCCH-Config.
struct paging_appconfig {
  /// SearchSpace to use for Paging. Values {0, 1}.
  unsigned paging_search_space_id = 1;
  /// Default Paging cycle in nof. Radio Frames. Values {32, 64, 128, 256}.
  unsigned default_paging_cycle = 128;
  /// Number of paging frames per DRX cycle. Values {oneT, halfT, quarterT, oneEighthT, oneSixteethT}.
  pcch_config::nof_pf_per_drx_cycle nof_pf = pcch_config::nof_pf_per_drx_cycle::oneT;
  /// Paging frame offset. Values {0,...,(T/nof_pf_per_paging_cycle) - 1}.
  unsigned pf_offset = 0;
  /// Number of paging occasions per paging frame. Values {1, 2, 4}.
  unsigned nof_po_per_pf = 1;
};

/// PDCCH application configuration.
struct pdcch_appconfig {
  /// Use an UE-dedicated or Common Search Space.
  search_space_configuration::type_t ue_ss_type = search_space_configuration::type_t::ue_dedicated;
  /// Flag specifying whether to use non-fallback or fallback DCI format in UE dedicated SearchSpace.
  bool dci_format_0_1_and_1_1 = true;
};

/// PDSCH application configuration.
struct pdsch_appconfig {
  /// Minimum modulation and coding scheme index for C-RNTI PDSCH allocations. Note that setting a high minimum MCS
  /// may lead to a high BLER if the SINR is low.
  unsigned min_ue_mcs = 0;
  /// Maximum modulation and coding scheme index for C-RNTI PDSCH allocations. To set a fixed MCS, set \c min_ue_mcs
  /// equal to the \c max_ue_mcs.
  unsigned max_ue_mcs = 28;
  /// RAR modulation and coding scheme index.
  unsigned fixed_rar_mcs = 0;
  /// SI modulation and coding scheme index.
  unsigned fixed_sib1_mcs = 5;
  /// Number of UE DL HARQ processes.
  unsigned nof_harqs = 16;
  /// Maximum number of consecutive DL KOs before an RLF is reported.
  unsigned max_consecutive_kos = 100;
  /// Redundancy version sequence to use. Each element can have one of the following values: {0, 1, 2, 3}.
  std::vector<unsigned> rv_sequence = {0, 2, 3, 1};
  /// MCS table to use for PDSCH
  pdsch_mcs_table mcs_table = pdsch_mcs_table::qam64;
  /// Number of antenna ports. If empty, the \c nof_ports is derived from the number of DL antennas.
  optional<unsigned> nof_ports;
};

/// PUSCH application configuration.
struct pusch_appconfig {
  /// \brief Minimum modulation and coding scheme index for C-RNTI PUSCH allocations. Note that setting a high minimum
  /// MCS may lead to a high BLER if the SINR is low.
  unsigned min_ue_mcs = 0;
  /// Maximum modulation and coding scheme index for C-RNTI PUSCH allocations. To set a fixed MCS, set \c min_ue_mcs
  /// equal to the \c max_ue_mcs.
  unsigned max_ue_mcs = 28;
  /// Maximum number of consecutive UL KOs before an RLF is reported.
  unsigned max_consecutive_kos = 100;
  /// Redundancy version sequence to use. Each element can have one of the following values: {0, 1, 2, 3}.
  std::vector<unsigned> rv_sequence = {0};
  /// MCS table to use for PUSCH
  pusch_mcs_table mcs_table = pusch_mcs_table::qam64;
  /// \c msg3-DeltaPreamble, TS 38.331. Values: {-1,...,6}.
  int msg3_delta_preamble = 6;
  /// \c p0-NominalWithGrant, TS 38.331. Value in dBm. Only even values allowed within {-202,...,24}.
  int p0_nominal_with_grant = -76;

  /// \c betaOffsetACK-Index1, \c BetaOffsets, TS 38.331. Values: {0,...,31}.
  unsigned b_offset_ack_idx_1 = 9;
  /// \c betaOffsetACK-Index2, \c BetaOffsets, TS 38.331. Values: {0,...,31}.
  unsigned b_offset_ack_idx_2 = 9;
  /// \c betaOffsetACK-Index3, \c BetaOffsets, TS 38.331. Values: {0,...,31}.
  unsigned b_offset_ack_idx_3 = 9;
  /// \c betaOffsetCSI-Part1-Index1, \c BetaOffsets, TS 38.331. Values: {0,...,31}.
  unsigned b_offset_csi_p1_idx_1 = 9;
  /// \c betaOffsetCSI-Part1-Index2, \c BetaOffsets, TS 38.331. Values: {0,...,31}.
  unsigned b_offset_csi_p1_idx_2 = 9;
  /// \c betaOffsetCSI-Part2-Index1, \c BetaOffsets, TS 38.331. Values: {0,...,31}.
  unsigned b_offset_csi_p2_idx_1 = 9;
  /// \c betaOffsetCSI-Part2-Index2, \c BetaOffsets, TS 38.331. Values: {0,...,31}.
  unsigned b_offset_csi_p2_idx_2 = 9;

  /// \brief Power level corresponding to MSG-3 TPC command in dB, as per Table 8.2-2, TS 38.213.
  /// Values {-6,...,8} and must be a multiple of 2.
  int msg3_delta_power = 8;
};

struct pucch_appconfig {
  /// \c PUCCH-ConfigCommon parameters.
  /// \c p0-nominal, TS 38.331. Value in dBm. Only even values allowed within {-202,...,24}.
  int p0_nominal = -90;

  /// \c PUCCH-Config parameters.
  /// Number of PUCCH Format 1 resources per UE for HARQ-ACK reporting. Values {1,...,8}.
  unsigned nof_ue_pucch_f1_res_harq = 3;
  /// Number of PUCCH Format 2 resources per UE for HARQ-ACK reporting. Values {1,...,8}.
  unsigned nof_ue_pucch_f2_res_harq = 6;
  /// Number of PUCCH Format 1 cell resources for SR. Values {1,...,4}.
  unsigned nof_cell_sr_resources = 2;

  /// PUCCH F1 resource parameters.
  /// Number of symbols for PUCCH Format 1. Values {4, 14}.
  unsigned f1_nof_symbols = 14;
  bool     f1_enable_occ  = false;
  /// \brief Number of different Initial Cyclic Shifts that can be used for PUCCH Format 1.
  /// Values: {1, 2, 3, 4, 6, 12}; 0 corresponds to "no cyclic shift".
  unsigned nof_cyclic_shift = 1;
  /// Set true for PUCCH Format 1 intra-slot frequency hopping.
  bool f1_intraslot_freq_hopping = false;

  /// PUCCH F2 resource parameters.
  /// Number of symbols for PUCCH Format 2. Values {1, 2}.
  unsigned f2_nof_symbols = 2;
  /// Max number of PRBs for PUCCH Format 2. Values {1,...,16}.
  unsigned f2_max_nof_rbs = 1;
  /// \brief Maximum payload in bits that can be carried by PUCCH Format 2. Values {-1,...,11}.
  /// Value -1 to unset. If this is set, \ref f2_max_nof_rbs is ignored.
  optional<unsigned> max_payload_bits;
  /// Set true for PUCCH Format 2 intra-slot frequency hopping. This field is ignored if f2_nof_symbols == 1.
  bool f2_intraslot_freq_hopping = false;
  /// Max code rate.
  max_pucch_code_rate max_code_rate = max_pucch_code_rate::dot_25;
};

/// Parameters that are used to initialize or build the \c PhysicalCellGroupConfig, TS 38.331.
struct phy_cell_group_appconfig {
  /// \brief \c p-NR-FR1, part of \c PhysicalCellGroupConfig, TS 38.331. Values: {-30,...,33}.
  /// The maximum total TX power to be used by the UE in this NR cell group across all serving cells in FR1.
  int p_nr_fr1 = 10;
};

/// Amplitude control application configuration.
struct amplitude_control_appconfig {
  /// Baseband gain back-off. This accounts for the signal PAPR and is applied regardless of clipping settings.
  float gain_backoff_dB = 12.0F;
  /// Power ceiling in dB, relative to the full scale amplitude of the radio.
  float power_ceiling_dBFS = -0.1F;
  /// Clipping of the baseband samples. If enabled, the samples that exceed the power ceiling are clipped.
  bool enable_clipping = false;
};

struct ssb_appconfig {
  /// SSB period in milliseconds.
  unsigned ssb_period_msec = 10;
  /// \brief \c ss-PBCH-BlockPower, part of \c ServingCellConfigCommonSIB, as per TS 38.331.
  /// Average EPRE of the REs that carry secondary synchronization signals in dBm used for SSB transmission.
  /// Values: {-60,..,70}
  int ssb_block_power = -16;
  /// PSS EPRE to SSS EPRE for SSB, as per TS 38.213, Section 4.1.
  ssb_pss_to_sss_epre pss_to_sss_epre = ssb_pss_to_sss_epre::dB_0;
};

struct csi_appconfig {
  /// \brief \c powerControlOffset, part of \c NZP-CSI-RS-Resource, as per TS 38.331.
  /// Power offset of PDSCH RE to NZP CSI-RS RE. Value in dB {-8,...,15}.
  int pwr_ctrl_offset = 0;
};

/// Base cell configuration.
struct base_cell_appconfig {
  /// Physical cell identifier.
  pci_t pci = 1;
  /// Downlink arfcn.
  unsigned dl_arfcn = 536020;
  /// NR band.
  optional<nr_band> band;
  /// Channel bandwidth in MHz.
  bs_channel_bandwidth_fr1 channel_bw_mhz = bs_channel_bandwidth_fr1::MHz20;
  /// Number of antennas in downlink.
  unsigned nof_antennas_dl = 1;
  /// Number of antennas in uplink.
  unsigned nof_antennas_ul = 1;
  /// Human readable full PLMN (without possible filler digit).
  std::string plmn = "00101";
  /// TAC.
  unsigned tac = 7;
  /// \c q-RxLevMin, part of \c cellSelectionInfo, \c SIB1, TS 38.311, in dBm.
  int q_rx_lev_min = -70;
  /// \c q-QualMin, part of \c cellSelectionInfo, \c SIB1, TS 38.311, in dB.
  int q_qual_min = -20;
  /// SSB parameters.
  ssb_appconfig ssb_cfg;
  /// PDCCH configuration.
  pdcch_appconfig pdcch_cfg;
  /// PDSCH configuration.
  pdsch_appconfig pdsch_cfg;
  /// PRACH configuration.
  prach_appconfig prach_cfg;
  /// PUSCH configuration.
  pusch_appconfig pusch_cfg;
  /// PUCCH configuration.
  pucch_appconfig pucch_cfg;
  /// Physical Cell Group parameters.
  phy_cell_group_appconfig pcg_cfg;
  /// Common subcarrier spacing for the entire resource grid. It must be supported by the band SS raster.
  subcarrier_spacing common_scs = subcarrier_spacing::kHz15;
  /// TDD slot configuration.
  optional<tdd_ul_dl_appconfig> tdd_ul_dl_cfg;
  /// Paging configuration.
  paging_appconfig paging_cfg;
  /// CSI configuration.
  csi_appconfig csi_cfg;
};

/// Cell configuration
struct cell_appconfig {
  /// Cell configuration.
  base_cell_appconfig cell;
};

/// RLC UM TX configuration
struct rlc_tx_um_appconfig {
  uint16_t sn_field_length; ///< Number of bits used for sequence number
  int32_t  t_reassembly;    ///< Timer used by rx to detect PDU loss (ms)
};

/// RLC UM RX configuration
struct rlc_rx_um_appconfig {
  uint16_t sn_field_length; ///< Number of bits used for sequence number
  int32_t  t_reassembly;    ///< Timer used by rx to detect PDU loss (ms)
};

/// RLC UM configuration
struct rlc_um_appconfig {
  rlc_tx_um_appconfig tx;
  rlc_rx_um_appconfig rx;
};

/// RLC UM TX configuration
struct rlc_tx_am_appconfig {
  uint16_t sn_field_length; ///< Number of bits used for sequence number
  int32_t  t_poll_retx;     ///< Poll retx timeout (ms)
  uint32_t max_retx_thresh; ///< Max retx threshold
  int32_t  poll_pdu;        ///< Insert poll bit after this many PDUs
  int32_t  poll_byte;       ///< Insert poll bit after this much data (bytes)
};

/// RLC UM RX configuration
struct rlc_rx_am_appconfig {
  uint16_t sn_field_length;   ///< Number of bits used for sequence number
  int32_t  t_reassembly;      ///< Timer used by rx to detect PDU loss (ms)
  int32_t  t_status_prohibit; ///< Timer used by rx to prohibit tx of status PDU (ms)
};

/// RLC AM configuration
struct rlc_am_appconfig {
  rlc_tx_am_appconfig tx;
  rlc_rx_am_appconfig rx;
};

/// RLC configuration
struct rlc_appconfig {
  std::string      mode = "am";
  rlc_um_appconfig um;
  rlc_am_appconfig am;
};

/// F1-U configuration at DU side
struct f1u_du_appconfig {
  int32_t t_notify; ///< Maximum backoff time for transmit/delivery notifications from DU to CU_UP (ms)
};

/// F1-U configuration at CU_UP side
struct f1u_cu_up_appconfig {
  int32_t t_notify; ///< Maximum backoff time for discard notifications from CU_UP to DU (ms)
};

struct pdcp_rx_appconfig {
  uint16_t sn_field_length;       ///< Number of bits used for sequence number
  int32_t  t_reordering;          ///< Timer used to detect PDUs losses (ms)
  bool     out_of_order_delivery; ///< Whether out-of-order delivery to upper layers is enabled
};

struct pdcp_tx_appconfig {
  uint16_t sn_field_length;        ///< Number of bits used for sequence number
  int32_t  discard_timer;          ///< Timer used to notify lower layers to discard PDUs (ms)
  bool     status_report_required; ///< Whether PDCP status report is required
};

struct pdcp_appconfig {
  bool              integrity_protection_required; ///< Whether DRB integrity is required
  pdcp_tx_appconfig tx;
  pdcp_rx_appconfig rx;
};
/// QoS configuration
struct qos_appconfig {
  five_qi_t           five_qi = uint_to_five_qi(9);
  rlc_appconfig       rlc;
  f1u_du_appconfig    f1u_du;
  f1u_cu_up_appconfig f1u_cu_up;
  pdcp_appconfig      pdcp;
};

struct amf_appconfig {
  std::string ip_addr                = "127.0.0.1";
  uint16_t    port                   = 38412;
  std::string bind_addr              = "127.0.0.1";
  int         sctp_rto_initial       = 120;
  int         sctp_rto_min           = 120;
  int         sctp_rto_max           = 500;
  int         sctp_init_max_attempts = 3;
  int         sctp_max_init_timeo    = 500;
  bool        no_core                = false;
};

struct cu_cp_appconfig {
  int inactivity_timer = 7200; // in seconds
};

struct log_appconfig {
  std::string filename    = "/tmp/gnb.log"; // Path to write log file or "stdout" to print to console.
  std::string all_level   = "warning";      // Default log level for all layers.
  std::string lib_level   = "warning"; // Generic log level assigned to library components without layer-specific level.
  std::string du_level    = "warning";
  std::string cu_level    = "warning";
  std::string phy_level   = "warning";
  std::string radio_level = "info";
  std::string mac_level   = "warning";
  std::string rlc_level   = "warning";
  std::string f1ap_level  = "warning";
  std::string f1u_level   = "warning";
  std::string pdcp_level  = "warning";
  std::string rrc_level   = "warning";
  std::string ngap_level  = "warning";
  std::string sdap_level  = "warning";
  std::string gtpu_level  = "warning";
  std::string sec_level   = "warning";
  std::string fapi_level  = "warning";
  std::string ofh_level   = "warning";
  int         hex_max_size      = 0;     // Maximum number of bytes to write when dumping hex arrays.
  bool        broadcast_enabled = false; // Set to true to log broadcasting messages and all PRACH opportunities.
  std::string phy_rx_symbols_filename;   // Set to a valid file path to print the received symbols.
};

struct pcap_appconfig {
  struct {
    std::string filename = "/tmp/gnb_ngap.pcap";
    bool        enabled  = false;
  } ngap;
  struct {
    std::string filename = "/tmp/gnb_e1ap.pcap";
    bool        enabled  = false;
  } e1ap;
  struct {
    std::string filename = "/tmp/gnb_f1ap.pcap";
    bool        enabled  = false;
  } f1ap;
  struct {
    std::string filename = "/tmp/gnb_mac.pcap";
    bool        enabled  = false;
  } mac;
};

/// Lower physical layer thread profiles.
enum class lower_phy_thread_profile {
  /// Same task worker as the rest of the PHY (ZMQ only).
  blocking = 0,
  /// Single task worker for all the lower physical layer task executors.
  single,
  /// Two task workers - one for the downlink and one for the uplink.
  dual,
  /// Dedicated task workers for each of the subtasks (downlink processing, uplink processing, reception and
  /// transmission).
  quad
};

/// Expert upper physical layer configuration.
struct expert_upper_phy_appconfig {
  /// Number of threads for encoding PDSCH. Set to one for no concurrency acceleration in the PDSCH encoding.
  unsigned nof_pdsch_threads = 1;
  /// Number of threads for processing PUSCH and PUCCH. It is set to 4 by default unless the available hardware
  /// concurrency is limited, in which case the most suitable number of threads between one and three will be selected.
  unsigned nof_ul_threads = std::min(4U, std::max(std::thread::hardware_concurrency(), 4U) - 3U);
  /// Number of PUSCH LDPC decoder iterations.
  unsigned pusch_decoder_max_iterations = 6;
  /// Set to true to enable the PUSCH LDPC decoder early stop.
  bool pusch_decoder_early_stop = true;
  /// System time-based throttling. See \ref lower_phy_configuration::system_time_throttling for more information.
  float lphy_dl_throttling = 0.0F;
};

struct test_mode_ue_appconfig {
  /// C-RNTI to assign to the test UE.
  rnti_t rnti = INVALID_RNTI;
  /// Whether PDSCH grants are automatically assigned to the test UE.
  bool pdsch_active = true;
  /// Whether PUSCH grants are automatically assigned to the test UE.
  bool pusch_active = true;
  /// Channel Quality Indicator to use for the test UE.
  unsigned cqi = 15;
  /// Rank Indicator to use for the test UE. This value has to be lower than the number of ports.
  unsigned ri = 1;
  /// Precoding Matrix Indicator to use for the test UE.
  unsigned pmi = 0;
};

/// gNB app Test Mode configuration.
struct test_mode_appconfig {
  /// Creates a UE with the given the given params for testing purposes.
  test_mode_ue_appconfig test_ue;
};

/// Expert SDR Radio Unit configuration.
struct ru_sdr_expert_appconfig {
  /// Lower physical layer thread profile.
  lower_phy_thread_profile lphy_executor_profile = lower_phy_thread_profile::dual;
};

/// gNB app SDR Radio Unit cell configuration.
struct ru_sdr_cell_appconfig {
  /// Amplitude control configuration.
  amplitude_control_appconfig amplitude_cfg;
};

/// gNB app SDR Radio Unit configuration.
struct ru_sdr_appconfig {
  /// Sampling frequency in MHz.
  double srate_MHz = 61.44;
  /// RF driver name.
  std::string device_driver = "uhd";
  /// RF driver arguments.
  std::string device_arguments = "";
  /// All transmit channel gain in decibels.
  double tx_gain_dB = 50.0;
  /// All receive channel gain in decibels.
  double rx_gain_dB = 60.0;
  /// Center frequency offset in hertz applied to all radio channels.
  double center_freq_offset_Hz = 0.0;
  /// Clock calibration in Parts Per Million (PPM). It is applied to the carrier frequency.
  double calibrate_clock_ppm = 0.0;
  /// LO Offset in MHz. It shifts the LO from the center frequency for moving the LO leakage out of the channel.
  double lo_offset_MHz = 0.0;
  /// \brief Rx to Tx radio time alignment calibration in samples.
  ///
  /// Compensates for the reception and transmission time misalignment inherent to the RF device. Setting this parameter
  /// overrides the default calibration, which is dependent on the selected RF device driver. Positive values reduce the
  /// RF transmission delay with respect to the RF reception. Since the UE receives the DL signal earlier, an incoming
  /// PRACH will also be detected earlier within the reception window. Negative values have the opposite effect, for
  /// example, a value of -1000 at a sample rate of 61.44 MHz increases the transmission delay and causes an incoming
  /// PRACH to be detected 16.3 us later within the reception window.
  optional<int> time_alignment_calibration;
  /// Synchronization source.
  std::string synch_source = "default";
  /// Clock source.
  std::string clock_source = "default";
  /// Over-the wire format. Determines the format in which samples are transported from the radio to the host.
  std::string otw_format = "default";
  /// Expert SDR Radio Unit settings.
  ru_sdr_expert_appconfig expert_cfg;
  /// SDR Radio Unit cells configuration.
  std::vector<ru_sdr_cell_appconfig> cells = {{}};
};

/// gNB app Open Fronthaul cell configuration.
struct ru_ofh_cell_appconfig {
  /// Ethernet network interface name.
  std::string network_interface = "enp1s0f0";
  /// Radio Unit MAC address.
  std::string ru_mac_address = "70:b3:d5:e1:5b:06";
  /// Distributed Unit MAC address.
  std::string du_mac_address = "00:11:22:33:00:77";
  /// V-LAN Tag control information field.
  uint16_t vlan_tag = 1U;
  /// RU PRACH port.
  unsigned ru_prach_port_id = 4U;
  /// RU Downlink port.
  std::vector<unsigned> ru_dl_port_id = {0, 1};
  /// RU Uplink port.
  std::vector<unsigned> ru_ul_port_id = {0, 1};
};

/// gNB app Open Fronthaul Radio Unit configuration.
struct ru_ofh_appconfig {
  /// Sets the maximum allowed processing delay in slots.
  unsigned max_processing_delay_slots = 2U;
  /// GPS Alpha - Valid value range: [0, 1.2288e7].
  unsigned gps_Alpha = 0;
  /// GPS Beta - Valid value range: [-32768, 32767].
  int gps_Beta = 0;
  /// \brief RU operating bandwidth.
  ///
  /// Set this option when the operating bandwidth of the RU is larger than the configured bandwidth of the cell.
  optional<bs_channel_bandwidth_fr1> ru_operating_bw;
  /// T1a maximum parameter for downlink Control-Plane in microseconds.
  unsigned T1a_max_cp_dl = 500U;
  /// T1a minimum parameter for downlink Control-Plane in microseconds.
  unsigned T1a_min_cp_dl = 258U;
  /// T1a maximum parameter for uplink Control-Plane in microseconds.
  unsigned T1a_max_cp_ul = 500U;
  /// T1a minimum parameter for uplink Control-Plane in microseconds.
  unsigned T1a_min_cp_ul = 285U;
  /// T1a maximum parameter for downlink User-Plane in microseconds.
  unsigned T1a_max_up = 300U;
  /// T1a minimum parameter for downlink User-Plane in microseconds.
  unsigned T1a_min_up = 85U;
  /// Enables the Control-Plane PRACH message signalling.
  bool is_prach_control_plane_enabled = false;
  /// \brief Downlink broadcast flag.
  ///
  /// If enabled, broadcasts the contents of a single antenna port to all downlink RU eAxCs.
  bool is_downlink_broadcast_enabled = false;
  /// Uplink compression method.
  std::string compression_method_ul = "bfp";
  /// Uplink compression bitwidth.
  unsigned compresion_bitwidth_ul = 9;
  /// Downlink compression method.
  std::string compression_method_dl = "bfp";
  /// Downlink compression bitwidth.
  unsigned compresion_bitwidth_dl = 9;
  /// IQ data scaling to be applied prior to Downlink data compression.
  float iq_scaling = 0.35F;
  /// Individual Open Fronthaul cells configurations.
  std::vector<ru_ofh_cell_appconfig> cells = {{}};
};

/// Monolithic gnb application configuration.
struct gnb_appconfig {
  /// Logging configuration.
  log_appconfig log_cfg;
  /// PCAP configuration.
  pcap_appconfig pcap_cfg;
  /// gNodeB identifier.
  uint32_t gnb_id = 411;
  /// Length of gNB identity in bits. Values {22,...,32}.
  uint8_t gnb_id_bit_length = 32;
  /// Node name.
  std::string ran_node_name = "srsgnb01";
  /// AMF configuration.
  amf_appconfig amf_cfg;
  /// CU-CP configuration.
  cu_cp_appconfig cu_cp_cfg;
  /// Radio Unit configuration.
  variant<ru_sdr_appconfig, ru_ofh_appconfig> ru_cfg = {ru_sdr_appconfig{}};
  /// \brief Base cell application configuration.
  ///
  /// \note When a cell is added, it will use the values of this base cell as default values for its base cell
  /// configuration. This parameter usage is restricted for filling cell information in the \remark cell_cfg variable.
  base_cell_appconfig common_cell_cfg;
  /// \brief Cell configuration.
  ///
  /// \note Add one cell by default.
  std::vector<cell_appconfig> cells_cfg = {{}};

  /// \brief QoS configuration.
  std::vector<qos_appconfig> qos_cfg;

  /// \brief Network slice configuration.
  std::vector<s_nssai_t> slice_cfg = {s_nssai_t{1}};

  /// Expert physical layer configuration.
  expert_upper_phy_appconfig expert_phy_cfg;

  /// Configuration for testing purposes.
  test_mode_appconfig test_mode_cfg = {};

  /// \brief NTN configuration.
  optional<ntn_config> ntn_cfg;
};

} // namespace srsran
