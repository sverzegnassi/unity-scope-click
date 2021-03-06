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

#ifndef CLICK_PACKAGE_H
#define CLICK_PACKAGE_H

#include <list>
#include <string>
#include <unordered_set>
#include <vector>
#include <functional>

#include <json/json.h>

#include <click/utils.h>

namespace json = Json;

namespace click {

struct Package
{
    struct JsonKeys
    {
        JsonKeys() = delete;

        constexpr static const char* embedded {"_embedded"};
        constexpr static const char* links{"_links"};
        constexpr static const char* self{"self"};
        constexpr static const char* href{"href"};
        constexpr static const char* ci_package {"clickindex:package"};
        constexpr static const char* ci_recommends {"clickindex:recommendation"};
        constexpr static const char* name{"name"};
        constexpr static const char* title{"title"};
        constexpr static const char* price{"price"};
        constexpr static const char* icon_url{"icon_url"};
        constexpr static const char* content{"content"};
        constexpr static const char* publisher{"publisher"};
        constexpr static const char* rating{"ratings_average"};
        constexpr static const char* version{"version"};

        // NOTE: The "price" field is deprecated in favor of "prices"
        constexpr static const char* prices{"prices"};
    };

    Package() = default;
    Package(const std::string& name, const std::string& title, double price, const std::string& icon_url, const std::string& url) :
        name(name),
        title(title),
        price(price),
        icon_url(icon_url),
        url(url)
    {
    }
    Package(const std::string& name, const std::string& title, double price, const std::string& icon_url, const std::string& url,
            const std::string& version, const std::string& content) :
        name(name),
        title(title),
        price(price),
        icon_url(icon_url),
        url(url),
        version(version),
        content(content)
    {
    }
    Package(const std::string& name, const std::string& version) :
        name(name),
        version(version)
    {
    }
    virtual ~Package() = default;

    std::string name; // formerly app_id
    std::string title;
    double price = 0.0f;
    std::string icon_url;
    std::string url;
    std::string version;
    std::string publisher;
    double rating = 0.0f;
    void matches (std::string query, std::function<bool> callback);
    std::string content;
    std::map<std::string, double> prices;

    struct hash_name {
    public :
        size_t operator()(const Package &package ) const
        {
            return std::hash<std::string>()(package.name);
        }
    };
};

typedef std::vector<Package> Packages;
typedef std::unordered_set<Package, Package::hash_name> PackageSet;

Package package_from_json_node(const Json::Value& item);
Packages package_list_from_json(const std::string& json);
Packages package_list_from_json_node(const Json::Value& root);

struct PackageDetails
{
    struct JsonKeys
    {
        JsonKeys() = delete;

        constexpr static const char* name{"name"};
        constexpr static const char* title{"title"};
        constexpr static const char* icon_url{"icon_url"};
        constexpr static const char* description{"description"};
        constexpr static const char* download_url{"download_url"};
        constexpr static const char* download_sha512{"download_sha512"};
        constexpr static const char* rating{"rating"};
        constexpr static const char* keywords{"keywords"};

        constexpr static const char* terms_of_service{"terms_of_service"};
        constexpr static const char* license{"license"};
        constexpr static const char* publisher{"publisher"};
        constexpr static const char* developer_name{"developer_name"};
        constexpr static const char* company_name{"company_name"};
        constexpr static const char* website{"website"};
        constexpr static const char* support_url{"support_url"};

        constexpr static const char* department{"department"};
        constexpr static const char* main_screenshot_url{"screenshot_url"};
        constexpr static const char* more_screenshot_urls{"screenshot_urls"};
        constexpr static const char* binary_filesize{"binary_filesize"};
        constexpr static const char* version{"version"};
        constexpr static const char* date_published{"date_published"};
        constexpr static const char* last_updated{"last_updated"};
        constexpr static const char* changelog{"changelog"};

        constexpr static const char* framework{"framework"};
    };

    static PackageDetails from_json(const std::string &json);

    Package package;

    std::string description;
    std::string download_url;
    std::string download_sha512;
    double rating;
    std::string keywords;

    std::string terms_of_service;
    std::string license;
    std::string publisher;
    std::string developer_name;
    std::string company_name;
    std::string website;
    std::string support_url;

    std::string main_screenshot_url;
    std::list<std::string> more_screenshots_urls;
    json::Value::UInt64 binary_filesize;
    std::string version;
    click::Date date_published;
    click::Date last_updated;
    std::string changelog;

    std::string framework;
    std::string department;
};

std::ostream& operator<<(std::ostream& out, const PackageDetails& details);

bool operator==(const Package& lhs, const Package& rhs);
bool operator==(const PackageDetails& lhs, const PackageDetails& rhs);

} // namespace click

#endif // CLICK_PACKAGE_H
