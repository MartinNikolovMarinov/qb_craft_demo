#include "stdafx.h"

#include <assert.h>
#include "stdafx.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <ratio>
#include <string>
#include <vector>

namespace {

static constexpr int32_t TEST_CASES = 100000;
static constexpr int32_t TEST_RND_ELEMENTS = 1000;
static std::array<std::string, TEST_RND_ELEMENTS> rndStrings;
static std::array<int64_t, TEST_RND_ELEMENTS> rndLongs;

static base_impl::QBRecordCollection testBaseImplementation;
static qb::QBRecordCollection testQBImplementation({ "column0", "column1", "column2", "column3" });

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

    // Populate test data
    {
        auto& c1 = testBaseImplementation;
        auto& c2 = testQBImplementation;
        bool ok = false;

        c1.reserve(TEST_CASES);

        ok = c2.createIndex("column1", qb::RecordValueType::String);
        assert(ok);
        ok = c2.createIndex("column2", qb::RecordValueType::Int64);
        assert(ok);
        ok = c2.createIndex("column3", qb::RecordValueType::String);
        assert(ok);

        c2.reserve(TEST_CASES);

        // Make sure all generated random elemnts are present in the collection:
        for (int32_t i = 0; i < TEST_RND_ELEMENTS; i++) {

            {
                base_impl::QBRecord r;
                r.column0 = uniqueIdx;
                r.column1 = rndStrings[i];
                r.column2 = rndLongs[i];
                r.column3 = rndStrings[TEST_RND_ELEMENTS - i - 1];
                c1.push_back(r);
            }

            {
                qb::Record<4> r;
                r.columns[0] = std::make_unique<qb::Int32RecordValue>(uniqueIdx);
                r.columns[1] = std::make_unique<qb::StrRecordValue>(rndStrings[i]);
                r.columns[2] = std::make_unique<qb::Int64RecordValue>(rndLongs[i]);
                r.columns[3] = std::make_unique<qb::StrRecordValue>(rndStrings[TEST_RND_ELEMENTS - i - 1]);
                c2.insertRecord(std::move(r));
            }

            uniqueIdx++;
        }

        // Fill the rest of the collection with random data.
        for (int32_t i = 0; i < (TEST_CASES - TEST_RND_ELEMENTS); i++) {
            int32_t rndIdx1 = core::genRndInt32(0, TEST_RND_ELEMENTS - 1);
            int32_t rndIdx2 = core::genRndInt32(0, TEST_RND_ELEMENTS - 1);
            int32_t rndIdx3 = core::genRndInt32(0, TEST_RND_ELEMENTS - 1);

            {
                base_impl::QBRecord r;
                r.column0 = uniqueIdx++;
                r.column1 = rndStrings[rndIdx1];
                r.column2 = rndLongs[rndIdx2];
                r.column3 = rndStrings[rndIdx3];
                c1.push_back(r);
            }

            {
                qb::Record<4> r;
                r.columns[0] = std::make_unique<qb::Int32RecordValue>(uniqueIdx);
                r.columns[1] = std::make_unique<qb::StrRecordValue>(rndStrings[rndIdx1]);
                r.columns[2] = std::make_unique<qb::Int64RecordValue>(rndLongs[rndIdx2]);
                r.columns[3] = std::make_unique<qb::StrRecordValue>(rndStrings[rndIdx3]);
                c2.insertRecord(std::move(r));
            }

            uniqueIdx++;
        }
    }
}

