// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "betting/bet.h"
#include "random.h"
#include "uint256.h"
#include "test/test_wagerr.h"

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

BOOST_AUTO_TEST_SUITE_END()
