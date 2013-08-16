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

private const string ACTION_INSTALL_CLICK = "install_click";
private const string ACTION_DOWNLOAD_COMPLETED = "download_completed";
private const string ACTION_OPEN_CLICK = "open_click";
private const string ACTION_PIN_TO_LAUNCHER = "pin_to_launcher";
private const string ACTION_UNINSTALL_CLICK = "uninstall_click";
private const string ACTION_CLOSE_PREVIEW = "close_preview";

public const string METADATA_APP_ID = "app_id";
public const string METADATA_TITLE = "title";
public const string METADATA_PRICE = "price";

errordomain ClickScopeError {
    INSTALL_ERROR
}

class ClickPreviewer: Unity.ResultPreviewer
{
    ClickScope scope;

    public ClickPreviewer (ClickScope scope)
    {
        this.scope = scope;
    }

    public override Unity.AbstractPreview? run () {
        return null;
    }

    public override void run_async (Unity.AbstractPreviewCallback async_callback) {
        var app_id = result.metadata.get(METADATA_APP_ID).get_string();
        scope.build_app_preview.begin(app_id, (obj, res) => {
            var preview = scope.build_app_preview.end(res);
            preview.add_action (new Unity.PreviewAction (ACTION_INSTALL_CLICK, ("Install"), null));
            async_callback (this, preview);
        });
    }
}

class ClickScope: Unity.AbstractScope
{
  const string HINT_SCREENSHOTS = "more-screenshots";
  const string HINT_KEYWORDS = "keywords";
  const string HINT_RATING = "rating";
  const string HINT_RATED = "rated";
  const string HINT_REVIEWS = "reviews";
  const string HINT_COMMENTS = "comments";

  const string LABEL_SCREENSHOTS = "Screenshots";
  const string LABEL_KEYWORDS = "Keywords";
  const string LABEL_RATING = "Rating";
  const string LABEL_RATED = "Rated";
  const string LABEL_REVIEWS = "Reviews";
  const string LABEL_COMMENTS = "Comments";

  public ClickScope ()
  {
  }

  public Variant fake_comments () {
    Variant comments[3] = {
      new Variant("(siss)", "gatox", 5, "Dec 28", "This is a great app!"),
      new Variant("(siss)", "mandel", 3, "Jan 29", "This is a fantastique app!"),
      new Variant("(siss)", "alecu", 1, "Jan 30", "Love the icons...")
    };
    return new Variant.array(new VariantType("(siss)"), comments);
  }

  Unity.Preview build_error_preview (string message) {
    var preview = new Unity.GenericPreview("Error", message, null);
    return preview;
  }

  internal async Unity.Preview build_app_preview(string app_id) {
    Unity.Preview result = null;
    var webservice = new ClickWebservice();
    webservice.get_details.begin(app_id, (obj, res) => {
        try {
            var details = webservice.get_details.end (res);
            var icon = new FileIcon(File.new_for_uri(details.icon_url));
            var screenshot = new FileIcon(File.new_for_uri(details.main_screenshot_url));
            var preview = new Unity.ApplicationPreview (details.title, "subtitle", details.description, icon, screenshot);
            preview.license = details.license;
            preview.add_info(new Unity.InfoHint.with_variant(HINT_SCREENSHOTS, LABEL_SCREENSHOTS, null, new Variant.strv(details.more_screenshot_urls)));
            preview.add_info(new Unity.InfoHint.with_variant(HINT_KEYWORDS, LABEL_KEYWORDS, null, new Variant.strv(details.keywords)));
            preview.add_info(new Unity.InfoHint.with_variant(HINT_RATING, LABEL_RATING, null, new Variant.int32(5)));
            preview.add_info(new Unity.InfoHint.with_variant(HINT_RATED, LABEL_RATED, null, new Variant.int32(3)));
            preview.add_info(new Unity.InfoHint.with_variant(HINT_REVIEWS, LABEL_REVIEWS, null, new Variant.int32(15)));
            // TODO: get the proper reviews from the rnr webservice:
            preview.add_info(new Unity.InfoHint.with_variant(HINT_COMMENTS, LABEL_COMMENTS, null, fake_comments ()));
            // TODO: get the proper reviews from the rnr webservice ^^^^^^^^^^^^^^^^^^^^^^^^^
            result = preview;
        } catch (WebserviceError e) {
            debug ("Error calling webservice: %s", e.message);
            // TODO: The actions may be wrong
            result = build_error_preview (e.message);
        }
        build_app_preview.callback ();
    });
    yield;
    return result;
  }

    Unity.ActivationResponse build_error_response (string message) {
        var error_preview = build_error_preview(message);
        error_preview.add_action (new Unity.PreviewAction (ACTION_CLOSE_PREVIEW, ("Close"), null));
        return new Unity.ActivationResponse.with_preview (error_preview);
    }

