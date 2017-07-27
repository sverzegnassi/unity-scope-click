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

#include <tests/mock_pay.h>

#include "clickstore/store-query.h"

#include <string>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "click/qtbridge.h"
#include "click/index.h"
#include "clickstore/store-query.h"
#include "click/application.h"
#include "click/departments-db.h"
#include "test_helpers.h"
#include "click/package.h"

#include <tests/fake_json.h>
#include <tests/mock_network_access_manager.h>

#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/testing/MockSearchReply.h>

using namespace ::testing;
using namespace click;
using namespace click::test::helpers;

namespace
{

class MockQueryBase : public click::Query {
public:
    MockQueryBase(const unity::scopes::CannedQuery& query, click::Index& index,
                  click::DepartmentLookup& depts,
                  std::shared_ptr<click::DepartmentsDb> depts_db,
                  click::HighlightList& highlights,
                  scopes::SearchMetadata const& metadata,
                  pay::Package& in_package) :
        click::Query(query, index, depts, depts_db, highlights, metadata, in_package)
    {
    }

    void run_under_qt(const std::function<void()> &task) {
        // when testing, do not actually run under qt
        task();
    }
};

class MockQuery : public MockQueryBase {
public:
    MockQuery(const unity::scopes::CannedQuery& query, click::Index& index,
              click::DepartmentLookup& depts,
              std::shared_ptr<click::DepartmentsDb> depts_db,
              click::HighlightList& highlights,
              scopes::SearchMetadata const& metadata,
              pay::Package& in_package) :
        MockQueryBase(query, index, depts, depts_db, highlights, metadata, in_package)
    {
    }
    void wrap_add_available_apps(const scopes::SearchReplyProxy &searchReply,
                                 const PackageSet &installedPackages,
                                 const std::string& categoryTemplate)
    {
        add_available_apps(searchReply, installedPackages, categoryTemplate);
    }
    MOCK_METHOD2(push_result, bool(scopes::SearchReplyProxy const&, scopes::CategorisedResult const&));
    MOCK_METHOD0(clickInterfaceInstance, click::Interface&());
    MOCK_METHOD1(finished, void(scopes::SearchReplyProxy const&));
    MOCK_METHOD5(register_category, scopes::Category::SCPtr(const scopes::SearchReplyProxy &searchReply,
                                                            const std::string &id,
                                                            const std::string &title,
                                                            const std::string &icon,
                                                            const scopes::CategoryRenderer &renderer_template));
    using click::Query::get_installed_packages; // allow tests to access protected method
    using click::Query::push_package;
};

class MockQueryRun : public MockQueryBase {
public:
    MockQueryRun(const unity::scopes::CannedQuery& query, click::Index& index,
                 click::DepartmentLookup& depts,
                 std::shared_ptr<click::DepartmentsDb> depts_db,
                 click::HighlightList& highlights,
                 scopes::SearchMetadata const& metadata,
                 pay::Package& in_package) :
        MockQueryBase(query, index, depts, depts_db, highlights, metadata, in_package)
    {
    }
    MOCK_METHOD3(add_available_apps,
                 void(scopes::SearchReplyProxy const&searchReply,
                      const PackageSet &locallyInstalledApps,
                      const std::string& categoryTemplate));
    MOCK_METHOD3(push_local_results, void(scopes::SearchReplyProxy const &replyProxy,
                                          std::vector<click::Application> const &apps,
                                          std::string& categoryTemplate));
    MOCK_METHOD0(get_installed_packages, PackageSet());
};
} // namespace

TEST(QueryTest, testAddAvailableAppsCallsClickIndex)
{
    MockIndex mock_index;
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    scopes::SearchMetadata metadata("en_EN", "phone");
    MockPayPackage pay_pkg;
    PackageSet no_installed_packages;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _)).Times(1);

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));
    EXPECT_CALL(q, finished(_)).Times(1);
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
}

MATCHER_P(CategoryHasNumberOfResults, number, "") { return arg.find(std::to_string(number)) != std::string::npos; }

TEST(QueryTest, testAddAvailableAppsPushesResults)
{
    click::Packages packages {
        {"name", "title", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    scopes::SearchMetadata metadata("en_EN", "phone");
    MockPayPackage pay_pkg;
    PackageSet no_installed_packages;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);

    ON_CALL(q, register_category(_, _, _, _, _)).WillByDefault(Return(ptrCat));
    EXPECT_CALL(q, register_category(_, "appstore", CategoryHasNumberOfResults(1), _, _));
    EXPECT_CALL(q, register_category(_, "recommends", _, _, _));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});

    auto expected_title = packages.front().title;
    EXPECT_CALL(q, push_result(_, Property(&scopes::CategorisedResult::title, expected_title)));
    EXPECT_CALL(q, finished(_)).Times(1);
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
}

