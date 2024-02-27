#include "stdafx.h"

#include <assert.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <ratio>
#include <string>
#include <vector>

// Functionality tets
// Edgecases
// Performance

using QBRecordCollection = qb::Collection<qb::Record<4>>;

QBRecordCollection QBFindMatchingRecords(const QBRecordCollection& records, const std::string& columnName, const std::string& matchString) {
    QBRecordCollection ret;
    if (records.empty()) return ret;
    return ret;
}

void debug_PrintCollection(const QBRecordCollection& c) {
    for (const auto& [id, r] : c.records) {
        std::cout << "{ '" << id << "': { ";
        size_t i = 0;
        for (i = 0; i < r.columns.size() - 1; i++) {
            std::cout << c.columnNames[i] << ": " << r.columns[i]->toStr() << ", ";
        }
        std::cout << c.columnNames[i] << ": " << r.columns[i]->toStr();
        std::cout << "}\n";
    }
    std::cout << std::endl;
}

static constexpr int32_t TEST_CASES = 100000;
static constexpr int32_t TEST_RND_ELEMENTS = 1000;
static std::array<std::string, TEST_RND_ELEMENTS> rndStrings;
static std::array<int64_t, TEST_RND_ELEMENTS> rndLongs;

static base_impl::QBRecordCollection testBaseImplementation;
static QBRecordCollection testQBImplementation;

void beforeTests() {
    srand(uint32_t(time(0)));

    for (int32_t i = 0; i < TEST_RND_ELEMENTS; i++) {
        int32_t strSize = core::genRndInt32(5, 29);
        rndStrings[i] = core::genRndStr(strSize);
    }

    for (int32_t i = 0; i < TEST_RND_ELEMENTS; i++) {
        rndLongs[i] = core::genRndInt64(0, 100);
    }

    static int uniqueIdx = 0;

    // Populate the base implementation
    {
        auto& c = testBaseImplementation;

        c.reserve(TEST_CASES);

        // Make sure all generated random elemnts are present in the collection:
        for (int32_t i = 0; i < TEST_RND_ELEMENTS; i++) {
            base_impl::QBRecord r;
            r.column0 = uniqueIdx++;
            r.column1 = rndStrings[i];
            r.column2 = rndLongs[i];
            r.column3 = rndStrings[TEST_RND_ELEMENTS - i - 1];
            c.push_back(r);
        }

        // Fill the rest of the collection with random data.
        for (int32_t i = 0; i < (TEST_CASES - TEST_RND_ELEMENTS); i++) {
            base_impl::QBRecord r;
            int32_t rndIdx1 = core::genRndInt32(0, TEST_RND_ELEMENTS - 1);
            int32_t rndIdx2 = core::genRndInt32(0, TEST_RND_ELEMENTS - 1);
            int32_t rndIdx3 = core::genRndInt32(0, TEST_RND_ELEMENTS - 1);
            r.column0 = uniqueIdx++;
            r.column1 = rndStrings[rndIdx1];
            r.column2 = rndLongs[rndIdx2];
            r.column3 = rndStrings[rndIdx3];
            c.push_back(r);
        }
    }

    // Populate the qb implementation
    {
        auto& c = testQBImplementation;

        c.columnNames = { "column0", "column1", "column2", "column3" };
        c.reserve(TEST_CASES);

        // Make sure all generated random elemnts are present in the collection:
        for (int32_t i = 0; i < TEST_RND_ELEMENTS; i++) {
            qb::Record<4> r;
            r.columns[0] = std::make_unique<qb::Int32RecordValue>(uniqueIdx);
            r.columns[1] = std::make_unique<qb::StrRecordValue>(rndStrings[i]);
            r.columns[2] = std::make_unique<qb::Int64RecordValue>(rndLongs[i]);
            r.columns[3] = std::make_unique<qb::StrRecordValue>(rndStrings[TEST_RND_ELEMENTS - i - 1]);
            c.insert(uniqueIdx, std::move(r));
            uniqueIdx++;
        }

        // Fill the rest of the collection with random data.
        for (int32_t i = 0; i < (TEST_CASES - TEST_RND_ELEMENTS); i++) {
            qb::Record<4> r;
            int32_t rndIdx1 = core::genRndInt32(0, TEST_RND_ELEMENTS - 1);
            int32_t rndIdx2 = core::genRndInt32(0, TEST_RND_ELEMENTS - 1);
            int32_t rndIdx3 = core::genRndInt32(0, TEST_RND_ELEMENTS - 1);
            r.columns[0] = std::make_unique<qb::Int32RecordValue>(uniqueIdx);
            r.columns[1] = std::make_unique<qb::StrRecordValue>(rndStrings[rndIdx1]);
            r.columns[2] = std::make_unique<qb::Int64RecordValue>(rndLongs[rndIdx2]);
            r.columns[3] = std::make_unique<qb::StrRecordValue>(rndStrings[rndIdx3]);
            c.insert(uniqueIdx, std::move(r));
            uniqueIdx++;
        }

        debug_PrintCollection(c);
    }
}

void runFunctionalTests() {
}

void runAllTestCases() {
    beforeTests();

    runFunctionalTests();
}

int main(int argc, _TCHAR* argv[]) {
    runAllTestCases();
	return 0;
}

