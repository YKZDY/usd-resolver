// SPDX-FileCopyrightText: Copyright (c) 2020-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.
#pragma once

#include "TestLog.h"
#include "utils/StringUtils.h"

#include <OmniClient.h>
#include <memory>
#include <mutex>
#include <string.h>
#include <vector>

// Legacy testing.
struct Tests
{
private:
    bool _thisFailed = false;
    int _numFailed = 0;
    int _numPassed = 0;
    int _inTest = 0;

public:
    int main()
    {
        omniClientSetLogCallback(
            [](const char* threadName, const char* component, OmniClientLogLevel level, const char* message)
            { testlog::printf("%c: %s: %s: %s\n", omniClientGetLogLevelChar(level), threadName, component, message); });
#ifdef _NDEBUG
        omniClientSetLogLevel(eOmniClientLogLevel_Verbose);
#else
        omniClientSetLogLevel(eOmniClientLogLevel_Debug);
#endif

        if (!omniClientInitialize(kOmniClientVersion))
        {
            return EXIT_FAILURE;
        }

        run();

        printf("%d run, %d failed\n", _numFailed + _numPassed, _numFailed);

        omniClientShutdown();

        return _numFailed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    virtual void run() = 0;

    class TestCase
    {
    public:
        TestCase(Tests* tests, const char* testName)
        {
            m_tests = tests;
            m_tests->startTest(testName);
        }
        TestCase(Tests* tests, const char* testName, const char* url)
        {
            m_tests = tests;
            m_tests->startTest(concat(testName, '(', url, ')').c_str());
        }
        ~TestCase()
        {
            m_tests->finishTest();
        }

    private:
        Tests* m_tests;
    };

    void startTest(const char* testName)
    {
        if (_inTest == 0)
        {
            testlog::start(testName);
            _thisFailed = false;
        }
        _inTest++;
    }
    void waitForTest(OmniClientRequestId requestId)
    {
        if (requestId == 0)
        {
            fail("Invalid request id");
        }
        else
        {
            omniClientWait(requestId);
        }
    }
    void finishTest()
    {
        _inTest--;
        if (_inTest <= 0)
        {
            _inTest = 0;
            testlog::finish(!_thisFailed);
        }
    }
    void fail(std::string const& reason)
    {
        testlog::print("****************************************\n");
        testlog::printf("Fail! %s\n", reason.c_str());
        testlog::print("****************************************\n");
        _numFailed++;
        _thisFailed = true;
    }
    void pass()
    {
        _numPassed++;
    }

    void verifyResult(OmniClientResult expectedResult, OmniClientResult result)
    {
        if (expectedResult != result)
        {
            fail(concat(
                "Got ", omniClientGetResultString(result), "; expected ", omniClientGetResultString(expectedResult)));
        }
        else
        {
            pass();
        }
    }
    void verifyContent(const char* expected, const char* actual, size_t actualSize)
    {
        const bool actualNull = !actual;
        const bool expectedNull = !expected;
        if (actualNull && expectedNull)
        {
            pass();
        }
        else if (actualNull && !expectedNull)
        {
            fail(concat("Got nothing; expected \"", expected, "\""));
        }
        else if (!actualNull && expectedNull)
        {
            fail(concat("Got \"", actual, "\"; expected nothing"));
        }
        else if (strncmp(expected, actual, actualSize) != 0)
        {
            fail(concat("Got \"", actual, "\"; expected \"", expected, "\""));
        }
    }
    bool verifyValue(uint64_t expected, uint64_t actual)
    {
        if (expected != actual)
        {
            fail(concat("Got ", actual, "; expected ", expected));
            return false;
        }
        else
        {
            pass();
            return true;
        }
    }
    bool verifyValue(const char* expected, const char* actual)
    {
        const bool actualNull = !actual;
        const bool expectedNull = !expected;
        if (actualNull && expectedNull)
        {
            pass();
            return true;
        }
        else if (actualNull && !expectedNull)
        {
            fail(concat("Got nothing; expected \"", expected, "\""));
        }
        else if (!actualNull && expectedNull)
        {
            fail(concat("Got \"", actual, "\"; expected nothing"));
        }
        else if (strcmp(expected, actual) != 0)
        {
            fail(concat("Got \"", actual, "\"; expected \"", expected, "\""));
        }
        return false;
    }

    bool verifyNotValue(uint64_t notValue, uint64_t actual)
    {
        if (notValue == actual)
        {
            fail(concat("Got unexpected ", actual));
            return false;
        }
        else
        {
            pass();
            return true;
        }
    }

    void testDelete(const char* url, OmniClientResult expectedResult)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
        };
        TestCase tc(this, "Delete", url);
        waitForTest(omniClientDelete(url, new Context{ this, expectedResult },
                                     [](void* userData, OmniClientResult result)
                                     {
                                         auto context = (Context*)userData;
                                         auto tests = context->tests;
                                         tests->verifyResult(context->expectedResult, result);
                                         delete context;
                                     }));
    }

    void testDelete(const char* url)
    {
        TestCase tc(this, "Delete", url);
        waitForTest(omniClientDelete(url, nullptr, nullptr));
    }

    void testCreateFolder(const char* url, OmniClientResult expectedResult)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
        };
        TestCase tc(this, "CreateFolder", url);
        waitForTest(omniClientCreateFolder(url, new Context{ this, expectedResult },
                                           [](void* userData, OmniClientResult result)
                                           {
                                               auto context = (Context*)userData;
                                               auto tests = context->tests;
                                               tests->verifyResult(context->expectedResult, result);
                                               delete context;
                                           }));
    }

    void testStat(const char* url, OmniClientResult expectedResult, uint64_t expectedFlags)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
            uint64_t expectedFlags;
        };
        TestCase tc(this, "Stat", url);
        waitForTest(omniClientStat(url, new Context{ this, expectedResult, expectedFlags },
                                   [](void* userData, OmniClientResult result, OmniClientListEntry const* entry)
                                   {
                                       auto context = (Context*)userData;
                                       auto tests = context->tests;
                                       tests->verifyResult(context->expectedResult, result);
                                       if (entry)
                                       {
                                           tests->verifyValue(context->expectedFlags, entry->flags);
                                       }
                                       delete context;
                                   }));
    }

    void testResolve(const char* relativePath,
                     char const* const* searchPaths,
                     uint32_t numSearchPaths,
                     OmniClientResult expectedResult,
                     const char* expectedUrl)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
            const char* expectedUrl;
        };
        TestCase tc(this, "Resolve", relativePath);
        waitForTest(omniClientResolve(
            relativePath, searchPaths, numSearchPaths, new Context{ this, expectedResult, expectedUrl },
            [](void* userData, OmniClientResult result, OmniClientListEntry const* entry, const char* url)
            {
                auto context = (Context*)userData;
                auto tests = context->tests;
                tests->verifyResult(context->expectedResult, result);
                if (entry)
                {
                    tests->verifyValue(context->expectedUrl, url);
                }
                delete context;
            }));
    }

    uint32_t testList(const char* url, OmniClientResult expectedResult)
    {
        return testList(url, expectedResult, UINT32_MAX);
    }

    uint32_t testList(const char* url, OmniClientResult expectedResult, uint32_t expectedNumEntries)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
            uint32_t expectedNumEntries;
            uint32_t actualNumEntries;
        };
        Context context{ this, expectedResult, expectedNumEntries, 0 };
        TestCase tc(this, "list", url);
        waitForTest(omniClientList(
            url, &context,
            [](void* userData, OmniClientResult result, uint32_t numEntries, OmniClientListEntry const* entries)
            {
                auto context = (Context*)userData;
                auto tests = context->tests;
                tests->verifyResult(context->expectedResult, result);
                if (context->expectedNumEntries != UINT32_MAX)
                {
                    tests->verifyValue(context->expectedNumEntries, numEntries);
                }
                context->actualNumEntries = numEntries;
            }));

        return context.actualNumEntries;
    }

    void testWrite(const char* url, OmniClientResult expectedResult, const char* contentStr)
    {
        testWrite(url, expectedResult, (void*)contentStr, strlen(contentStr));
    }

    void testWrite(const char* url, OmniClientResult expectedResult, void* contentBuffer, size_t contentSize)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
        };
        // Note: this only works if we're wrapping a static constant string!
        OmniClientContent content;
        content.buffer = contentBuffer;
        content.size = contentSize;
        content.free = nullptr;
        TestCase tc(this, "Write", url);
        waitForTest(omniClientWriteFile(url, &content, new Context{ this, expectedResult },
                                        [](void* userData, OmniClientResult result)
                                        {
                                            auto context = (Context*)userData;
                                            auto tests = context->tests;
                                            tests->verifyResult(context->expectedResult, result);
                                            delete context;
                                        }));
    }

    void testRead(const char* url, OmniClientResult expectedResult, const char* expectedContent)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
            const char* expectedContent;
        };
        TestCase tc(this, "Read", url);
        waitForTest(omniClientReadFile(
            url, new Context{ this, expectedResult, expectedContent },
            [](void* userData, OmniClientResult result, const char* version, OmniClientContent* content)
            {
                auto context = (Context*)userData;
                auto tests = context->tests;
                tests->verifyResult(context->expectedResult, result);
                tests->verifyContent(
                    context->expectedContent, content ? (char*)content->buffer : nullptr, content ? content->size : 0);
                delete context;
            }));
    }

    void testCopy(const char* srcUrl,
                  const char* dstUrl,
                  OmniClientResult expectedResult,
                  OmniClientCopyBehavior behavior = eOmniClientCopy_ErrorIfExists)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
        };
        TestCase tc(this, "Copy", concat(srcUrl, " -> ", dstUrl).c_str());
        waitForTest(omniClientCopy(
            srcUrl, dstUrl, new Context{ this, expectedResult },
            [](void* userData, OmniClientResult result)
            {
                auto context = (Context*)userData;
                auto tests = context->tests;
                tests->verifyResult(context->expectedResult, result);
                delete context;
            },
            behavior));
    }

    void testGetAcls(const char* url,
                     OmniClientResult expectedResult,
                     uint32_t expectedNumEntries,
                     OmniClientAclEntry* expectedEntries)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
            uint32_t expectedNumEntries;
            OmniClientAclEntry* expectedEntries;
        };
        TestCase tc(this, "getAcls", url);
        waitForTest(omniClientGetAcls(
            url, new Context{ this, expectedResult, expectedNumEntries, expectedEntries },
            [](void* userData, OmniClientResult result, uint32_t numEntries, OmniClientAclEntry* entries)
            {
                auto context = (Context*)userData;
                auto tests = context->tests;
                tests->verifyResult(context->expectedResult, result);
                if (tests->verifyValue(context->expectedNumEntries, numEntries))
                {
                    for (uint32_t i = 0; i < numEntries; i++)
                    {
                        tests->verifyValue(context->expectedEntries[i].name, entries[i].name);
                        tests->verifyValue(context->expectedEntries[i].access, entries[i].access);
                    }
                }
                delete context;
            }));
    }

    void testSetAcls(const char* url, uint32_t numEntries, OmniClientAclEntry* entries, OmniClientResult expectedResult)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
        };
        TestCase tc(this, "setAcls", url);
        waitForTest(omniClientSetAcls(url, numEntries, entries, new Context{ this, expectedResult },
                                      [](void* userData, OmniClientResult result)
                                      {
                                          auto context = (Context*)userData;
                                          auto tests = context->tests;
                                          tests->verifyResult(context->expectedResult, result);
                                          delete context;
                                      }));
    }

    void testLock(const char* url, OmniClientResult expectedResult)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
        } context{ this, expectedResult };
        TestCase tc(this, "lock", url);
        waitForTest(omniClientLock(url, &context,
                                   [](void* userData, OmniClientResult result)
                                   {
                                       auto context = (Context*)userData;
                                       auto tests = context->tests;
                                       tests->verifyResult(context->expectedResult, result);
                                   }));
    }

    void testUnlock(const char* url, OmniClientResult expectedResult)
    {
        struct Context
        {
            Tests* tests;
            OmniClientResult expectedResult;
        } context{ this, expectedResult };
        TestCase tc(this, "unlock", url);
        waitForTest(omniClientUnlock(url, &context,
                                     [](void* userData, OmniClientResult result)
                                     {
                                         auto context = (Context*)userData;
                                         auto tests = context->tests;
                                         tests->verifyResult(context->expectedResult, result);
                                     }));
    }

    struct JoinContext
    {
        Tests* tests;
        OmniClientResult expectedResult;
        bool finished = false;
    };
    struct JoinResult
    {
        OmniClientRequestId channelId;
        JoinContext* joinContext;

        JoinResult(OmniClientRequestId id, JoinContext* context) : channelId(id), joinContext(context)
        {
        }

        ~JoinResult()
        {
            omniClientStop(channelId);
            delete joinContext;
        }
    };

    std::shared_ptr<JoinResult> testJoinChannel(const char* url, OmniClientResult expectedResult)
    {
        TestCase tc(this, "JoinChannel", url);

        auto joinContext = new JoinContext{ this, expectedResult };
        auto channelId =
            omniClientJoinChannel(url, joinContext,
                                  [](void* userData, OmniClientResult result, OmniClientChannelEvent eventType,
                                     const char* from, struct OmniClientContent* content)
                                  {
                                      auto context = (JoinContext*)userData;
                                      if (!context->finished)
                                      {
                                          auto tests = context->tests;
                                          tests->verifyResult(context->expectedResult, result);
                                          context->finished = true;
                                      }
                                  });
        waitForTest(channelId);

        return std::make_shared<JoinResult>(channelId, joinContext);
    }

    void testSendMessage(const char* url, OmniClientResult expectedResult)
    {
        TestCase tc(this, "SendMessage", url);

        auto joinResult = testJoinChannel(url, expectedResult);

        struct SendContext
        {
            Tests* tests;
            OmniClientResult expectedResult;
        };
        OmniClientContent content = {};
        content.buffer = (void*)".";
        content.size = 1;
        content.free = nullptr;
        waitForTest(omniClientSendMessage(joinResult->channelId, &content, new SendContext{ this, expectedResult },
                                          [](void* userData, OmniClientResult result)
                                          {
                                              auto context = (SendContext*)userData;
                                              auto tests = context->tests;
                                              tests->verifyResult(context->expectedResult, result);
                                              delete context;
                                          }));
    }

    // This test verifies that:
    // 1. You can issue a read inside a list callback
    // 2. You can issue a stop from inside a read callback
    void testStopInReadInList(const char* baseUrl)
    {
        struct Context
        {
            Tests* tests;
            std::string baseUrl;
            std::vector<OmniClientRequestId> readRequestIds;
        };
        auto context = new Context{ this, baseUrl / std::string() };
        TestCase tc(this, "StopInReadInList", baseUrl);
        auto listRequestId = omniClientList(
            baseUrl, context,
            [](void* userData, OmniClientResult result, uint32_t numEntries, OmniClientListEntry const* entries)
            {
                auto context = (Context*)userData;
                auto tests = context->tests;
                omniClientPushBaseUrl(context->baseUrl.c_str());
                for (uint32_t i = 0; i < numEntries; i++)
                {
                    if (entries[i].flags & fOmniClientItem_ReadableFile)
                    {
                        struct ReadContext
                        {
                            Context* context;
                            uint32_t i;
                        };
                        context->readRequestIds.resize(numEntries);
                        context->readRequestIds[i] =
                            omniClientReadFile(entries[i].relativePath, new ReadContext{ context, i },
                                               [](void* userData, OmniClientResult result, const char* version,
                                                  struct OmniClientContent* content)
                                               {
                                                   auto readContext = (ReadContext*)userData;
                                                   auto context = readContext->context;
                                                   auto tests = context->tests;
                                                   tests->verifyResult(eOmniClientResult_Ok, result);
                                                   omniClientStop(context->readRequestIds[readContext->i]);
                                                   delete readContext;
                                               });
                        tests->verifyNotValue(context->readRequestIds[i], (OmniClientRequestId)0);
                    }
                }
                omniClientPopBaseUrl(context->baseUrl.c_str());
            });
        omniClientWait(listRequestId);
        for (auto&& readRequestId : context->readRequestIds)
        {
            omniClientWait(readRequestId);
        }
        waitForTest(listRequestId);
        delete context;
    }
};
