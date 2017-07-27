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

#include <click/pay.h>

#include <click/webclient.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>


namespace
{

    constexpr static const char* FAKE_PURCHASES_LIST_JSON{R"foo(
            [
                {
                    "state": "Complete",
                    "package_name": "com.example.fake",
                    "refundable_until": "1970-01-01T00:01:23Z",
                    "open_id": "https:\/\/login.ubuntu.com/+openid/fakeuser"
                }
            ]
        )foo"};


    constexpr static const char* FAKE_PURCHASES_LIST_JSON_NULL_TIMESTAMP{R"foo(
            [
                {
                    "state": "Complete",
                    "package_name": "com.example.fake",
                    "refundable_until": null,
                    "open_id": "https:\/\/login.ubuntu.com/+openid/fakeuser"
                }
            ]
        )foo"};



    class MockPayPackage : public pay::Package {
    public:
        MockPayPackage()
            : Package(QSharedPointer<click::web::Client>())
        {
        }

        MockPayPackage(const QSharedPointer<click::web::Client>& client)
            : Package(client)
        {
        }

        void pay_package_refund(const std::string& pkg_name)
        {
            callbacks[pkg_name + pay::APPENDAGE_REFUND](pkg_name, success);
            do_pay_package_refund(pkg_name);
        }

        void pay_package_verify(const std::string& pkg_name)
        {
            callbacks[pkg_name + pay::APPENDAGE_VERIFY](pkg_name, success);
            do_pay_package_verify(pkg_name);
        }

        bool is_refundable(const std::string& pkg_name)
        {
            do_is_refundable(pkg_name);
            return refundable;
        }

        MOCK_METHOD0(setup_pay_service, void());
        MOCK_METHOD1(do_pay_package_refund, void(const std::string&));
        MOCK_METHOD1(do_pay_package_verify, void(const std::string&));
        MOCK_METHOD1(do_is_refundable, void(const std::string&));

        bool refundable = false;
        bool success = false;
        pay::PurchaseSet purchases;
};

} // namespace
