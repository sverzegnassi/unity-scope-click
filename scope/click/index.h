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

#ifndef CLICKINDEX_H
#define CLICKINDEX_H


#include <string>
#include <list>
#include <iosfwd>
#include <functional>

#include "webclient.h"


namespace click {

const std::string SEARCH_BASE_URL = "https://search.apps.ubuntu.com/";
const std::string SEARCH_PATH = "api/v1/search";
const std::string SUPPORTED_FRAMEWORKS = "framework:ubuntu-sdk-13.10";
const std::string QUERY_ARGNAME = "q";
const std::string ARCHITECTURE = "architecture:";
const std::string DETAILS_PATH = "api/v1/package/";

struct Package
{
    Package() = default;
    Package(std::string name, std::string title, std::string price, std::string icon_url, std::string url) :
        name(name),
        title(title),
        price(price),
        icon_url(icon_url),
        url(url)
    {
    }
    Package(std::string name, std::string title, std::string price, std::string icon_url, std::string url, std::string version) :
        name(name),
        title(title),
        price(price),
        icon_url(icon_url),
        url(url),
        version(version)
    {
    }
    virtual ~Package() = default;

    std::string name; // formerly app_id
    std::string title;
    std::string price;
    std::string icon_url;
    std::string url;
    std::string version;
    void matches (std::string query, std::function<bool> callback);
};

class PackageManager
{
public:
    void uninstall (const Package& package, std::function<void(int, std::string)>);
    virtual void execute_uninstall_command (const std::string& command, std::function<void(int, std::string)>);
};

typedef std::list<Package> PackageList;

PackageList package_list_from_json(const std::string& json);

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
        constexpr static const char* rating{"rating"};
        constexpr static const char* keywords{"keywords"};
        constexpr static const char* terms_of_service{"terms_of_service"};
        constexpr static const char* license{"license"};
        constexpr static const char* publisher{"publisher"};
        constexpr static const char* main_screenshot_url{"screenshot_url"};
        constexpr static const char* more_screenshot_urls{"screenshot_urls"};
        constexpr static const char* binary_filesize{"binary_filesize"};
        constexpr static const char* version{"version"};
        constexpr static const char* framework{"framework"};
    };

    std::string name; // formerly app_id
    std::string title;
    std::string icon_url;
    std::string description;
    std::string download_url;
    std::string rating;
    std::string keywords;
    std::string terms_of_service;
    std::string license;
    std::string publisher;
    std::string main_screenshot_url;
    std::list<std::string> more_screenshots_urls;
    std::string binary_filesize;
    std::string version;
    std::string framework;
    void loadJson(const std::string &json);
};

std::ostream& operator<<(std::ostream& out, const PackageDetails& details);

class Cancellable
{
protected:
    QSharedPointer<click::web::Response> response;
public:
    Cancellable() {}
    Cancellable(QSharedPointer<click::web::Response> response) : response(response) {}
    virtual void cancel() { if (response) {response->abort();} }
    virtual ~Cancellable() {}
};


class Index
{
protected:
    QSharedPointer<web::Client> client;
public:
    enum class Error {NoError, CredentialsError, NetworkError};
    Index(const QSharedPointer<click::web::Client>& client);
    Cancellable search (const std::string& query, std::function<void(PackageList)> callback);
    Cancellable get_details(const std::string& package_name, std::function<void(PackageDetails, Error)> callback);
    ~Index();
};

bool operator==(const Package& lhs, const Package& rhs);
bool operator==(const PackageDetails& lhs, const PackageDetails& rhs);

} // namespace click

#endif // CLICKINDEX_H

