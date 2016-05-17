/*
    Copyright 2014 Ilya Zhuravlev

    This file is part of Acquisition.

    Acquisition is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Acquisition is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Acquisition.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "steamlogindialog.h"
#include "ui_steamlogindialog.h"

#include <QCloseEvent>
#include "QsLog.h"
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QFile>
#include <QSettings>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

#include "filesystem.h"

SteamLoginDialog::SteamLoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SteamLoginDialog)
{
    ui->setupUi(this);
	//Create QT5.6 form of cookie store. (no longer a cookiejar: WebEngine cannot access NetworkAccessManager and therefore no cookiejar)
	cookie_store = ui->webView->page()->profile()->cookieStore();
	//link to the event handler when a cookie is added so We can manage the cookies ourself, because this new CookieStore is stupid.
	connect(cookie_store, &QWebEngineCookieStore::cookieAdded, this, &SteamLoginDialog::handleCookieAdded);
}

SteamLoginDialog::~SteamLoginDialog() {
    delete ui;
}

void SteamLoginDialog::closeEvent(QCloseEvent *e) {
    if (!completed_)
        emit Closed();
    QDialog::closeEvent(e);
}

void SteamLoginDialog::Init() {
	completed_, gotcookie2, gotcookie3 = false;
    disconnect(this, SLOT(OnLoadFinished()));
    cookie_store->loadAllCookies();
    ui->webView->setContent("Loading, please wait...");

	QUrl url = QUrl("https://www.pathofexile.com/login/steam");
	ui->webView->load(url);	//QTWebEngine cannot POST (wtf?)

    settings_path_ = Filesystem::UserDir() + "/settings.ini";
    SetSteamCookie(LoadSteamCookie());

    connect(ui->webView, SIGNAL(loadFinished(bool)), this, SLOT(OnLoadFinished()));
}

extern const char* POE_COOKIE_NAME;

//after 8 hours of debugging this, the weird behavior of the POESESSID has come to light
void SteamLoginDialog::OnLoadFinished() {
    if (completed_)
        return;
    QString url = ui->webView->url().toString();

	//cookie 1 = first page will load to the normal login area with no credentials, and set a POESESSID (useless)
	// ... read below for #2....
	//and cookie 3 will set a 3rd POESESSID (invalidating the other two) when it realizes we have authorized with STEAM
	if (url.startsWith("https://www.pathofexile.com")) {
		for (auto cookie : m_cookies){
			if (cookie.name() == POE_COOKIE_NAME && cookie.domain().contains("pathofexile.com")){
				session_id = QString(cookie.value());
				if (gotcookie2)
					gotcookie3 = true;
			}
		}
	}
	//cookie 2 will be detected once STEAM authorizes the machine (after a steam login), and set a 2nd POESESSID
	//then redirect back to the POE website 
	else if (url.startsWith("https://steamcommunity.com")){
		QSettings settings(settings_path_.c_str(), QSettings::IniFormat);
		bool rembme = settings.value("remember_me_checked").toBool();
		for (auto cookie : m_cookies) {
			if (cookie.name().startsWith("steamMachineAuth") && cookie.domain().contains("steamcommunity.com") && cookie.name().length() > 16) {
				//take note, there are not any "cookiesForUrl" methods
				gotcookie2 = true;
				if (rembme && !settings.contains("steamMachineAuth")) {
					SaveSteamCookie(cookie);
				}
				else if (rembme && cookie != LoadSteamCookie()) {
					SaveSteamCookie(cookie);
				}
			}
		}
	}

	if (gotcookie2 && gotcookie3){
		completed_ = true;
		emit CookieReceived(session_id);
		close();
	}
}

void SteamLoginDialog::SetSteamCookie(QNetworkCookie steam_cookie) {
    // Sets a cookie on the steamcommunity.com url so that users don't have to
    // do the email verification every time that they sign in on with steam.

    QNetworkCookie cookie;	//make a blank/initialized cookie. 
	if (cookie.value().length() > 1){
		cookie = steam_cookie;	//continue and load it in, but only if we have non-blank data
	}	//otherwise just load the initialized cookie in (better than loading half a named cookie with no data)

	cookie_store->setCookie(cookie, QUrl("https://steamcommunity.com/openid/"));
}

void SteamLoginDialog::SaveSteamCookie(QNetworkCookie cookie) {
    // Saves the file to a file located in the acquisition directory.

    QSettings settings(settings_path_.c_str(), QSettings::IniFormat);
    settings.setValue("steam-id", cookie.name().remove(0, 16));
    settings.setValue(cookie.name(), cookie.value());
}

QNetworkCookie SteamLoginDialog::LoadSteamCookie() {
    // Function to load the cookie from a file located in the acquisition
    // directory.

    QString cookie_name = "steamMachineAuth";
    QString cookie_value;
    QSettings settings(settings_path_.c_str(), QSettings::IniFormat);

    cookie_name.append(settings.value("steam-id").toString());
    cookie_value = settings.value(cookie_name).toString();

    QNetworkCookie cookie(cookie_name.toUtf8(), cookie_value.toUtf8());
    cookie.setPath("/");
    cookie.setDomain("steamcommunity.com");

    return cookie;
}

bool SteamLoginDialog::containsCookie(const QNetworkCookie &cookie)
{
	for (auto c : m_cookies) {
		if (c.hasSameIdentifier(cookie))
			return true;
	}
	return false;
}

void SteamLoginDialog::handleCookieAdded(const QNetworkCookie &cookie)
{
	// Find existing cookies and remove them, so we can re-add them, possibly with new values.
	if (containsCookie(cookie)){
		m_cookies.removeOne(cookie);
	}
	m_cookies.append(cookie);
}
