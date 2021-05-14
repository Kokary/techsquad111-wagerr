// Copyright (c) 2020 The Wagerr developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <betting/bet_db.h>

std::string CMappingDB::ToTypeName(MappingType type)
{
    switch (type) {
    case sportMapping:
        return "sports";
    case roundMapping:
        return "rounds";
    case teamMapping:
        return "teamnames";
    case tournamentMapping:
        return "tournaments";
    case individualSportMapping:
        return "individualSports";
    case contenderMapping:
        return "contenders";
    }
    return "";
}

MappingType CMappingDB::FromTypeName(const std::string& name)
{
    if (name == ToTypeName(sportMapping)) {
        return sportMapping;
    }
    if (name == ToTypeName(roundMapping)) {
        return roundMapping;
    }
    if (name == ToTypeName(teamMapping)) {
        return teamMapping;
    }
    if (name == ToTypeName(tournamentMapping)) {
        return tournamentMapping;
    }
    if (name == ToTypeName(individualSportMapping)) {
        return individualSportMapping;
    }
    if (name == ToTypeName(contenderMapping)) {
        return contenderMapping;
    }
    return static_cast<MappingType>(-1);
}

bool CFieldEventDB::IsMarketOpen(const FieldBetMarketType type, const size_t contendersCount) {
    switch(type) {
        case outright:
            break; // always open
        case show:
            if (contendersCount < 8)
                return false;
            break;
        case place:
            if (contendersCount < 5)
                return false;
            break;
        default:
            return false;
    }
    return true;
}

void CFieldEventDB::ExtractDataFromTx(const CFieldEventTx& tx)
{
    nEventId = tx.nEventId;
    nStartTime = tx.nStartTime;
    nSport = tx.nSport;
    nTournament = tx.nTournament;
    nStage = tx.nStage;
    nGroupType = tx.nGroupType;
    nMarginPercent = tx.nMarginPercent;

    for (const auto& tx_contender : tx.mContendersInputOdds) {
        contenders[tx_contender.first] = ContenderInfo{tx_contender.second, 0, 0, 0, 0};
    }
}

void CFieldEventDB::ExtractDataFromTx(const CFieldUpdateOddsTx& tx)
{
    // Add new contenders and update current input odds
    for (const auto& tx_contender : tx.mContendersInputOdds) {
        contenders[tx_contender.first].nInputOdds = tx_contender.second;
        contenders[tx_contender.first].nOutrightOdds = 0;
        contenders[tx_contender.first].nPlaceOdds = 0;
        contenders[tx_contender.first].nShowOdds = 0;
        contenders[tx_contender.first].nModifier = 0;
    }
}

void CFieldEventDB::ExtractDataFromTx(const CFieldUpdateMarginTx& tx)
{
    // Update margin percent
    nMarginPercent = tx.nMarginPercent;
}

