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

#include <unity/scopes/testing/MockPreviewReply.h>
#include <unity/scopes/testing/Result.h>

#include <gtest/gtest.h>
#include <click/preview.h>
#include <fake_json.h>

using namespace ::testing;
using namespace unity::scopes;

class FakeResult : public Result
{
public:
    FakeResult(VariantMap& vm) : Result(vm)
    {
    }

};

class FakePreview : public click::PreviewStrategy
{
public:
    FakePreview(Result& result) : click::PreviewStrategy::PreviewStrategy(result)
    {

    }

    void run(const PreviewReplyProxy &/*reply*/)
    {

    }
    using click::PreviewStrategy::screenshotsWidgets;
    using click::PreviewStrategy::descriptionWidgets;
    using click::PreviewStrategy::build_other_metadata;
    using click::PreviewStrategy::build_updates_table;
    using click::PreviewStrategy::build_whats_new;
};

class PreviewsBaseTest : public Test
{
public:
    VariantMap vm;
    VariantMap internal;
    VariantMap attrs;

    PreviewsBaseTest()
    {
        attrs["uri"] = "fake://result";
        vm["internal"] = internal;
        vm["attrs"] = attrs;
    }
};

class PreviewStrategyTest : public PreviewsBaseTest
{

};

TEST_F(PreviewStrategyTest, testScreenshotsWidget)
{
    FakeResult result{vm};
    FakePreview preview{result};
    click::PackageDetails details;
    details.main_screenshot_url = "sshot1";
    details.more_screenshots_urls = {"sshot2", "sshot3"};

    auto widgets = preview.screenshotsWidgets(details);
    ASSERT_EQ(widgets.size(), 1);
    auto first_widget = widgets.front();
    auto attributes = first_widget.attribute_values();
    auto sources = attributes["sources"].get_array();
    ASSERT_EQ(sources[0].get_string(), "sshot1");
    ASSERT_EQ(sources[1].get_string(), "sshot2");
    ASSERT_EQ(sources[2].get_string(), "sshot3");
}

class PreviewStrategyDescriptionTest : public PreviewStrategyTest
{
public:
    FakeResult result{vm};
    FakePreview preview{result};
    click::PackageDetails details;
    unity::scopes::PreviewWidgetList widgets;

    PreviewStrategyDescriptionTest()
    {
    }
    void assertWidgetAttribute(int n, std::string attribute_name, std::string expected_value)
    {
        ASSERT_GT(widgets.size(), n);
        auto widget = std::next(widgets.begin(), n);
        auto attributes = widget->attribute_values();
        ASSERT_EQ(expected_value, attributes[attribute_name].get_string());
    }
    void assertWidgetNestedVariantArray(int n, int x, std::string expected_title, std::string expected_value)
    {
        auto widget = std::next(widgets.begin(), n);
        auto attributes = widget->attribute_values();
        auto found_title = attributes["values"].get_array()[x].get_array()[0].get_string();
        ASSERT_EQ(found_title, expected_title);
        auto found_value = attributes["values"].get_array()[x].get_array()[1].get_string();
        ASSERT_EQ(found_value, expected_value);
    }
};

TEST_F(PreviewStrategyDescriptionTest, testDescriptionWidgetsFull)
{
    details = click::PackageDetails::from_json(FAKE_JSON_PACKAGE_DETAILS);
    widgets = preview.descriptionWidgets(details);

    assertWidgetAttribute(0, "title", "Info");
    assertWidgetAttribute(0, "text", details.description);

    assertWidgetNestedVariantArray(1, 0, "Publisher/Creator", "Fake Publisher");
    assertWidgetNestedVariantArray(1, 1, "Seller", "Fake Company");
    assertWidgetNestedVariantArray(1, 2, "Website", "http://example.com");
    assertWidgetNestedVariantArray(1, 3, "Contact", "http://example.com/support");
    assertWidgetNestedVariantArray(1, 4, "License", "Proprietary");

    assertWidgetAttribute(2, "title", "Updates");
    assertWidgetNestedVariantArray(2, 0, "Version number", "0.2");
    assertWidgetNestedVariantArray(2, 1, "Last updated", "Jul 3, 2014");
    assertWidgetNestedVariantArray(2, 2, "First released", "Nov 3, 2013");
    assertWidgetNestedVariantArray(2, 3, "Size", "173.4 KiB");

    assertWidgetAttribute(3, "title", "What's new");
    assertWidgetAttribute(3, "text", preview.build_whats_new(details));

}

TEST_F(PreviewStrategyDescriptionTest, testDescriptionWidgetsMinimal)
{
    details = click::PackageDetails::from_json(FAKE_JSON_PACKAGE_DETAILS_DEB);
    widgets = preview.descriptionWidgets(details);

    ASSERT_EQ(1, widgets.size());

    assertWidgetAttribute(0, "title", "Info");
    assertWidgetAttribute(0, "text", details.description);
}

TEST_F(PreviewStrategyDescriptionTest, testDescriptionWidgetsNone)
{
    widgets = preview.descriptionWidgets(details);
    ASSERT_EQ(0, widgets.size());
}

class FakePreviewStrategy :  public click::PreviewStrategy
{
public:
    FakePreviewStrategy(const unity::scopes::Result& result) : PreviewStrategy(result) {}
    using click::PreviewStrategy::pushPackagePreviewWidgets;
    std::vector<std::string> call_order;
    virtual void run(unity::scopes::PreviewReplyProxy const& /*reply*/)
    {
    }

    virtual unity::scopes::PreviewWidgetList screenshotsWidgets (const click::PackageDetails &/*details*/) {
        call_order.push_back("screenshots");
        return {};
    }
    virtual unity::scopes::PreviewWidgetList headerWidgets (const click::PackageDetails &/*details*/) {
        call_order.push_back("header");
        return {};
    }
};

