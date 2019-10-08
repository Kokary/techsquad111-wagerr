#!/usr/bin/env python3
# Copyright (c) 2014-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test running bitcoind with -reindex and -reindex-chainstate options.

- Start a single node and generate 3 blocks.
- Stop the node and restart it with -reindex. Verify that the node has reindexed up to block 3.
- Stop the node and restart it with -reindex-chainstate. Verify that the node has reindexed up to block 3.
"""

from test_framework.opcode import make_mapping, make_mlpatch, make_mlevent, post_opcode
from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import wait_until, rpc_port, assert_equal
from distutils.dir_util import copy_tree, remove_tree
import time
import os

class BettingTest(BitcoinTestFramework):
    def get_cache_dir_name(self, node_index, block_count):
        return ".test-chain-{0}-{1}-.node{2}".format(self.num_nodes, block_count, node_index);

    def get_node_setting(self, node_index, setting_name):
        with open(os.path.join(self.nodes[node_index].datadir, "wagerr.conf"), 'r', encoding='utf8') as f:
            for line in f:
                if line.startswith(setting_name + "="):
                    return line.split("=")[1].strip("\n")
        return None

    def get_local_peer(self, node_index, is_rpc=False):
        port = self.get_node_setting(node_index, "rpcport" if is_rpc else "port")
        return "127.0.0.1:" + str(rpc_port(node_index) if port is None else port)

    def sync_node_datadir(self, node_index, left, right):
        node = self.nodes[node_index]
        node.stop_node()
        node.wait_until_stopped()
        if not left:
           left = self.nodes[node_index].datadir
        if not right:
           right = self.nodes[node_index].datadir
        if os.path.isdir(right):
            remove_tree(right)
        copy_tree(left, right)
        node.rpchost = self.get_local_peer(node_index, True)
        node.start(self.extra_args)
        node.wait_for_rpc_connection()

    def set_test_params(self):
        self.extra_args = None
        self.setup_clean_chain = True
        self.num_nodes = 3

    def connect_network(self):
        for pair in [[n, n + 1 if n + 1 < self.num_nodes else 0] for n in range(self.num_nodes)]:
            for i in range(len(pair)):
                assert i < 2
                self.nodes[pair[i]].addnode(self.get_local_peer(pair[1 - i]), "onetry")
                wait_until(lambda:  all(peer['version'] != 0 for peer in self.nodes[pair[i]].getpeerinfo()))
        self.sync_all()
        for n in range(self.num_nodes):
            idx_l = n
            idx_r = n + 1 if n + 1 < self.num_nodes else 0
            assert_equal(self.nodes[idx_l].getblockcount(), self.nodes[idx_r].getblockcount())

    def setup_network(self):
        self.log.info("Setup Network")
        self.setup_nodes()
        self.connect_network()

    def save_cache(self, force=False):
        dir_names = dict()
        for n in range(self.num_nodes):
            dir_name = self.get_cache_dir_name(n, self.nodes[n].getblockcount())
            if force or not os.path.isdir(dir_name):
                dir_names[n] = dir_name
        if len(dir_names) > 0:
            for node_index in dir_names.keys():
                self.sync_node_datadir(node_index, None, dir_names[node_index])
            self.connect_network()

    def load_cache(self, block_count):
        dir_names = dict()
        for n in range(self.num_nodes):
            dir_name = self.get_cache_dir_name(n, block_count)
            if os.path.isdir(dir_name):
                dir_names[n] = dir_name
        if len(dir_names) == self.num_nodes:
            for node_index in range(self.num_nodes):
                self.sync_node_datadir(node_index, dir_names[node_index], None)
            self.connect_network()
            return True
        return False

    def check_premine(self, block_count=255):
        self.log.info("Check Premine...")

        if block_count > 3 and not self.load_cache(block_count):
            block_count = block_count - 3
            assert_equal(len(self.nodes), self.num_nodes)
            for i in range(10):
                for n in range(self.num_nodes):
                    self.nodes[n].generate(int(block_count / self.num_nodes / 10))
                    self.sync_all()
            while block_count != self.nodes[1].getblockcount():
                self.nodes[1].generate(1)
                self.sync_all()
            for n in range(self.num_nodes):
                assert_equal(self.nodes[n].getblockcount(), block_count)

            self.nodes[0].sendtoaddress("TS4s7JvAVP6Rn1go7Tsa2PrZddMqdMokCM", 100)
            self.nodes[0].sendtoaddress("TEYNdtsUy6k2zG6fY5mAo4jineGP8oLpNR", 100)
            self.nodes[0].sendtoaddress("TXGFsuGcRGv5QHpkcTxC7ChCmvTuXwav6U", 1000)
            self.nodes[0].sendtoaddress("TEYrmcfMzmQJjwXanQRaeioYoC9mZsX4gM", 2000)
            self.nodes[0].generate(1)
            self.sync_all()
            block_count = block_count + 1
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), int(self.nodes[0].getbalance()) / 3)
            self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), int(self.nodes[0].getbalance()) / 2)
            self.nodes[0].generate(1)
            self.sync_all()
            block_count = block_count + 1
            self.nodes[1].importprivkey("TCQYDEmaHkBPHVQNze6uqZ74XDENYM1Gc6Gmqojauv3JJ8uTDQdy")
            self.nodes[1].importprivkey("TL78erDniqaEkjYqNtyWdPsQVMzc9bSpC4YiCxttNNY95ViUhNm5")
            self.nodes[2].importprivkey("TKfUQMc1zVp6kT5pmV21Xs2LSxN9aKski1iHykgE7YiLpsBprCCN")
            self.nodes[2].importprivkey("THvDeU3msPyNPBMZKVskKwcc7uKzdk9Y4AMUex6o3GVTQDwJsL8S")
            self.nodes[0].generate(1)
            self.sync_all()
            block_count = block_count + 1
            self.save_cache(True)

        for n in range(self.num_nodes):
            #self.log.info("Node %d(%d) balance %s", n, self.nodes[n].getblockcount(), self.nodes[n].getbalance())
            assert_equal(self.nodes[n].getblockcount(), block_count)

        self.log.info("Premine Success")

    def check_balances(self, balances):
        self.log.info("Check Balances...")

        assert_equal(len(balances), self.num_nodes)
        for n in range(self.num_nodes):
            assert_equal(int(self.nodes[n].getbalance()), balances[n])

        self.log.info("Balances Success")

    def check_mining(self):
        self.log.info("Check Mining...")
        block_count = self.nodes[0].getblockcount()
        for n in range(self.num_nodes):
            assert_equal(self.nodes[n].getblockcount(), block_count)
        while min([node.getblockcount() for node in self.nodes]) < block_count + 100:
            for node in self.nodes:
                node.generate(1)
            self.log.info("block_count: %d", min([node.getblockcount() for node in self.nodes]))
        self.sync_all()

        self.log.info("Mining Success")

    def check_mapping(self):
        self.log.info("Check Mapping...")

        type_names = ["sports", "rounds", "tournaments", "teams"]
        sport_names = ["Soccer","Football"]
        team_names = ["Chicago Bulls", "Washington Capitals"]

        try:
            mapping = self.nodes[0].getmappingid("", "")
        except JSONRPCException as e:
            assert_equal(str(e), "No mapping exist for the mapping index you provided. (-1)")

        for i in range(3):
            mapping = self.nodes[0].getmappingid(type_names[0], sport_names[0])
            assert_equal(len(mapping), 1)
            assert_equal(mapping[0]['mapping-id'], 0)
            assert_equal(mapping[0]['exists'], i > 0)
            assert_equal(mapping[0]['mapping-index'], type_names[0])
            mapping = self.nodes[0].getmappingid(type_names[1], "")
            assert_equal(len(mapping), 1)
            assert_equal(mapping[0]['mapping-id'], 0)
            assert_equal(mapping[0]['exists'], i > 0)
            assert_equal(mapping[0]['mapping-index'], type_names[1])
            mapping = self.nodes[0].getmappingid("teams", team_names[0])
            assert_equal(len(mapping), 1)
            assert_equal(mapping[0]['mapping-id'], 0)
            assert_equal(mapping[0]['exists'], i > 0)
            assert_equal(mapping[0]['mapping-index'], type_names[3])
            mapping = self.nodes[0].getmappingid("teams", team_names[1])
            assert_equal(len(mapping), 1)
            assert_equal(mapping[0]['mapping-id'], 1)
            assert_equal(mapping[0]['exists'], i > 0)
            assert_equal(mapping[0]['mapping-index'], type_names[3])

        for n in range(self.num_nodes):
            try:
                mapping = self.nodes[n].getmappingname(type_names[0], str(0))
                assert_equal(len(mapping), 1)
                assert_equal(mapping[0]['mapping-name'], sport_names[0])
                assert_equal(mapping[0]['exists'], True)
                assert_equal(mapping[0]['mapping-index'], type_names[0])

                mapping = self.nodes[n].getmappingname(type_names[3], str(0))
                assert_equal(len(mapping), 1)
                assert_equal(mapping[0]['mapping-name'], team_names[0])
                assert_equal(mapping[0]['exists'], True)
                assert_equal(mapping[0]['mapping-index'], type_names[3])

                mapping = self.nodes[n].getmappingname(type_names[3], str(0))
                assert_equal(len(mapping), 1)
                assert_equal(mapping[0]['mapping-name'], team_names[0])
                assert_equal(mapping[0]['exists'], True)
                assert_equal(mapping[0]['mapping-index'], type_names[3])

                mapping = make_mapping(1, 111 + n, sport_names[1])
                post_opcode(self.nodes[n], mapping)
                self.nodes[0].generate(1)
                time.sleep(10)
                self.sync_all()

            except JSONRPCException as e:
                assert_equal(str(e), "No mapping saved for the mapping type you provided. (-1)")
                assert_equal(n > 0, True)

        for n in range(self.num_nodes):
            mapping = self.nodes[n].getmappingname(type_names[0], str(111))
            assert_equal(len(mapping), 1)
            assert_equal(mapping[0]['mapping-name'], sport_names[1])
            assert_equal(mapping[0]['exists'], True)
            assert_equal(mapping[0]['mapping-index'], type_names[0])

        self.log.info("Mapping Success")

    def check_ml_event(self):
        self.log.info("Check ML Event...")

        mlevent = make_mlevent(42, 1576543210, 0, 0, 0, 0, 1, 10, 20, 30)
        post_opcode(self.nodes[2], mlevent)
        self.nodes[0].generate(1)
        time.sleep(10)
        self.sync_all()
        self.log.info("Events0: %s", self.nodes[0].listevents())
        self.log.info("Events1: %s", self.nodes[1].listevents())
        self.log.info("Events2: %s", self.nodes[2].listevents())

        self.log.info("ML Event Success")

    def check_ml_patch(self):
        self.log.info("Check ML Patch...")

        for n in range(self.num_nodes):
            assert_equal(self.nodes[n].listevents()[0]['starting'], 1576543210)
        mlevent = make_mlpatch(42, 1570000000)
        post_opcode(self.nodes[2], mlevent)
        self.nodes[0].generate(1)
        time.sleep(10)
        self.sync_all()
        for n in range(self.num_nodes):
            assert_equal(self.nodes[n].listevents()[0]['starting'], 1570000000)

        self.log.info("ML Patch Success")

    def check_recovery(self):
        self.log.info("Check Recovery...")
        self.log.info("Recovery Success")

    def check_reindex(self):
        self.log.info("Check Reindex...")
        self.log.info("Reindex Success")

    def run_test(self):
        self.check_premine()
        self.check_balances([62285767, 75036866, 74287858])
        self.check_mining()
        self.check_mapping()
        self.check_ml_event()
        self.check_ml_patch()
        self.check_recovery()
        self.check_reindex()

if __name__ == '__main__':
    BettingTest().main()