    public override Unity.ActivationResponse? activate (Unity.ScopeResult result, Unity.SearchMetadata metadata, string? action_id) {
        Unity.Preview preview = null;
        var app_id = result.metadata.get(METADATA_APP_ID).get_string();

        MainLoop mainloop = new MainLoop ();
        build_app_preview.begin(app_id, (obj, res) => {
            preview = build_app_preview.end(res);
            mainloop.quit ();
        });
        mainloop.run ();

        if (action_id == null) {
            preview.add_action (new Unity.PreviewAction (ACTION_INSTALL_CLICK, ("Install"), null));
            return new Unity.ActivationResponse.with_preview(preview);
        } else if (action_id == ACTION_INSTALL_CLICK) {
            preview.add_action (new Unity.PreviewAction (ACTION_DOWNLOAD_COMPLETED, ("*** download_completed"), null));
            preview.add_info(new Unity.InfoHint.with_variant("show_progressbar", "Progressbar", null, new Variant.boolean(true)));
            var response = new Unity.ActivationResponse.with_preview(preview);
            debug ("################## INSTALLATION started: %s, %s", action_id, app_id);
            mainloop = new MainLoop ();
            install_app.begin(app_id, (obj, res) => {
                // FIXME: forced to be a string
                try {
                    string object_path = install_app.end(res);
                    preview.add_info(new Unity.InfoHint.with_variant("progressbar_source", "Progress Source", null, object_path));
                } catch (ClickScopeError e) {
                    debug ("Error starting installation: %s", e.message);
                    response = build_error_response (e.message);
                }
                mainloop.quit ();
            });
            mainloop.run ();
            return response;
        } else if (action_id == ACTION_DOWNLOAD_COMPLETED) {
            preview.add_action (new Unity.PreviewAction (ACTION_OPEN_CLICK, ("Open"), null));
            preview.add_action (new Unity.PreviewAction (ACTION_PIN_TO_LAUNCHER, ("Pin to launcher"), null));
            preview.add_action (new Unity.PreviewAction (ACTION_UNINSTALL_CLICK, ("Uninstall"), null));
            debug ("######## RETURNING PREVIEW ########## ACTION started: %s", action_id);
            return new Unity.ActivationResponse.with_preview(preview);
        } else if (action_id == ACTION_OPEN_CLICK) {
            var click_if = new ClickInterface ();
            var response = new Unity.ActivationResponse(Unity.HandledType.HIDE_DASH);
            click_if.execute.begin(app_id, (obj, res) => {
                try {
                    click_if.execute.end(res);
                } catch (ClickError e) {
                    debug ("cannot execute application: %s", e.message);
                    response = build_error_response (e.message);
                }
            });
            return response;
        } else if (action_id == ACTION_CLOSE_PREVIEW) {
            return new Unity.ActivationResponse(Unity.HandledType.HIDE_DASH);
        } else {
            debug ("################## ACTION started: %s", action_id);
            return null;
        }
    }

    public async GLib.ObjectPath install_app (string app_id) throws ClickScopeError {
        try {
            debug ("getting details for %s", app_id);
            var click_ws = new ClickWebservice ();
            var app_details = yield click_ws.get_details (app_id);
            debug ("got details: %s", app_details.title);
            var u1creds = new UbuntuoneCredentials ();
            debug ("getting creds");
            var credentials = yield u1creds.get_credentials ();
            debug ("got creds: %s", credentials["token"]);
            var signed_download = new SignedDownload (credentials);

            var download_url = app_details.download_url;
            // TODO: this is only valid while click package downloads are unauthenticated
            download_url += "?noauth=1";

            debug ("starting download of %s from: %s", app_id, download_url);
            var download_object_path = yield signed_download.start_download (download_url, app_id);
            debug ("download started: %s", download_object_path);
            return download_object_path;
        } catch (Error e) {
            debug ("cannot install app %s: %s", app_id, e.message);
            throw new ClickScopeError.INSTALL_ERROR (e.message);
        }
    }

  public override Unity.ScopeSearchBase create_search_for_query (Unity.SearchContext ctx)
  {
    var search = new ClickSearch ();
    search.set_search_context (ctx);
    return search;
  }

  public override Unity.ResultPreviewer create_previewer (Unity.ScopeResult result, Unity.SearchMetadata metadata)
  {
    debug ("creating previewer *********************************88");
    var cpp = new ClickPreviewer(this);
    cpp.set_scope_result(result);
    cpp.set_search_metadata(metadata);
    return cpp;
  }

