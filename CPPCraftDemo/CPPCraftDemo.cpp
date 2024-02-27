#include "stdafx.h"

#include <assert.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <ratio>
#include <string>
#include <vector>

using QBRecordCollection = qb::Collection<4>;

QBRecordCollection QBFindMatchingRecords(const QBRecordCollection& records, const std::string& columnName, const std::string& matchString) {
    bool ok = false;
    QBRecordCollection ret = records.match(columnName, matchString, ok);
    assert(ok);
    return ret;
}

static constexpr int32_t TEST_CASES = 100000;
static constexpr int32_t TEST_RND_ELEMENTS = 1000;
static std::array<std::string, TEST_RND_ELEMENTS> rndStrings;
static std::array<int64_t, TEST_RND_ELEMENTS> rndLongs;

static base_impl::QBRecordCollection testBaseImplementation;
static QBRecordCollection testQBImplementation ({"column0", "column1", "column2", "column3"});

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
        bool ok = false;

        ok = c.createIndex("column1", qb::RecordValueType::String);
        assert(ok);
        ok = c.createIndex("column2", qb::RecordValueType::Int64);
        assert(ok);
        ok = c.createIndex("column3", qb::RecordValueType::String);
        assert(ok);

        c.reserve(TEST_CASES);

        // Make sure all generated random elemnts are present in the collection:
        for (int32_t i = 0; i < TEST_RND_ELEMENTS; i++) {
            qb::Record<4> r;
            r.columns[0] = std::make_unique<qb::Int32RecordValue>(uniqueIdx);
            r.columns[1] = std::make_unique<qb::StrRecordValue>(rndStrings[i]);
            r.columns[2] = std::make_unique<qb::Int64RecordValue>(rndLongs[i]);
            r.columns[3] = std::make_unique<qb::StrRecordValue>(rndStrings[TEST_RND_ELEMENTS - i - 1]);
            c.insertRecord(std::move(r));
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
            c.insertRecord(std::move(r));
            uniqueIdx++;
        }
    }
}

//void runFunctionalTests() {
//    {
//        QBRecordCollection c({ "column0", "column1", "column2", "column3" });
//
//        bool ok = false;
//        ok = c.createIndex("column1", qb::RecordValueType::String);
//        assert(ok);
//        ok = c.createIndex("column2", qb::RecordValueType::Int64);
//        assert(ok);
//        ok = c.createIndex("column3", qb::RecordValueType::String);
//        assert(ok);
//
//        {
//            auto res = c.match("column0", "0", ok);
//            res.debug_PrintCollection();
//        }
//
//        ok = c.insertRecord({
//            {
//                std::make_unique<qb::Int32RecordValue>(0),
//                std::make_unique<qb::StrRecordValue>("data1"),
//                std::make_unique<qb::Int64RecordValue>(60),
//                std::make_unique<qb::StrRecordValue>("data2")
//            }
//        });
//        assert(ok);
//
//        {
//            auto res = c.match("column0", "0", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column1", "data1", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column2", "60", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column3", "data2", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column2", "not a number so what ?", ok);
//            assert(!ok);
//            res.debug_PrintCollection();
//        }
//
//         ok = c.insertRecord({
//            {
//                std::make_unique<qb::Int32RecordValue>(1),
//                std::make_unique<qb::StrRecordValue>("data1"),
//                std::make_unique<qb::Int64RecordValue>(60),
//                std::make_unique<qb::StrRecordValue>("data2")
//            }
//        });
//        assert(ok);
//
//        {
//            auto res = c.match("column0", "1", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column1", "data1", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column2", "60", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column3", "data2", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column2", "not a number so what ?", ok);
//            assert(!ok);
//            res.debug_PrintCollection();
//        }
//
//        c.remove(1);
//
//        {
//            auto res = c.match("column0", "1", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column1", "data1", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column2", "60", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column3", "data2", ok);
//            assert(ok);
//            res.debug_PrintCollection();
//        }
//        {
//            auto res = c.match("column2", "not a number so what ?", ok);
//            assert(!ok);
//            res.debug_PrintCollection();
//        }
//    }
//}

template <size_t TCount>
void runPerfTestFindMatchingIn() {
    size_t useTheResultToAvoidCompilerOptimization1 = 0;

    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int32_t i = 0; i < TCount; i++) {
            auto res = QBFindMatchingRecords(testQBImplementation, "column1", rndStrings[i % TEST_RND_ELEMENTS]);
            useTheResultToAvoidCompilerOptimization1 += res.size();
        }
        for (int32_t i = 0; i < TCount; i++) {
            auto res = QBFindMatchingRecords(testQBImplementation, "column2", std::to_string(rndLongs[i % TEST_RND_ELEMENTS]));
            useTheResultToAvoidCompilerOptimization1 += res.size();
        }
        for (int32_t i = 0; i < TCount; i++) {
            auto res = QBFindMatchingRecords(testQBImplementation, "column3", rndStrings[i % TEST_RND_ELEMENTS]);
            useTheResultToAvoidCompilerOptimization1 += res.size();
        }

        auto end = std::chrono::high_resolution_clock::now();

        std::cout << "QBFindMatchingRecords: " << TCount << " iterations took: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    }

   size_t useTheResultToAvoidCompilerOptimization2 = 0;

    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int32_t i = 0; i < TCount; i++) {
            auto res = base_impl::QBFindMatchingRecords(testBaseImplementation, "column1", rndStrings[i]);
            useTheResultToAvoidCompilerOptimization2 += res.size();
        }
        for (int32_t i = 0; i < TCount; i++) {
            auto res = base_impl::QBFindMatchingRecords(testBaseImplementation, "column2", std::to_string(rndLongs[i]));
            useTheResultToAvoidCompilerOptimization2 += res.size();
        }
        for (int32_t i = 0; i < TCount; i++) {
            auto res = base_impl::QBFindMatchingRecords(testBaseImplementation, "column3", rndStrings[i]);
            useTheResultToAvoidCompilerOptimization2 += res.size();
        }

        auto end = std::chrono::high_resolution_clock::now();

        std::cout << "base_impl::QBFindMatchingRecords: " << TCount << " iterations took: "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    }
}

void runAllTestCases() {
    beforeTests();

    runPerfTestFindMatchingIn<10>();
    std::cout << std::endl;
    runPerfTestFindMatchingIn<100>();
    std::cout << std::endl;
}

int main(int argc, _TCHAR* argv[]) {
    runAllTestCases();
    return 0;
}

