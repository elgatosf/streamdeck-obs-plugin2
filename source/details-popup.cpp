// Copyright (C) 2020 - 2021 Corsair GmbH
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "obs-module.h"
#include "obs-frontend-api.h"
#include "ui_popup.h"

#include "server.hpp"
#include "version.hpp"

void plugin_show_details(void*)
{
	obs_frontend_push_ui_translation(obs_module_get_string);
	QWidget*      window = (QWidget*)obs_frontend_get_main_window();
	std::unique_ptr<Ui_DetailsPopup> ui(new Ui_DetailsPopup);
	QDialog popup(window);
	ui->setupUi(&popup);
	std::string obsVersionText = obs_module_text("OBSPluginVersion");
	std::string sdVersionText = obs_module_text("SDPluginVersion");
	obsVersionText += STREAMDECK_VERSION;
	sdVersionText += streamdeck::server::instance()->remote_version_string();
	ui->obsPluginVersion->setText(obsVersionText.c_str());
	ui->sdPluginVersion->setText(sdVersionText.c_str());
	popup.setFixedSize(popup.size());
	popup.setWindowFlags(popup.windowFlags() & ~Qt::WindowContextHelpButtonHint);
	popup.exec();
	obs_frontend_pop_ui_translation();
}