  public override Unity.CategorySet get_categories ()
  {
    var categories = new Unity.CategorySet ();
    var icon = new FileIcon(File.new_for_path("/usr/share/icons/unity-icon-theme/places/svg/group-treat-yourself.svg"));
    categories.add (new Unity.Category("more", "More suggestions", icon, Unity.CategoryRenderer.GRID));
    return categories;
  }

  public override Unity.FilterSet get_filters ()
  {
    return new Unity.FilterSet ();
  }

  public override Unity.Schema get_schema ()
  {
    return new Unity.Schema ();
  }

  public override string get_group_name ()
  {
    return "com.canonical.Unity.Scope.Applications.Click";
  }

  public override string get_unique_name ()
  {
    return "/com/canonical/unity/scope/applications/click";
  }
}

class ClickSearch: Unity.ScopeSearchBase
{
  private void add_result (App app)
  {
    // File icon_file = File.new_for_uri (app.icon_url);
    // var icon = new FileIcon (icon_file); //currently unused

    var result = Unity.ScopeResult ();
    result.icon_hint = app.icon_url;
    result.category = 0;
    result.result_type = Unity.ResultType.DEFAULT;
    result.mimetype = "inode/folder";
    result.title = app.title;
    result.comment = "no comment";
    result.dnd_uri = "test::uri";
    result.metadata = new HashTable<string, Variant> (str_hash, str_equal);
    debug (app.title);
    //result.uri = // need an url into an app webstore
    result.metadata.insert(METADATA_APP_ID, new GLib.Variant.string(app.app_id));
    result.metadata.insert(METADATA_TITLE, new GLib.Variant.string(app.title));
    result.metadata.insert(METADATA_PRICE, new GLib.Variant.string(app.price));

    var download_progress = get_download_progress(app.app_id);
    debug ("download progress source for app %s: %s", app.app_id, download_progress);
    if (download_progress != null) {
        result.metadata.insert("show_progressbar", new Variant.boolean(true));
        result.metadata.insert("progressbar_source", new GLib.Variant.string(download_progress));
    }
    search_context.result_set.add_result (result);
  }


  public override void run ()
  {
  }

  public override void run_async (Unity.ScopeSearchBaseCallback async_callback)
  {
    // TODO: revert fake surfacing query
    if (search_context.search_query == "") {
        search_context.search_query = "a*";
    }
    // TODO: revert fake surfacing query ^^^^^^^^^^^^^

    var webservice = new ClickWebservice();
    webservice.search.begin(search_context.search_query, (obj, res) => {
        try {
            var apps = webservice.search.end (res);
            foreach (var app in apps) {
                add_result (app);
            }
        } catch (WebserviceError e) {
            debug ("Error calling webservice: %s", e.message);
            // TODO: warn about this some other way, like notifications
        }
        async_callback(this);
    });
  }
}


/* The GIO File for logging to. */
static File log_file = null;

/* Method to convert the log level name to a string */
static string _level_string (LogLevelFlags level)
{
	switch (level & LogLevelFlags.LEVEL_MASK) {
	case LogLevelFlags.LEVEL_ERROR:
		return "ERROR";
	case LogLevelFlags.LEVEL_CRITICAL:
		return "CRITICAL";
	case LogLevelFlags.LEVEL_WARNING:
		return "WARNING";
	case LogLevelFlags.LEVEL_MESSAGE:
		return "MESSAGE";
	case LogLevelFlags.LEVEL_INFO:
		return "INFO";
	case LogLevelFlags.LEVEL_DEBUG:
		return "DEBUG";
	}
	return "UNKNOWN";
}

static void ClickScopeLogHandler (string ? domain,
								  LogLevelFlags level,
								  string message)
{
	Log.default_handler (domain, level, message);

    try {
        var log_stream = log_file.append_to (FileCreateFlags.NONE);

        if (log_stream != null) {
            string log_message = "[%s] - %s: %s\n".printf(
                domain, _level_string (level), message);
            log_stream.write (log_message.data);
            log_stream.flush ();
            log_stream.close ();
        }
    } catch (GLib.Error e) {
        // ignore all errors when trying to log to disk
    }
}


int main ()
{
    var scope = new ClickScope();
    var exporter = new Unity.ScopeDBusConnector (scope);
	var cache_dir = Environment.get_user_cache_dir ();
	if (FileUtils.test (cache_dir, FileTest.EXISTS | FileTest.IS_DIR)) {
			var log_path = Path.build_filename (cache_dir,
												"unity-scope-click.log");
			log_file = File.new_for_path (log_path);
			Log.set_handler ("unity-scope-click", LogLevelFlags.LEVEL_MASK,
							 ClickScopeLogHandler);
	}

    try {
        exporter.export ();
    } catch (GLib.Error e) {
        error ("Cannot export scope to DBus: %s", e.message);
    }
    Unity.ScopeDBusConnector.run ();

    return 0;
}