TEST_F(PreviewStrategyTest, testScreenshotsPushedAfterHeader)
{
    FakeResult result{vm};
    unity::scopes::testing::MockPreviewReply reply;
    std::shared_ptr<unity::scopes::testing::MockPreviewReply> replyptr{&reply, [](unity::scopes::testing::MockPreviewReply*){}};
    click::PackageDetails details;

    FakePreviewStrategy preview{result};
    preview.pushPackagePreviewWidgets(replyptr, details, {});
    std::vector<std::string> expected_order {"header", "screenshots"};
    ASSERT_EQ(expected_order, preview.call_order);
}

class StrategyChooserTest : public Test {
protected:
    unity::scopes::testing::Result result;
    unity::scopes::ActionMetadata metadata;
    unity::scopes::VariantMap metadict;
    QSharedPointer<click::web::Client> client;
    QSharedPointer<click::network::AccessManager> nam;
    std::shared_ptr<click::DepartmentsDb> depts;
    const std::string FAKE_SHA512 = "FAKE_SHA512";

public:
    StrategyChooserTest() : metadata("en_EN", "phone") {
    }
};

class MockablePreview : public click::Preview {
public:
    MockablePreview(const unity::scopes::Result& result, const unity::scopes::ActionMetadata& metadata) :
        click::Preview(result, metadata)
    {

    }

    MOCK_METHOD6(build_installing, click::PreviewStrategy*(const std::string&, const std::string&,
                const unity::scopes::Result&, const QSharedPointer<click::web::Client>&,
                const QSharedPointer<click::network::AccessManager>&, std::shared_ptr<click::DepartmentsDb>));
};

TEST_F(StrategyChooserTest, testSha512IsUsed) {
    metadict["action_id"] = click::Preview::Actions::INSTALL_CLICK;
    metadict["download_url"] = "fake_download_url";
    metadict["download_sha512"] = FAKE_SHA512;
    metadata.set_scope_data(unity::scopes::Variant(metadict));
    MockablePreview preview(result, metadata);
    EXPECT_CALL(preview, build_installing(_, FAKE_SHA512, _, _, _, _));
    preview.choose_strategy(client, nam, depts);
}


class UninstalledPreviewTest : public PreviewsBaseTest {
public:
    FakeResult result{vm};
    click::PackageDetails details;
    unity::scopes::PreviewWidgetList widgets;
    QSharedPointer<click::web::Client> client;
    QSharedPointer<click::network::AccessManager> nam;
    std::shared_ptr<click::DepartmentsDb> depts;
    unity::scopes::testing::MockPreviewReply reply;
    std::shared_ptr<unity::scopes::testing::MockPreviewReply> replyptr{&reply, [](unity::scopes::testing::MockPreviewReply*){}};
};

class FakeDownloader : public click::Downloader {
    std::string object_path;
    std::function<void (std::string)> callback;
public:
    FakeDownloader(const std::string& object_path, const QSharedPointer<click::network::AccessManager>& networkAccessManager)
        : click::Downloader(networkAccessManager), object_path(object_path)
    {

    }
    void get_download_progress(std::string /*package_name*/, const std::function<void (std::string)> &callback)
    {
        this->callback = callback;
    }

    void activate_callback()
    {
        callback(object_path);
    }
};

class FakeUninstalledPreview : public click::UninstalledPreview {
    std::string object_path;
public:
    std::unique_ptr<FakeDownloader> fake_downloader;
    FakeUninstalledPreview(const std::string& object_path,
                           const unity::scopes::Result& result,
                           const QSharedPointer<click::web::Client>& client,
                           const std::shared_ptr<click::DepartmentsDb>& depts,
                           const QSharedPointer<click::network::AccessManager>& nam)
        : click::UninstalledPreview(result, client, depts, nam), object_path(object_path),
          fake_downloader(new FakeDownloader(object_path, nam))
    {

    }

    virtual click::Downloader* get_downloader(const QSharedPointer<click::network::AccessManager> &/*nam*/)
    {
        return fake_downloader.get();
    }

    void populateDetails(std::function<void (const click::PackageDetails &)> details_callback,
                         std::function<void (const click::ReviewList &, click::Reviews::Error)> reviews_callback) {
        click::PackageDetails details;
        details_callback(details);
        reviews_callback({}, click::Reviews::Error::NoError);
    }
    MOCK_METHOD1(uninstalledActionButtonWidgets, scopes::PreviewWidgetList (const click::PackageDetails &details));
    MOCK_METHOD1(progressBarWidget, scopes::PreviewWidgetList(const std::string& object_path));
};

TEST_F(UninstalledPreviewTest, testDownloadInProgress) {
    std::string fake_object_path = "/fake/object/path";

    result["name"] = "fake_app_name";
    scopes::PreviewWidgetList response;
    FakeUninstalledPreview preview(fake_object_path, result, client, depts, nam);
    EXPECT_CALL(preview, progressBarWidget(_))
            .Times(1)
            .WillOnce(Return(response));
    preview.run(replyptr);
    preview.fake_downloader->activate_callback();
}

TEST_F(UninstalledPreviewTest, testNoDownloadProgress) {
    std::string fake_object_path = "";

    result["name"] = "fake_app_name";
    scopes::PreviewWidgetList response;
    FakeUninstalledPreview preview(fake_object_path, result, client, depts, nam);
    EXPECT_CALL(preview, uninstalledActionButtonWidgets(_))
            .Times(1)
            .WillOnce(Return(response));
    preview.run(replyptr);
    preview.fake_downloader->activate_callback();
}