// ReCalculate Market specific Odds
void CFieldEventDB::CalcOdds()
{

    size_t noneZeroCount = 0;
    std::map<uint32_t, uint32_t> mContendersIdsOdds;
    for (const auto& contender : contenders) {
        if (contender.second.nInputOdds != 0) noneZeroCount++;
        mContendersIdsOdds[contender.first] = contender.second.nInputOdds;
    }

    bool isPlaceOpen = CFieldEventDB::IsMarketOpen(place, noneZeroCount);
    bool isShowOpen = CFieldEventDB::IsMarketOpen(show, noneZeroCount);

    double mPlace = 0.0, mShow = 0.0;
    double XPlace = 0.0, XShow = 0.0;

    std::vector<std::pair<uint32_t, uint32_t>> vContendersPlaceOddsMods;
    if (isPlaceOpen) {
        switch (nGroupType) {
        case FieldEventGroupType::animalRacing:
        {
            const double lambda = GetLambda(noneZeroCount);
            for (const auto& cont : mContendersIdsOdds) {
                if (cont.second == 0)
                    vContendersPlaceOddsMods.emplace_back(0, 0);
                else
                    vContendersPlaceOddsMods.emplace_back(
                        CalculateAnimalPlaceOdds(cont.first, lambda, mContendersIdsOdds),
                        contenders.at(cont.first).nModifier
                    );
            }

            break;
        }
        case FieldEventGroupType::other:
        {
            // Evaluates the probability of the provided player - with index idx - arriving amongst the first 3
            std::vector<std::vector<uint32_t>> vIdsPermutations;
            Permutations2(mContendersIdsOdds, vIdsPermutations);
            for (const auto& cont : mContendersIdsOdds) {
                if (cont.second == 0)
                    vContendersPlaceOddsMods.emplace_back(0, 0);
                else
                    vContendersPlaceOddsMods.emplace_back(
                        CalculateOddsInFirstN(cont.first, vIdsPermutations, mContendersIdsOdds),
                        contenders.at(cont.first).nModifier
                    );
            }

            break;
        }
        default:
            break;
        }

        double realMarginIn = (static_cast<double>(nMarginPercent) / 100.0) * 2.0;
        mPlace = CalculateM(vContendersPlaceOddsMods, realMarginIn);
        XPlace = CalculateX(vContendersPlaceOddsMods, realMarginIn);

        LogPrint("wagerr", "Place m = %f X = %f\n", mShow, XShow);
    }

    std::vector<std::pair<uint32_t, uint32_t>> vContendersShowOddsMods;
    if (isShowOpen) {
        switch (nGroupType) {
        case FieldEventGroupType::animalRacing:
        {
            const double lambda = GetLambda(noneZeroCount);
            const double rho = GetRHO(noneZeroCount);
            for (const auto& cont : mContendersIdsOdds) {
                if (cont.second == 0)
                    vContendersShowOddsMods.emplace_back(0, 0);
                else
                    vContendersShowOddsMods.emplace_back(
                        CalculateAnimalShowOdds(cont.first, lambda, rho, mContendersIdsOdds),
                        contenders.at(cont.first).nModifier
                    );
            }

            break;
        }
        case FieldEventGroupType::other:
        {
            // Evaluates the probability of the provided player - with index idx - arriving amongst the first 3
            std::vector<std::vector<uint32_t>> vIdsPermutations;
            Permutations3(mContendersIdsOdds, vIdsPermutations);
            for (const auto& cont : mContendersIdsOdds) {
                if (cont.second == 0)
                    vContendersShowOddsMods.emplace_back(0, 0);
                else
                    vContendersShowOddsMods.emplace_back(
                        CalculateOddsInFirstN(cont.first, vIdsPermutations, mContendersIdsOdds),
                        contenders.at(cont.first).nModifier
                    );
            }

            break;
        }
        default:
            break;
        }

        double realMarginIn = (static_cast<double>(nMarginPercent) / 100.0) * 3.0;
        mShow = CalculateM(vContendersShowOddsMods, realMarginIn);
        XShow = CalculateX(vContendersShowOddsMods, realMarginIn);

        LogPrint("wagerr", "Show m = %f X = %f\n", mShow, XShow);
    }

    // Update all contenders odds
    size_t i = 0;
    for (auto& contender : contenders) {
        uint32_t modifier = contender.second.nModifier;
        contender.second.nPlaceOdds = isPlaceOpen ? CalculateMarketOdds(XPlace, mPlace, vContendersPlaceOddsMods[i].first, modifier) : 0;
        contender.second.nShowOdds = isShowOpen ? CalculateMarketOdds(XShow, mShow, vContendersShowOddsMods[i].first, modifier) : 0;
        i++;
    }
}

double CFieldEventDB::GetLambda(const size_t ContendersSize) {
    switch (ContendersSize) {
        case 3: return 0.6667;
        case 4:	return 0.6996;
        case 5:	return 0.7207;
        case 6:	return 0.7359;
        case 7:	return 0.7475;
        case 8:	return 0.7569;
        case 9:	return 0.7648;
        case 10: return 0.7714;
        case 11: return 0.7771;
        case 12: return 0.7822;
        case 13: return 0.7867;
        case 14: return 0.7907;
        default: return 0.76;
    }
}

double CFieldEventDB::GetRHO(const size_t ContendersSize) {
    switch (ContendersSize) {
        case 4: return 0.5336;
        case 5: return 0.5703;
        case 6: return 0.5952;
        case 7: return 0.6138;
        case 8: return 0.6285;
        case 9: return 0.6406;
        case 10: return 0.6508;
        case 11: return 0.6596;
        case 12: return 0.6672;
        case 13: return 0.6741;
        case 14: return 0.6802;
        default: return 0.62;
    }
}