TEST(QueryTest, testAddAvailableAppsCallsFinished)
{
    click::Packages packages {
        {"name", "title", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    scopes::SearchMetadata metadata("en_EN", "phone");
    MockPayPackage pay_pkg;
    PackageSet no_installed_packages;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    EXPECT_CALL(q, push_result(_, _));
    EXPECT_CALL(q, finished(_)).Times(1);
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
}

TEST(QueryTest, testQueryRunCallsAddAvailableApps)
{
    click::Packages packages {
        {"name", "title", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    scopes::SearchMetadata metadata("en_EN", "phone");
    MockPayPackage pay_pkg;
    PackageSet no_installed_packages;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQueryRun q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    auto reply = scopes::SearchReplyProxy();
    EXPECT_CALL(q, get_installed_packages()).WillOnce(Return(no_installed_packages));
    EXPECT_CALL(q, add_available_apps(reply, no_installed_packages, _));

    // No need to test purchases in this testcase
    ASSERT_EQ(setenv(Configuration::PURCHASES_ENVVAR, "0", 1), 0);

    q.run(reply);
}

MATCHER_P(HasPackageName, n, "") { return arg[click::Query::ResultKeys::NAME].get_string() == n; }
MATCHER_P(IsInstalled, b, "") { return arg[click::Query::ResultKeys::INSTALLED].get_bool() == b; }

TEST(QueryTest, testDuplicatesNotFilteredAnymore)
{
    click::Packages packages {
        {"org.example.app1", "app title1", 0.0, "icon", "uri"},
        {"org.example.app2", "app title2", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet one_installed_package {
        {"org.example.app2", "0.2"}
    };
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    auto expected_name1 = packages.front().name;
    EXPECT_CALL(q, push_result(_, HasPackageName(expected_name1)));
    auto expected_name2 = packages.back().name;
    EXPECT_CALL(q, push_result(_, HasPackageName(expected_name2)));
    EXPECT_CALL(q, finished(_)).Times(1);
    q.wrap_add_available_apps(reply, one_installed_package, FAKE_CATEGORY_TEMPLATE);
}

TEST(QueryTest, testInstalledPackagesFlaggedAsSuch)
{
    click::Packages packages {
        {"org.example.app1", "app title1", 0.0, "icon", "uri"},
        {"org.example.app2", "app title2", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet one_installed_package {
        {"org.example.app2", "0.2"}
    };
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    EXPECT_CALL(q, push_result(_, IsInstalled(true)));
    EXPECT_CALL(q, push_result(_, IsInstalled(false)));
    EXPECT_CALL(q, finished(_)).Times(1);
    q.wrap_add_available_apps(reply, one_installed_package, FAKE_CATEGORY_TEMPLATE);
}

TEST(QueryTest, testDepartmentsDbIsUpdated)
{
    auto dept1 = std::make_shared<click::Department>("1", "Department one", "http://one.com", true);
    dept1->set_subdepartments({
            std::make_shared<click::Department>("1-1", "Department two", "http://two.com", false),
            std::make_shared<click::Department>("1-2", "Department three", "http://three.com", false)
            });
    DepartmentList init_departments({dept1});
    auto depts_db = std::make_shared<MockDepartmentsDb>(":memory:", true);

    EXPECT_CALL(*depts_db, store_department_name(_, _, _)).Times(3);
    EXPECT_CALL(*depts_db, store_department_mapping("1", ""));
    EXPECT_CALL(*depts_db, store_department_mapping("1-1", "1"));
    EXPECT_CALL(*depts_db, store_department_mapping("1-2", "1"));

    MockIndex mock_index(click::Packages(), click::DepartmentList(), init_departments);
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet one_installed_package {
        {"org.example.app2", "0.2"}
    };
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, depts_db, highlights, metadata, pay_pkg);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    EXPECT_CALL(q, finished(_)).Times(1);
    q.wrap_add_available_apps(reply, one_installed_package, FAKE_CATEGORY_TEMPLATE);
}

TEST(QueryTest, testSearchInDepartment)
{
    auto dept1 = std::make_shared<click::Department>("1", "Department one", "http://one.com", true);
    DepartmentList init_departments({dept1});
    auto depts_db = std::make_shared<MockDepartmentsDb>(":memory:", true);

    MockIndex mock_index(click::Packages(), click::DepartmentList(), init_departments);
    scopes::SearchMetadata metadata("en_EN", "phone");

    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "1");
    MockQuery q(query, mock_index, dept_lookup, depts_db, highlights, metadata, pay_pkg);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, "1", _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    std::list<std::string> expected_departments({"", "1"});
    scopes::testing::MockSearchReply mock_reply;
    EXPECT_CALL(mock_reply, register_departments(MatchesDepartments(expected_departments)));
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    EXPECT_CALL(q, finished(_)).Times(1);
    q.wrap_add_available_apps(reply, PackageSet(), FAKE_CATEGORY_TEMPLATE);
}

class FakeInterface : public click::Interface
{
public:
    MOCK_METHOD1(get_installed_packages, void(std::function<void(PackageSet, click::InterfaceError)> callback));
};

TEST(QueryTest, testGetInstalledPackages)
{
    click::Packages uninstalled_packages {
        {"name", "title", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(uninstalled_packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    PackageSet installed_packages{{"package_1", "0.1"}};

    FakeInterface fake_interface;
    EXPECT_CALL(q, clickInterfaceInstance()).WillOnce(ReturnRef(fake_interface));
    EXPECT_CALL(fake_interface, get_installed_packages(_)).WillOnce(Invoke(
        [&](std::function<void(PackageSet, click::InterfaceError)> callback){
            callback(installed_packages, click::InterfaceError::NoError);
    }));

    ASSERT_EQ(q.get_installed_packages(), installed_packages);
}

typedef std::pair<bool, bool> _PurchasedValues;
MATCHER_P(PurchasedProperties, b, "") { return arg[click::Query::ResultKeys::PURCHASED].get_bool() == b.first && arg[click::Query::ResultKeys::INSTALLED].get_bool() == b.second; }

TEST(QueryTest, testQueryRunPurchased)
{
    ASSERT_EQ(setenv(Configuration::PURCHASES_ENVVAR, "1", 1), 0);
    click::Packages packages {
        {"name", "title", 0.99, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    scopes::SearchMetadata metadata("en_EN", "phone");
    MockPayPackage pay_pkg;
    PackageSet no_installed_packages;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    q.purchased_apps.insert({"name"});
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);

    ON_CALL(q, register_category(_, _, _, _, _)).WillByDefault(Return(ptrCat));
    EXPECT_CALL(q, register_category(_, "appstore", _, _, _));
    EXPECT_CALL(q, register_category(_, "recommends", _, _, _));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});

    EXPECT_CALL(q, push_result(_, PurchasedProperties(_PurchasedValues{true, false}))).Times(1);
    EXPECT_CALL(q, finished(_));

    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
    ASSERT_EQ(unsetenv(Configuration::PURCHASES_ENVVAR), 0);
}

TEST(QueryTest, testQueryRunPurchasedAndInstalled)
{
    ASSERT_EQ(setenv(Configuration::PURCHASES_ENVVAR, "1", 1), 0);
    click::Packages packages {
        {"name", "title", 0.99, "icon", "uri"}
    };
    PackageSet one_installed_package {
        {"name", "0.2"}
    };
    MockIndex mock_index(packages);
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    scopes::SearchMetadata metadata("en_EN", "phone");
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    q.purchased_apps.insert({"name"});
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);

    ON_CALL(q, register_category(_, _, _, _, _)).WillByDefault(Return(ptrCat));
    EXPECT_CALL(q, register_category(_, "appstore", _, _, _));
    EXPECT_CALL(q, register_category(_, "recommends", _, _, _));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});

    EXPECT_CALL(q, push_result(_, PurchasedProperties(_PurchasedValues{true, true}))).Times(1);
    EXPECT_CALL(q, finished(_));

    q.wrap_add_available_apps(reply, one_installed_package, FAKE_CATEGORY_TEMPLATE);
    ASSERT_EQ(unsetenv(Configuration::PURCHASES_ENVVAR), 0);
}

TEST(QueryTest, testPushPackageSkipsPricedApps)
{
    ASSERT_EQ(setenv(Configuration::PURCHASES_ENVVAR, "0", 1), 0);

    click::Packages packages {
        {"org.example.app1", "app title1", 1.99, "icon", "uri"},
        {"org.example.app2", "app title2", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet no_installed_packages;
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    auto expected_name2 = packages.back().name;
    EXPECT_CALL(q, push_result(_, HasPackageName(expected_name2)));
    EXPECT_CALL(q, finished(_)).Times(1);
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);

    ASSERT_EQ(unsetenv(Configuration::PURCHASES_ENVVAR), 0);
}

TEST(QueryTest, testPushPackagePushesPricedApps)
{
    ASSERT_EQ(setenv(Configuration::PURCHASES_ENVVAR, "1", 1), 0);

    click::Packages packages {
        {"org.example.app1", "app title1", 1.99, "icon", "uri"},
        {"org.example.app2", "app title2", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet no_installed_packages;
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    auto expected_name1 = packages.front().name;
    EXPECT_CALL(q, push_result(_, HasPackageName(expected_name1)));
    auto expected_name2 = packages.back().name;
    EXPECT_CALL(q, push_result(_, HasPackageName(expected_name2)));
    EXPECT_CALL(q, finished(_)).Times(1);
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);

    ASSERT_EQ(unsetenv(Configuration::PURCHASES_ENVVAR), 0);
}

MATCHER_P(HasAttributes, b, "") { return (!arg["attributes"].is_null()) == b; }

TEST(QueryTest, testPushPackagePushesAttributes)
{
    click::Packages packages {
        {"org.example.app1", "app title1", 0.0, "icon", "uri"},
    };
    MockIndex mock_index(packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet no_installed_packages;
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    EXPECT_CALL(q, push_result(_, HasAttributes(true)));
    EXPECT_CALL(q, finished(_)).Times(1);
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
}

MATCHER_P(HasPrice, price, "") { return arg["price"].get_double() == price; }

TEST(QueryTest, testPushPackagePushesPriceUSD)
{
    ASSERT_EQ(unsetenv(Configuration::CURRENCY_ENVVAR), 0);
    ASSERT_EQ(setenv(Configuration::PURCHASES_ENVVAR, "1", 1), 0);

    Json::Value root;
    Json::Reader().parse(FAKE_JSON_SEARCH_RESULT_ONE, root);
    auto const embedded = root[Package::JsonKeys::embedded];
    auto const ci_package = embedded[Package::JsonKeys::ci_package];

    Packages packages = package_list_from_json_node(ci_package);

    MockIndex mock_index(packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet no_installed_packages;
    DepartmentLookup dept_lookup;
    HighlightList highlights;
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", "", "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    EXPECT_CALL(q, push_result(reply, HasPrice(1.99)));
    q.push_package(reply, ptrCat, no_installed_packages, packages[0]);

    ASSERT_EQ(unsetenv(Configuration::PURCHASES_ENVVAR), 0);
}

TEST(QueryTest, testPushPackagePushesPriceEUR)
{
    ASSERT_EQ(setenv(Configuration::CURRENCY_ENVVAR, "EUR", 1), 0);
    ASSERT_EQ(setenv(Configuration::PURCHASES_ENVVAR, "1", 1), 0);

    Json::Value root;
    Json::Reader().parse(FAKE_JSON_SEARCH_RESULT_ONE, root);
    auto const embedded = root[Package::JsonKeys::embedded];
    auto const ci_package = embedded[Package::JsonKeys::ci_package];

    Packages packages = package_list_from_json_node(ci_package);

    MockIndex mock_index(packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet no_installed_packages;
    DepartmentLookup dept_lookup;
    HighlightList highlights;
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", "", "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    EXPECT_CALL(q, push_result(reply, HasPrice(1.69)));
    q.push_package(reply, ptrCat, no_installed_packages, packages[0]);

    ASSERT_EQ(unsetenv(Configuration::CURRENCY_ENVVAR), 0);
    ASSERT_EQ(unsetenv(Configuration::PURCHASES_ENVVAR), 0);
}

MATCHER_P(HasVersion, v, "") { return arg[click::Query::ResultKeys::VERSION].get_string() == v; }

TEST(QueryTest, testPushPackagePushesVersion)
{
    auto const fake_version = "0.83b";
    click::Packages packages {
        {"org.example.app1", "app title1", 0.0, "icon", "uri", fake_version, "scope"},
    };
    MockIndex mock_index(packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet no_installed_packages;
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    MockPayPackage pay_pkg;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, nullptr, highlights, metadata, pay_pkg);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    EXPECT_CALL(q, push_result(_, HasVersion(fake_version)));
    EXPECT_CALL(q, finished(_)).Times(1);
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
}

