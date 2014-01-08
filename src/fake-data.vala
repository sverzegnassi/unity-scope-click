/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

const string FAKE_JSON_SEARCH_RESULT = """
[
  {
    "name": "org.example.awesomelauncher",
    "title": "Awesome Launcher",
    "description": "This is an awesome launcher.",
    "price": 1.99,
    "icon_url": "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png",
    "resource_url": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher"
  },
  {
    "name": "org.example.fantastiqueapp",
    "title": "Fantastic App",
    "description": "This is a fantasticc app.",
    "price": 0.0,
    "icon_url": "http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-find-more-apps-64x64.png",
    "resource_url": "http://search.apps.ubuntu.com/api/v1/package/org.example.fantasticapp"
  },
  {
    "name": "org.example.awesomewidget",
    "title": "Awesome Widget",
    "description": "This is an awesome widget.",
    "price": 1.99,
    "icon_url": "http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-photos-and-videos-64x64.png",
    "resource_url": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomewidget"
  }
]
""";

/*
const string FAKE_JSON_PACKAGE_DETAILS = """
{"requires_license_key": false, "video_urls": ["http://vimeo.com/61594280"], "website": "http://example.org/full_app/en/", "screenshot_url": "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_03.jpg", "icon_url": "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png", "binary_filesize": 123456, "download_url": "http://software-center.ubuntu.com/org.example.full_app/full_app-0.1.1.tar.gz", "version": "0.1.1", "company_name": "The Fantastic App Company", "screenshot_urls": ["http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_01.jpg", "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_02.jpg", "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_03.jpg"], "support_url": "http://example.org/full_app/support/en/", "title": "The Full Application", "price": 0.0, "terms_of_service": "http://example.org/full_app/terms/en/", "description": "This application does magic.", "framework": "ubuntu-sdk-13.10", "icon_urls": ["64|http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png"], "keywords": ["fantastic app", "fantastic application", "absolutely fantastic", "app", "searchterm"], "name": "org.example.full_app_real_icons", "license": "GPLv3", "date_published": "2013-01-01 00:00:00", "click_version": "0.1", "license_key_path": ""}
""";
*/

const string FAKE_JSON_PACKAGE_DETAILS = """
{"requires_license_key": false, "screenshot_url": "http://software-center.ubuntu.com/site_media/appmedia/org.example.full_app/screenshots/ss1.png", "video_urls": ["http://software-center.ubuntu.com/site_media/appmedia/org.example.full_app/videos/vid1.png", "http://software-center.ubuntu.com/site_media/appmedia/org.example.full_app/videos/vid2.png"], "terms_of_service": "http://example.org/full_app/terms/en/", "keywords": ["fantastic app", "fantastic application", "fantastic", "app"], "title": "The Full Application", "support_url": "http://example.org/full_app/support/en/", "icon_url": "http://software-center.ubuntu.com/site_media/appmedia/org.example.full_app/icons/icon64.png", "binary_filesize": 23456, "download_url": "http://software-center.ubuntu.com/org.example.full_app/full_app-0.1.1.tar.gz", "version": "0.1.1", "company_name": "The Fantastic App Company", "screenshot_urls": ["http://software-center.ubuntu.com/site_media/appmedia/org.example.full_app/screenshots/ss1.png", "http://software-center.ubuntu.com/site_media/appmedia/org.example.full_app/screenshots/ss2.png"], "website": "http://example.org/full_app/en/", "description": "This application contains values for all the fields in the Solr schema.", "price": 0.0, "date_published": "2013-01-01T00:00:00", "icon_urls": {"64": "http://software-center.ubuntu.com/site_media/appmedia/org.example.full_app/icons/icon64.png"}, "name": "org.example.full_app", "license": "GPLv3", "framework": "ubuntu-sdk-13.10", "click_version": "0.1", "license_key_path": ""}
""";

const string SKINNY_PACKAGE_DETAILS = """
{"website": "", "name": "ar.com.beuno.wheather-touch", "license": "Proprietary", "title": "Weather", "support_url": "http://beuno.com.ar", "icon_url": "http://developer.staging.ubuntu.com/site_media/appmedia/2013/07/weather-icone-6797-64.png", "price": 0.0, "license_key_path": "", "binary_filesize": 177582, "download_url": "https://public.apps.staging.ubuntu.com/download/ar.com.beuno/wheather-touch/ar.com.beuno.wheather-touch-0.2", "click_version": "0.1", "framework": "None", "version": "0.2", "company_name": "", "terms_of_service": "", "screenshot_url": "", "icon_urls": {"64": "http://developer.staging.ubuntu.com/site_media/appmedia/2013/07/weather-icone-6797-64.png"}, "requires_license_key": false, "date_published": "2013-07-16T21:50:34.874000", "description": "Weather\nA weather application."}
""";

const string FAKE_APP_MANIFEST = """
[
    {
        "framework": "ubuntu-sdk-13.10",
        "hooks": {
            "evilapp": {
                "apparmor": "apparmor/evilapp.json",
                "desktop": "evilapp.desktop"
            }
        },
        "maintainer": "Jamie Strandboge <jamie@ubuntu.com>",
        "name": "com.ubuntu.developer.pedrocan.evilapp",
        "title": "Evil App",
        "version": "0.4"
    },

    {
        "framework": "ubuntu-sdk-13.10",
        "hooks": {
            "ubuntu-weather": {
                "apparmor": "apparmor/ubuntu-weather.json",
                "desktop": "ubuntu-weather-app.desktop"
            }
        },
        "maintainer": "Ubuntu App Cats <ubuntu-touch-coreapps@lists.launchpad.net>",
        "name": "com.ubuntu.ubuntu-weather",
        "title": "Weather application",
        "version": "0.2"
    }
]
""";

const string FAKE_RNR_REVIEW_RESULTS = """
[
    {
         "origin": "canonical",
         "rating": 1,
         "hide": false,
         "app_name": "",
         "language": "en",
         "reviewer_username": "gbtest",
         "usefulness_total": 31,
         "usefulness_favorable": 30,
         "review_text": "Cause system hangs a lot",
         "date_deleted": null,
         "summary": "poor linux version",
         "version": "4.2.0.11-0ubuntu0.12.04.2",
         "id": 7678621,
         "date_created": "2013-08-11 13:23:13",
         "reviewer_displayname": "Brian Test",
         "package_name": "skyter",
         "distroseries": "precise"
    },
    {
         "origin": "canonical",
         "rating": 4,
         "hide": false,
         "app_name": "",
         "language": "en",
         "reviewer_username": "user3844",
         "usefulness_total": 6,
         "usefulness_favorable": 6,
         "review_text": "Great app versions",
         "date_deleted": null,
         "summary": "works stable, but has a bad interface",
         "version": "4.2.0.11-0ubuntu0.12.04.2",
         "id": 78054,
         "date_created": "2013-09-09 18:59:20",
         "reviewer_displayname": "Oron Pista",
         "package_name": "skyter",
         "distroseries": "raring"
    }
]
""";