uint32_t CFieldEventDB::CalculateAnimalPlaceOdds(const uint32_t idx, const double lambda, const std::map<uint32_t, uint32_t>& mContendersOutrightOdds)
{
    // First place
    double resultProb = 1.0 / (static_cast<double>(mContendersOutrightOdds.at(idx)) / BET_ODDSDIVISOR);
    // Second place
    for (auto& contender : mContendersOutrightOdds) {
        if (contender.second == 0) continue;
        if (contender.first == idx) continue;

        const auto idx1 = contender.first;
        const auto idx2 = idx;

        double den = 0.0;
        for (auto it = mContendersOutrightOdds.begin(); it != mContendersOutrightOdds.end(); it++) {
            if (it->second == 0) continue;
            if (it->first == contender.first) continue;
            const double prob = 1.0 / (static_cast<double>(it->second) / BET_ODDSDIVISOR);
            den += pow(prob, lambda);
        }

        const double probIdx1 = 1.0 / (static_cast<double>(mContendersOutrightOdds.at(idx1)) / BET_ODDSDIVISOR);
        const double probIdx2 = 1.0 / (static_cast<double>(mContendersOutrightOdds.at(idx2)) / BET_ODDSDIVISOR);
        resultProb += probIdx1 * pow(probIdx2, lambda) / den;
    }

    return static_cast<uint32_t>((1.0 / resultProb) * BET_ODDSDIVISOR);
}

uint32_t CFieldEventDB::CalculateAnimalShowOdds(const uint32_t idx, const double lambda, const double rho, const std::map<uint32_t, uint32_t>& mContendersOutrightOdds)
{
    // First two places
    double resultProb = 1.0 / (static_cast<double>(CalculateAnimalPlaceOdds(idx, lambda, mContendersOutrightOdds)) / BET_ODDSDIVISOR);
    // Third place
    for (auto it_i = mContendersOutrightOdds.begin(); it_i != mContendersOutrightOdds.end(); it_i++) {
        if (it_i->second == 0) continue;
        if (it_i->first == idx) continue;
        for (auto it_j = mContendersOutrightOdds.begin(); it_j != mContendersOutrightOdds.end(); it_j++) {
            if (it_j->second == 0) continue;
            if (it_j->first == idx) continue;
            if (it_j->first == it_i->first) continue;

            const auto idx1 = it_i->first;
            const auto idx2 = it_j->first;
            const auto idx3 = idx;

            double den1 = 0.0;
            for (auto it_k = mContendersOutrightOdds.begin(); it_k != mContendersOutrightOdds.end(); it_k++) {
                if (it_k->second == 0) continue;
                if (it_k->first == idx1) continue;
                double prob = 1.0 / (static_cast<double>(it_k->second) / BET_ODDSDIVISOR);
                den1 += pow(prob, lambda);
            }

            double den2 = 0.0;
            for (auto it_k = mContendersOutrightOdds.begin(); it_k != mContendersOutrightOdds.end(); it_k++) {
                if (it_k->second == 0) continue;
                if (it_k->first == idx1) continue;
                if (it_k->first == idx2) continue;
                double prob = 1.0 / (static_cast<double>(it_k->second) / BET_ODDSDIVISOR);
                den2 += pow(prob, rho);
            }

            double probIdx1 = 1.0 / (static_cast<double>(mContendersOutrightOdds.at(idx1)) / BET_ODDSDIVISOR);
            double probIdx2 = 1.0 / (static_cast<double>(mContendersOutrightOdds.at(idx2)) / BET_ODDSDIVISOR);
            double probIdx3 = 1.0 / (static_cast<double>(mContendersOutrightOdds.at(idx3)) / BET_ODDSDIVISOR);
            resultProb += probIdx1 * pow(probIdx2, lambda) * pow(probIdx3, rho) / (den1 * den2);
        }
    }

    return static_cast<uint32_t>((1.0 / resultProb) * BET_ODDSDIVISOR);
}

