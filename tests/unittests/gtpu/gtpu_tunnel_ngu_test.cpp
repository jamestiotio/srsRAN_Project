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

#include "gtpu_test_shared.h"
#include "lib/gtpu/gtpu_pdu.h"
#include "srsran/gtpu/gtpu_tunnel_ngu_factory.h"
#include "srsran/gtpu/gtpu_tunnel_rx.h"
#include "srsran/gtpu/gtpu_tunnel_tx.h"
#include <gtest/gtest.h>
#include <queue>

using namespace srsran;

class gtpu_tunnel_rx_lower_dummy : public gtpu_tunnel_ngu_rx_lower_layer_notifier
{
  void on_new_sdu(byte_buffer sdu, qos_flow_id_t qos_flow_id) final
  {
    last_rx             = std::move(sdu);
    last_rx_qos_flow_id = qos_flow_id;
  }

public:
  byte_buffer   last_rx;
  qos_flow_id_t last_rx_qos_flow_id;
};
class gtpu_tunnel_tx_upper_dummy : public gtpu_tunnel_tx_upper_layer_notifier
{
  void on_new_pdu(byte_buffer buf, const ::sockaddr_storage& /*addr*/) final { last_tx = std::move(buf); }

public:
  byte_buffer last_tx;
};

class gtpu_tunnel_rx_upper_dummy : public gtpu_tunnel_rx_upper_layer_interface
{
public:
  void handle_pdu(byte_buffer pdu) final { last_rx = std::move(pdu); }

  byte_buffer last_rx;
};

/// Fixture class for GTP-U tunnel NG-U tests
class gtpu_tunnel_ngu_test : public ::testing::Test
{
public:
  gtpu_tunnel_ngu_test() :
    logger(srslog::fetch_basic_logger("TEST", false)), gtpu_logger(srslog::fetch_basic_logger("GTPU", false))
  {
  }

protected:
  void SetUp() override
  {
    // init test's logger
    srslog::init();
    logger.set_level(srslog::basic_levels::debug);

    // init GTP-U logger
    gtpu_logger.set_level(srslog::basic_levels::debug);
    gtpu_logger.set_hex_dump_max_size(100);
  }

  void TearDown() override
  {
    // flush logger after each test
    srslog::flush();
  }

  // Test logger
  srslog::basic_logger& logger;

  // GTP-U logger
  srslog::basic_logger& gtpu_logger;
  gtpu_tunnel_logger    gtpu_rx_logger{"GTPU", {0, 1, "DL"}};
  gtpu_tunnel_logger    gtpu_tx_logger{"GTPU", {0, 1, "UL"}};

  // GTP-U tunnel entity
  std::unique_ptr<gtpu_tunnel_ngu> gtpu;

  // Surrounding tester
  gtpu_tunnel_rx_lower_dummy gtpu_rx = {};
  gtpu_tunnel_tx_upper_dummy gtpu_tx = {};
};

/// \brief Test correct creation of GTP-U entity
TEST_F(gtpu_tunnel_ngu_test, entity_creation)
{
  // init GTP-U entity
  gtpu_tunnel_ngu_creation_message msg = {};
  msg.cfg.rx.local_teid                = 0x1;
  msg.cfg.tx.peer_teid                 = 0x2;
  msg.cfg.tx.peer_addr                 = "127.0.0.1";
  msg.rx_lower                         = &gtpu_rx;
  msg.tx_upper                         = &gtpu_tx;
  gtpu                                 = create_gtpu_tunnel_ngu(msg);

  ASSERT_NE(gtpu, nullptr);
};

/// \brief Test correct reception of GTP-U packet with PDU Session Container
TEST_F(gtpu_tunnel_ngu_test, rx_sdu)
{
  // init GTP-U entity
  gtpu_tunnel_ngu_creation_message msg = {};
  msg.cfg.rx.local_teid                = 0x2;
  msg.cfg.tx.peer_teid                 = 0xbc1e3be9;
  msg.cfg.tx.peer_addr                 = "127.0.0.1";
  msg.rx_lower                         = &gtpu_rx;
  msg.tx_upper                         = &gtpu_tx;
  gtpu                                 = create_gtpu_tunnel_ngu(msg);

  byte_buffer orig_vec  = make_byte_buffer(gtpu_ping_vec_teid_2_qfi_1);
  byte_buffer strip_vec = make_byte_buffer(gtpu_ping_vec_teid_2_qfi_1);
  gtpu_header tmp;
  bool        read_ok = gtpu_read_and_strip_header(tmp, strip_vec, gtpu_rx_logger);
  ASSERT_EQ(read_ok, true);

  gtpu_tunnel_rx_upper_layer_interface* rx = gtpu->get_rx_upper_layer_interface();
  rx->handle_pdu(std::move(orig_vec));
  ASSERT_EQ(strip_vec, gtpu_rx.last_rx);
  ASSERT_EQ(uint_to_qos_flow_id(1), gtpu_rx.last_rx_qos_flow_id);
};

/// \brief Test correct transmission of GTP-U packet
TEST_F(gtpu_tunnel_ngu_test, tx_pdu)
{
  // init GTP-U entity
  gtpu_tunnel_ngu_creation_message msg = {};
  msg.cfg.rx.local_teid                = 0x1;
  msg.cfg.tx.peer_teid                 = 0x2;
  msg.cfg.tx.peer_addr                 = "127.0.0.1";
  msg.rx_lower                         = &gtpu_rx;
  msg.tx_upper                         = &gtpu_tx;
  gtpu                                 = create_gtpu_tunnel_ngu(msg);

  byte_buffer orig_vec{gtpu_ping_vec_teid_2};
  byte_buffer strip_vec{gtpu_ping_vec_teid_2};
  gtpu_header tmp;
  bool        read_ok = gtpu_read_and_strip_header(tmp, strip_vec, gtpu_rx_logger);
  ASSERT_EQ(read_ok, true);

  gtpu_tunnel_tx_lower_layer_interface* tx = gtpu->get_tx_lower_layer_interface();
  tx->handle_sdu(std::move(strip_vec));
  ASSERT_EQ(orig_vec, gtpu_tx.last_tx);
};

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
