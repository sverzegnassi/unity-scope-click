/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QThread>
#include <QTimer>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <click/download-manager.h>

#include "mock_network_access_manager.h"
#include "mock_ubuntuone_credentials.h"


namespace
{
const QString TEST_URL("http://test.local/");

struct DownloadManagerTest : public ::testing::Test
{
    DownloadManagerTest() : app(argc, argv)
    {

        QSharedPointer<click::network::AccessManager> namPtr(&mockNam,
                                                             [](click::network::AccessManager*) {});

        QSharedPointer<click::CredentialsService> csPtr(&mockCredentialsService,
                                                        [](click::CredentialsService*) {});

        dm.reset(new click::DownloadManager(namPtr, csPtr));

        QObject::connect(
                    &testTimeout, &QTimer::timeout,
                    [this]() { app.quit(); FAIL() << "Operation timed out."; } );
    }

    void SetUp()
    {
        const int oneSecondInMsec = 1000;
        testTimeout.start(2 * oneSecondInMsec);
    }

    void Quit()
    {
        app.quit();
    }

    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app;
    QTimer testTimeout;
    QScopedPointer<click::DownloadManager> dm;
    MockNetworkAccessManager mockNam;
    MockCredentialsService mockCredentialsService;
};


struct DownloadManagerMockClient {
    MOCK_METHOD1(onFetchClickErrorEmitted, void(QString errorMessage));
};

} // anon namespace


TEST_F(DownloadManagerTest, testFetchClickTokenCredentialsNotFound)
{
    using namespace ::testing;

    // Mock the credentials service, to signal that creds are not found:
    EXPECT_CALL(mockCredentialsService, getCredentials())
        .Times(1).WillOnce(
            InvokeWithoutArgs(&mockCredentialsService, 
                              &MockCredentialsService::credentialsNotFound));

    // We should not hit the NAM without creds:
    EXPECT_CALL(mockNam, head(_)).Times(0);

    // Connect the mock client to downloadManager's error signal,
    // to check that it sends appropriate signals:

    DownloadManagerMockClient mockDownloadManagerClient;
    QObject::connect(dm.data(), &click::DownloadManager::clickTokenFetchError,
                     [&mockDownloadManagerClient](const QString& error)
                     {
                         mockDownloadManagerClient.onFetchClickErrorEmitted(error);
                     });

    // The error signal should be sent once, and we clean up the
    // test's QCoreApplication in its handler:
    EXPECT_CALL(mockDownloadManagerClient, onFetchClickErrorEmitted(_))
            .Times(1)
            .WillOnce(
                InvokeWithoutArgs(
                    this,
                    &DownloadManagerTest::Quit));

    // Now start the function we're testing, after a delay. This is
    // awkwardly verbose because QTimer::singleShot doesn't accept
    // arguments or lambdas.

    // We need to delay the call until after the app.exec() call so
    // that when we call app.quit() on success, there is a running app
    // to quit.
    QScopedPointer<QTimer> timer(new QTimer());
    timer->setSingleShot(true);
    QObject::connect(timer.data(), &QTimer::timeout, [&]() {
            dm->fetchClickToken(TEST_URL);
        } );
    timer->start(0);
    
    // now exec the app so events can proceed:
    app.exec();

}

// Test Cases:

// void TestDownloadManager::testFetchClickTokenCredsFoundButNetworkError()
// {
//     _tdm.setShouldSignalCredsFound(true);
//     FakeNam::shouldSignalNetworkError = true;
//     QSignalSpy spy(&_tdm, SIGNAL(clickTokenFetchError(QString)));
//     _tdm.fetchClickToken(TEST_URL);
//     QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, SCOPE_TEST_TIMEOUT_MSEC);
// }

// void TestDownloadManager::testFetchClickTokenSuccess()
// {
//     _tdm.setShouldSignalCredsFound(true);
//     FakeNam::shouldSignalNetworkError = false;
//     QSignalSpy spy(&_tdm, SIGNAL(clickTokenFetched(QString)));
//     _tdm.fetchClickToken(TEST_URL);
//     QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, SCOPE_TEST_TIMEOUT_MSEC);
// }
