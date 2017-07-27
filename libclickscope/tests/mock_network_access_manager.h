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

#ifndef MOCK_NETWORK_ACCESS_MANAGER_H
#define MOCK_NETWORK_ACCESS_MANAGER_H

#include <click/network_access_manager.h>

#include <gmock/gmock.h>

#include <QObject>
#include <QNetworkReply>

class MockNetworkReply : public click::network::Reply
{
public slots:
    void sendFinished();
    void sendError();

public:
    MockNetworkReply()
    {
        // Set a default value for QByteArray-returning mocked methods.
        ::testing::DefaultValue<QByteArray>::Set(QByteArray(""));
        ON_CALL(*this, attribute(::testing::_)).WillByDefault(::testing::Return(0));
    }

    MOCK_METHOD0(abort, void());
    MOCK_METHOD0(readAll, QByteArray());
    MOCK_METHOD1(attribute, QVariant(QNetworkRequest::Attribute));
    MOCK_METHOD1(hasRawHeader, bool(const QByteArray&));
    MOCK_METHOD1(rawHeader, QString(const QByteArray &headerName));

    // We have to typedef the result here as the preprocessor is dumb
    // and would interpret the "," in the template spec as part of the
    // macro declaration and not part of the signature.
    typedef QList<QPair<QByteArray, QByteArray>> ResultType;
    MOCK_METHOD0(rawHeaderPairs, ResultType());
    MOCK_METHOD0(errorString, QString());
};

struct MockNetworkAccessManager : public click::network::AccessManager
{
    MockNetworkAccessManager()
    {
    }

    MOCK_METHOD1(get, QSharedPointer<click::network::Reply>(QNetworkRequest&));
    MOCK_METHOD1(head, QSharedPointer<click::network::Reply>(QNetworkRequest&));
    MOCK_METHOD2(post, QSharedPointer<click::network::Reply>(QNetworkRequest&, QByteArray&));
    MOCK_METHOD3(sendCustomRequest, QSharedPointer<click::network::Reply>(QNetworkRequest&, QByteArray&, QIODevice*));

    static QList<QByteArray> scripted_responses;
    static QList<QNetworkRequest> performed_get_requests;
    static QList<QNetworkRequest> performed_head_requests;
    static bool shouldSignalNetworkError;
};

#endif // MOCK_NETWORK_ACCESS_MANAGER_H