void runFunctionalTests() {
    std::cout << "Running functional tests" << std::endl;

    using QBRecordCollection = qb::QBRecordCollection;

    auto assertColumns = [](const auto& r, const auto& c1, const auto& c2, const auto& c3, const auto& c4) {
        auto* idRecord = dynamic_cast<qb::Int32RecordValue*>(r.columns[0].get());
        assert(idRecord);
        assert(idRecord->value == c1);

        auto* strRecord = dynamic_cast<qb::StrRecordValue*>(r.columns[1].get());
        assert(strRecord);
        assert(strRecord->value == c2);

        auto* intRecord = dynamic_cast<qb::Int64RecordValue*>(r.columns[2].get());
        assert(intRecord);
        assert(intRecord->value == c3);

        auto* strRecord2 = dynamic_cast<qb::StrRecordValue*>(r.columns[3].get());
        assert(strRecord2);
        assert(strRecord2->value == c4);
    };

    struct TestRecord {
        int32_t id;
        std::string column1;
        int64_t column2;
        std::string column3;
    };

    auto assertResult = [&](const QBRecordCollection& res, size_t expectedSize, const std::vector<TestRecord>& expected) {
        assert(res.size() == expectedSize);
        size_t i = 0;
        for (const auto& [id, r] : res) {
            auto& ex = expected[i];
            assertColumns(r, ex.id, ex.column1, ex.column2, ex.column3);
            i++;
        }
    };

    {
        bool ok = false;
        QBRecordCollection c({ "column0", "column1", "column2", "column3" });
        assert(c.createIndex("column1", qb::RecordValueType::String));
        assert(c.createIndex("column2", qb::RecordValueType::Int64));
        assert(c.createIndex("column3", qb::RecordValueType::String));

        {
            auto res = c.match("column0", "0", ok);
            assert(ok);
            assert(res.size() == 0);
        }

        ok = c.insertRecord({
            {
                std::make_unique<qb::Int32RecordValue>(0),
                std::make_unique<qb::StrRecordValue>("data1"),
                std::make_unique<qb::Int64RecordValue>(60),
                std::make_unique<qb::StrRecordValue>("data2")
            }
        });
        assert(ok);

        {
            auto res = c.match("column0", "0", ok);
            assert(ok);
            assert(res.size() == 1);

            assertResult(res, 1, { { 0, "data1", 60, "data2" } });
        }
        {
            auto res = c.match("column1", "data1", ok);
            assert(ok);
            assert(res.size() == 1);

            assertResult(res, 1, { { 0, "data1", 60, "data2" } });
        }
        {
            auto res = c.match("column2", "60", ok);
            assert(ok);
            assert(res.size() == 1);

            assertResult(res, 1, { { 0, "data1", 60, "data2" } });
        }
        {
            auto res = c.match("column3", "data2", ok);
            assert(ok);
            assert(res.size() == 1);

            assertResult(res, 1, { { 0, "data1", 60, "data2" } });
        }

        {
            auto res = c.match("column3", "data1", ok);
            assert(ok); // Should this be false? I don't think so.
            assert(res.size() == 0);
        }
        {
            auto res = c.match("column2", "Not a number! Should not crash, or throw.", ok);
            assert(!ok);
            assert(res.size() == 0);
        }

        ok = c.insertRecord({
            {
                std::make_unique<qb::Int32RecordValue>(1),
                std::make_unique<qb::StrRecordValue>("data1"),
                std::make_unique<qb::Int64RecordValue>(60),
                std::make_unique<qb::StrRecordValue>("data2")
            }
        });
        assert(ok);

        {
            auto res = c.match("column0", "1", ok);
            assert(ok);
            assert(res.size() == 1);

            assertResult(res, 1, { { 1, "data1", 60, "data2" } });
        }
        {
            auto res = c.match("column1", "data1", ok);
            assert(ok);
            assert(res.size() == 2);

            assertResult(res, 2, { { 0, "data1", 60, "data2" }, { 1, "data1", 60, "data2" } });
        }
        {
            auto res = c.match("column2", "60", ok);
            assert(ok);
            assert(res.size() == 2);

            assertResult(res, 2, { { 0, "data1", 60, "data2" }, { 1, "data1", 60, "data2" } });
        }
        {
            auto res = c.match("column3", "data2", ok);
            assert(ok);
            assert(res.size() == 2);

            assertResult(res, 2, { { 0, "data1", 60, "data2" }, { 1, "data1", 60, "data2" } });
        }

        c.remove(0);

        {
            auto res = c.match("column0", "0", ok);
            assert(ok);
            assert(res.size() == 0);
        }
        {
            auto res = c.match("column1", "data1", ok);
            assert(ok);
            assert(res.size() == 1);

            assertResult(res, 1, { { 1, "data1", 60, "data2" } });
        }
        {
            auto res = c.match("column2", "60", ok);
            assert(ok);
            assert(res.size() == 1);

            assertResult(res, 1, { { 1, "data1", 60, "data2" } });
        }
        {
            auto res = c.match("column3", "data2", ok);
            assert(ok);
            assert(res.size() == 1);

            assertResult(res, 1, { { 1, "data1", 60, "data2" } });
        }
    }
}

template <size_t TCount>
void runPerfTestFindMatchingIn() {
    std::cout << "Running perf test with " << TCount << " iterations" << std::endl;

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
            auto res = base_impl::QBFindMatchingRecords(testBaseImplementation, "column1", rndStrings[i % TEST_RND_ELEMENTS]);
            useTheResultToAvoidCompilerOptimization2 += res.size();
        }
        for (int32_t i = 0; i < TCount; i++) {
            auto res = base_impl::QBFindMatchingRecords(testBaseImplementation, "column2", std::to_string(rndLongs[i % TEST_RND_ELEMENTS]));
            useTheResultToAvoidCompilerOptimization2 += res.size();
        }
        for (int32_t i = 0; i < TCount; i++) {
            auto res = base_impl::QBFindMatchingRecords(testBaseImplementation, "column3", rndStrings[i % TEST_RND_ELEMENTS]);
            useTheResultToAvoidCompilerOptimization2 += res.size();
        }

        auto end = std::chrono::high_resolution_clock::now();

        std::cout << "base_impl::QBFindMatchingRecords: " << TCount << " iterations took: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    }

    assert(useTheResultToAvoidCompilerOptimization1 == useTheResultToAvoidCompilerOptimization2);
}

}

void runAllTestCases() {
    beforeTests();

    runFunctionalTests();
    std::cout << std::endl;

    int i = 0;
    while (i++ < 20) {
        runPerfTestFindMatchingIn<10>();
        std::cout << std::endl;
        runPerfTestFindMatchingIn<100>();
        std::cout << std::endl;
        runPerfTestFindMatchingIn<1000>();
        std::cout << std::endl;
        runPerfTestFindMatchingIn<10000>();
        std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "All tests passed!" << std::endl;
    std::cout << std::endl;
}