void CFieldEventDB::Permutations2(const std::map<uint32_t, uint32_t>& mContendersOdds, std::vector<std::vector<uint32_t>>& perms)
{
    for (const auto& cont_i : mContendersOdds) {
        if (cont_i.second == 0) continue;
        for (const auto& cont_j : mContendersOdds) {
            if (cont_j.second == 0) continue;
            if (cont_i.first == cont_j.first) continue;
            perms.push_back({cont_i.first, cont_j.first});
        }
    }
}

void CFieldEventDB::Permutations3(const std::map<uint32_t, uint32_t>& mContendersOdds, std::vector<std::vector<uint32_t>>& perms)
{
    // TODO: optimize this
    for (const auto& cont_i : mContendersOdds) {
        if (cont_i.second == 0) continue;
        for (const auto& cont_j : mContendersOdds) {
            if (cont_j.second == 0) continue;
            if (cont_i.first == cont_j.first) continue;
            for (const auto& cont_k : mContendersOdds) {
                if (cont_k.second == 0) continue;
                if (cont_k.first == cont_i.first) continue;
                if (cont_k.first == cont_j.first) continue;
                perms.push_back({cont_i.first, cont_j.first, cont_k.first});
            }
        }
    }
}

uint32_t CFieldEventDB::CalculateOddsInFirstN(const uint32_t idx, const std::vector<std::vector<uint32_t>>& permutations, const std::map<uint32_t, uint32_t>& mContendersOutrightOdds)
{
    double resultProb = 0.0;
    for (const auto& perm : permutations) {
        bool isInArr = false;
        for (const auto val : perm) {
            if (val == idx) {
                isInArr = true;
                break;
            }
        }

        if (!isInArr) continue;

        std::vector<double> currentProbs;
        for (uint32_t cont_id : perm) {
            currentProbs.push_back(1.0 / (static_cast<double>(mContendersOutrightOdds.at(cont_id)) / BET_ODDSDIVISOR));
        }

        double evalExactOrder = 1.0;
        for (const auto prob : currentProbs) {
            evalExactOrder *= prob;
        }

        double den = 1;
        for (size_t i = 0; i < currentProbs.size(); i++) {
            double q = 1;
            for (size_t j = 0; j < i; j++) {
                q = q - currentProbs[j];
            }
            den = den * q;
        }

        resultProb = resultProb + (evalExactOrder / den);
    }

    return static_cast<uint32_t>((1.0 / resultProb) * BET_ODDSDIVISOR);
}

double CFieldEventDB::CalculateX(const std::vector<std::pair<uint32_t, uint32_t>>& vContendersOddsMods, const double realMarginIn)
{
    const double h = 0.000001;
    double X = 1;
    double Xd = X + h;
    double Xs = X - h;

    for (const auto& cont : vContendersOddsMods) {
        if (cont.first == 0) continue;

        double f = 0.0, fd = 0.0, fs = 0.0;
        for (const auto& cont : vContendersOddsMods) {
            const auto& outrightOdds = static_cast<double>(cont.first) / BET_ODDSDIVISOR;
            const auto& modifier = static_cast<double>(cont.second) / MODIFIER_DIVISOR;

            if (outrightOdds == 0.0) continue;

            f += 1.0 / (1.0 + (X + modifier) * (outrightOdds - 1.0));
            fd += 1.0 / (1.0 + (Xd + modifier) * (outrightOdds - 1.0));
            fs += 1.0 / (1.0 + (Xs + modifier) * (outrightOdds - 1.0));
        }

        f -= realMarginIn;
        fd -= realMarginIn;
        fs -= realMarginIn;

        double der = (fd - fs) / h;
        X = X - (f / der);
        Xd = X + h;
        Xs = X - h;
    }

    return X;
}

double CFieldEventDB::CalculateM(const std::vector<std::pair<uint32_t, uint32_t>>& vContendersOddsMods, const double realMarginIn)
{
    const double h = 0.000001;
    double m = 1;
    double md = m + h;
    double ms = m - h;

    for (const auto& cont : vContendersOddsMods) {
        if (cont.first == 0) continue;

        double f = 0.0, fd = 0.0, fs = 0.0;
        for (const auto& cont : vContendersOddsMods) {
            const auto& outrightOdds = static_cast<double>(cont.first) / BET_ODDSDIVISOR;
            const auto& modifier = static_cast<double>(cont.second) / MODIFIER_DIVISOR;

            if (outrightOdds == 0.0) continue;

            double outrightP = 1.0 / outrightOdds;
            f += pow(outrightP, (m + modifier));
            fd += pow(outrightP, (md + modifier));
            fs += pow(outrightP, (ms + modifier));
        }

        f -= realMarginIn;
        fd -= realMarginIn;
        fs -= realMarginIn;

        double der = (fd - fs) / h;
        m = m - (f / der);
        md = m + h;
        ms = m - h;
    }

    return m;
}

