// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "betting/bet.h"
#include "random.h"
#include "uint256.h"
#include "test/test_wagerr.h"
#include <cstdlib>
#include <cstdio>

#include <vector>
#include <map>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem/operations.hpp>

BOOST_FIXTURE_TEST_SUITE(betting_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(betting_mappings_test)
{
    boost::filesystem::remove_all(CMappingsDB::GetDbName());
    CMapping mapping{};
    CMappingsDB db{};
    MappingsIndex mappingsIndex{};

    BOOST_CHECK(!db.Read(teamMapping, mappingsIndex));
    BOOST_CHECK(mappingsIndex.size() == 0);
    BOOST_CHECK(!db.Read(sportMapping, mappingsIndex));
    BOOST_CHECK(mappingsIndex.size() == 0);
    BOOST_CHECK(!db.Read(roundMapping, mappingsIndex));
    BOOST_CHECK(mappingsIndex.size() == 0);
    BOOST_CHECK(!db.Read(tournamentMapping, mappingsIndex));
    BOOST_CHECK(mappingsIndex.size() == 0);

    mapping.nId = 1;
    mapping.nMType = teamMapping;
    mapping.sName = "Football";
    mapping.nVersion = PROTOCOL_VERSION;
    db.Save(mapping);
    BOOST_CHECK(db.Read(teamMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 1);
    BOOST_CHECK(!db.Read(sportMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 1);
    BOOST_CHECK(!db.Read(roundMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 1);
    BOOST_CHECK(!db.Read(tournamentMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 1);

    mapping.nId = 2;
    mapping.nMType = sportMapping;
    mapping.sName = "Hockey";
    mapping.nVersion = PROTOCOL_VERSION;
    db.Save(mapping);
    BOOST_CHECK(db.Read(teamMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 0);
    BOOST_CHECK(db.Read(sportMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 1);
    BOOST_CHECK(!db.Read(roundMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 1);
    BOOST_CHECK(!db.Read(tournamentMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 1);

    mapping.nId = 4;
    mapping.nMType = roundMapping;
    mapping.sName = "Kickboxing";
    mapping.nVersion = PROTOCOL_VERSION;
    db.Save(mapping);
    BOOST_CHECK(db.Read(teamMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 0);
    BOOST_CHECK(db.Read(sportMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 0);
    BOOST_CHECK(db.Read(roundMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 1);
    BOOST_CHECK(!db.Read(tournamentMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 1);

    mapping.nId = 8;
    mapping.nMType = tournamentMapping;
    mapping.sName = "MortalCombat";
    mapping.nVersion = PROTOCOL_VERSION;
    db.Save(mapping);
    BOOST_CHECK(db.Read(teamMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 0);
    BOOST_CHECK(db.Read(sportMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 0);
    BOOST_CHECK(db.Read(roundMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 0);
    BOOST_CHECK(db.Read(tournamentMapping, mappingsIndex));
    BOOST_CHECK_EQUAL(mappingsIndex.size(), 1);
    BOOST_CHECK_EQUAL(mappingsIndex.count(mapping.nId), 1);
}

BOOST_AUTO_TEST_CASE(betting_events_test)
{
    boost::filesystem::remove_all(CEventsDB::GetDbName());
    CEventsDB db{};
    EventsIndex eventsIndex{};
    CPeerlessEvent plEvent{};

    BOOST_CHECK(!db.Read(eventsIndex));

    plEvent.nEventId = 1;
    db.Save(plEvent);
    BOOST_CHECK(db.Read(eventsIndex));
    BOOST_CHECK(eventsIndex.size() == 1);
    BOOST_CHECK(eventsIndex.count(1) == 1);

    for (auto i = 2; i < 1000; i++) {
        plEvent.nEventId = i;
        db.Save(plEvent);
        BOOST_CHECK(db.Read(eventsIndex));
        BOOST_CHECK(eventsIndex.size() == i);
        BOOST_CHECK(eventsIndex.count(i) == 1);
    }
}

BOOST_AUTO_TEST_CASE(betting_results_test)
{
    boost::filesystem::remove_all(CResultsDB::GetDbName());
    CResultsDB db{};
    ResultsIndex resultsIndex{};
    CPeerlessResult plResult{};

    BOOST_CHECK(!db.Read(resultsIndex));

    plResult.nEventId = 1;
    db.Save(plResult);
    BOOST_CHECK(db.Read(resultsIndex));
    BOOST_CHECK(resultsIndex.size() == 1);
    BOOST_CHECK(resultsIndex.count(1) == 1);
}

BOOST_AUTO_TEST_CASE(betting_dbcheck_test)
{
    boost::filesystem::remove_all(CEventsDB::GetDbName());
    std::srand(std::time(nullptr));
    int nEventCount = 0;

    for (auto i{0}; i < 2; i++) {
        CEventsDB db{};
        CPeerlessEvent plEvent{};
        EventsIndex eventsIndex{};

        if (i == 0) {
            nEventCount = 1000 + std::rand() % 1000;
            BOOST_CHECK(!db.Read(eventsIndex));
            for (auto j{0}; j < nEventCount; j++) {
                plEvent.nEventId = j;
                db.Save(plEvent);
            }
        } else {
            BOOST_CHECK(db.Read(eventsIndex));
            for (auto j{0}; j < nEventCount; j++) {
                BOOST_CHECK(eventsIndex.count(j) == 1);
                BOOST_CHECK(eventsIndex[j].nEventId == j);
            }
        }
    }
}

/*BOOST_AUTO_TEST_CASE(betting_crash_test)
{
    const std::string envFlag{"WAGGER_UNIT_TEST_BETTING_CRASH"};

//    for (auto i{0}; i < 40; i++) {
//        const char c{static_cast<char>(1.0 * ('z' - 'a') * std::rand() / RAND_MAX)};
//        envFlag.append(1, c + 'a');
//    }

    if (std::getenv(envFlag.c_str()) == nullptr) {
        std::string cmdline{"env "};
//        const std::string flagFile{};
        cmdline.append(envFlag);
        cmdline.append("=1 ");
//        cmdline.append(1, '=');
//        cmdline.append(flagFile);
//        cmdline.append(1, ' ');
        cmdline.append(boost::unit_test::framework::master_test_suite().argv[0]);
//        cmdline.append(1, ' ');
        cmdline.append(" --run_test=betting_tests");
//        boost::filesystem::ofstream(boost::filesystem::path(flagFile));
        boost::filesystem::remove_all(CEventsDB::GetDbName());
        runCommand(cmdline);

        CEventsDB db{};
        EventsIndex eventsIndex{};
        CPeerlessEvent plEvent{};
        BOOST_CHECK(db.Read(eventsIndex));
        BOOST_CHECK(eventsIndex.size() > 0);
        BOOST_CHECK(eventsIndex.size() < 1000);
        for (const auto& pair : eventsIndex) {
            BOOST_CHECK(pair.first < 1000);
            BOOST_CHECK(pair.first == pair.second.nEventId);
        }
        BOOST_TEST_MESSAGE("Fork Complete");
    } else {
        BOOST_TEST_MESSAGE("Running Fork");
        CEventsDB db{};
        EventsIndex eventsIndex{};
        CPeerlessEvent plEvent{};
        BOOST_TEST_MESSAGE("Reading DB");
        BOOST_CHECK(!db.Read(eventsIndex));
        std::srand(std::time(nullptr));
        BOOST_TEST_MESSAGE("Writing DB");
        for (auto i = 0; i < 1000; i++) {
            plEvent.nEventId = i;
            db.Save(plEvent);
            if (i > std::rand() % 333 + 666) {
                int* p = nullptr;
                *p = 42;
            }
        }
    }

//    std::thread thread{[]() {
//            CEventsDB db{};
//            EventsIndex eventsIndex{};
//            CPeerlessEvent plEvent{};

//            BOOST_CHECK(!db.Read(eventsIndex));

//            for (auto i = 1; i < 100; i++) {
//                plEvent.nEventId = i;
//                db.Save(plEvent);
//            }
//    }};
//    thread.join();
}*/

BOOST_AUTO_TEST_SUITE_END()