uint32_t CFieldEventDB::CalculateMarketOdds(const double X, const double m, const uint32_t outrightOdds, const uint16_t modifier)
{
    uint32_t outputOdds = 0;
    if (outrightOdds == 0) {
        return outputOdds;
    }

    const double o = static_cast<double>(outrightOdds) / BET_ODDSDIVISOR;
    double oddsX = 1.0 + (X + modifier) * (o - 1.0);

    const double p = 1.0 / o;
    double oddsM = pow(p, (-m-modifier));

    outputOdds = static_cast<uint32_t>((oddsM + oddsX) / 2.0 * BET_ODDSDIVISOR);
    return outputOdds;
}

/*
 * CBettingDB methods
 */

CFlushableStorageKV& CBettingDB::GetDb()
{
    return db;
}

bool CBettingDB::Flush()
{
    return db.Flush();
}

std::unique_ptr<CStorageKVIterator> CBettingDB::NewIterator()
{
    return db.NewIterator();
}

unsigned int CBettingDB::GetCacheSize()
{
    return db.GetCacheSize();
}

unsigned int CBettingDB::GetCacheSizeBytesToWrite()
{
    return db.GetCacheSizeBytesToWrite();
}

size_t CBettingDB::dbWrapperCacheSize()
{
    return 10 << 20;
}

std::string CBettingDB::MakeDbPath(const char* name)
{
using namespace boost::filesystem;

    std::string result{};
    path dir{GetDataDir()};

    dir /= "betting";
    dir /= name;

    if (boost::filesystem::is_directory(dir) || boost::filesystem::create_directories(dir) ) {
        result = boost::to_string(dir);
        result.erase(0, 1);
        result.erase(result.size() - 1);
    }

    return result;
}

/*
 * CBettingsView methods
 */

// copy constructor for creating DB cache
CBettingsView::CBettingsView(CBettingsView* phr) {
    mappings = MakeUnique<CBettingDB>(*phr->mappings.get());
    results = MakeUnique<CBettingDB>(*phr->results.get());
    events = MakeUnique<CBettingDB>(*phr->events.get());
    bets = MakeUnique<CBettingDB>(*phr->bets.get());
    fieldEvents = MakeUnique<CBettingDB>(*phr->fieldEvents.get());
    fieldResults = MakeUnique<CBettingDB>(*phr->fieldResults.get());
    fieldBets = MakeUnique<CBettingDB>(*phr->fieldBets.get());
    undos = MakeUnique<CBettingDB>(*phr->undos.get());
    payoutsInfo = MakeUnique<CBettingDB>(*phr->payoutsInfo.get());
    quickGamesBets = MakeUnique<CBettingDB>(*phr->quickGamesBets.get());
    chainGamesLottoEvents = MakeUnique<CBettingDB>(*phr->chainGamesLottoEvents.get());
    chainGamesLottoBets = MakeUnique<CBettingDB>(*phr->chainGamesLottoBets.get());
    chainGamesLottoResults = MakeUnique<CBettingDB>(*phr->chainGamesLottoResults.get());
    failedBettingTxs = MakeUnique<CBettingDB>(*phr->failedBettingTxs.get());
}

bool CBettingsView::Flush() {
    return mappings->Flush() &&
            results->Flush() &&
            events->Flush() &&
            bets->Flush() &&
            fieldEvents->Flush() &&
            fieldResults->Flush() &&
            fieldBets->Flush() &&
            undos->Flush() &&
            payoutsInfo->Flush() &&
            quickGamesBets->Flush() &&
            chainGamesLottoEvents->Flush() &&
            chainGamesLottoBets->Flush() &&
            chainGamesLottoResults->Flush();
            failedBettingTxs->Flush();
}

unsigned int CBettingsView::GetCacheSize() {
    return mappings->GetCacheSize() +
            results->GetCacheSize() +
            events->GetCacheSize() +
            bets->GetCacheSize() +
            fieldEvents->GetCacheSize() +
            fieldResults->GetCacheSize() +
            fieldBets->GetCacheSize() +
            undos->GetCacheSize() +
            payoutsInfo->GetCacheSize() +
            quickGamesBets->GetCacheSize() +
            chainGamesLottoEvents->GetCacheSize() +
            chainGamesLottoBets->GetCacheSize() +
            chainGamesLottoResults->GetCacheSize() +
            failedBettingTxs->GetCacheSize();
}

unsigned int CBettingsView::GetCacheSizeBytesToWrite() {
    return mappings->GetCacheSizeBytesToWrite() +
            results->GetCacheSizeBytesToWrite() +
            events->GetCacheSizeBytesToWrite() +
            bets->GetCacheSizeBytesToWrite() +
            fieldEvents->GetCacheSizeBytesToWrite() +
            fieldResults->GetCacheSizeBytesToWrite() +
            fieldBets->GetCacheSizeBytesToWrite() +
            undos->GetCacheSizeBytesToWrite() +
            payoutsInfo->GetCacheSizeBytesToWrite() +
            quickGamesBets->GetCacheSizeBytesToWrite() +
            chainGamesLottoEvents->GetCacheSizeBytesToWrite() +
            chainGamesLottoBets->GetCacheSizeBytesToWrite() +
            chainGamesLottoResults->GetCacheSizeBytesToWrite() +
            failedBettingTxs->GetCacheSizeBytesToWrite();
}

void CBettingsView::SetLastHeight(uint32_t height) {
    if (!undos->Exists(std::string("LastHeight"))) {
        undos->Write(std::string("LastHeight"), height);
    }
    else {
        undos->Update(std::string("LastHeight"), height);
    }
}

uint32_t CBettingsView::GetLastHeight() {
    uint32_t height;
    if (!undos->Read(std::string("LastHeight"), height))
        return 0;
    return height;
}

bool CBettingsView::SaveBettingUndo(const BettingUndoKey& key, std::vector<CBettingUndoDB> vUndos) {
    assert(!undos->Exists(key));
    return undos->Write(key, vUndos);
}

bool CBettingsView::EraseBettingUndo(const BettingUndoKey& key) {
    return undos->Erase(key);
}

std::vector<CBettingUndoDB> CBettingsView::GetBettingUndo(const BettingUndoKey& key) {
    std::vector<CBettingUndoDB> vUndos;
    if (undos->Read(key, vUndos))
        return vUndos;
    else
        return std::vector<CBettingUndoDB>{};
}

bool CBettingsView::ExistsBettingUndo(const BettingUndoKey& key) {
    return undos->Exists(key);
}

void CBettingsView::PruneOlderUndos(const uint32_t height) {
    static std::vector<unsigned char> lastHeightKey = CBettingDB::DbTypeToBytes(std::string("LastHeight"));
    std::vector<CBettingUndoDB> vUndos;
    BettingUndoKey key;
    std::string str;
    auto it = undos->NewIterator();
    std::vector<BettingUndoKey> vKeysToDelete;
    for (it->Seek(std::vector<unsigned char>{}); it->Valid(); it->Next()) {
        // check that key is serialized "LastHeight" key and skip if true
        if (it->Key() == lastHeightKey) {
            continue;
        }
        CBettingDB::BytesToDbType(it->Key(), key);
        CBettingDB::BytesToDbType(it->Value(), vUndos);
        if (vUndos[0].height < height) {
            vKeysToDelete.push_back(key);
        }
    }
    for (auto && key : vKeysToDelete) {
        undos->Erase(key);
    }
}

bool CBettingsView::SaveFailedTx(const FailedTxKey& key) {
    assert(!failedBettingTxs->Exists(key));
    return failedBettingTxs->Write(key, 0);
}

bool CBettingsView::ExistFailedTx(const FailedTxKey& key) {
    return failedBettingTxs->Exists(key);
}

bool CBettingsView::EraseFailedTx(const FailedTxKey& key) {
    return failedBettingTxs->Erase(key);
}