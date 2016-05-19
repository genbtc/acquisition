/*
    Copyright 2014 Ilya Zhuravlev

    This file is part of Acquisition.

    Acquisition is free software: you can redistribute it and/or Gearify
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

#include "geartypefilter.h"

#include <QComboBox>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include "QsLog.h"

#include "geartypelist.h"
#include "porting.h"

SelectedGearType::SelectedGearType(const std::string &name) :
    data_(name),
    Gear_select_(std::make_unique<QComboBox>()),
    delete_button_(std::make_unique<QPushButton>("x"))
{
    Gear_select_->setEditable(true);
	Gear_select_->addItems(gear_string_list);
    Gear_completer_ = new QCompleter(gear_string_list);
    Gear_completer_->setCompletionMode(QCompleter::PopupCompletion);
    Gear_completer_->setFilterMode(Qt::MatchContains);
    Gear_completer_->setCaseSensitivity(Qt::CaseInsensitive);
    Gear_select_->setCompleter(Gear_completer_);

    Gear_select_->setCurrentIndex(Gear_select_->findText(name.c_str()));

}

void SelectedGearType::Update() {
    data_.Gear = Gear_select_->currentText().toStdString();
}

enum LayoutColumn {
    kDeleteButton,
    kColumnCount
};

void SelectedGearType::AddToLayout(QGridLayout *layout, int index) {
    int combobox_pos = index * 2;
    int minmax_pos = index * 2 + 1;
    layout->addWidget(Gear_select_.get(), combobox_pos, 0, 1, LayoutColumn::kColumnCount);
    layout->addWidget(delete_button_.get(), minmax_pos, LayoutColumn::kDeleteButton);
}

void SelectedGearType::CreateSignalMappings(QSignalMapper *signal_mapper, int index) {
    QObject::connect(Gear_select_.get(), SIGNAL(currentIndexChanged(int)), signal_mapper, SLOT(map()));
    QObject::connect(delete_button_.get(), SIGNAL(clicked()), signal_mapper, SLOT(map()));

    signal_mapper->setMapping(Gear_select_.get(), index + 1);
    // hack to distinguish update and delete, delete event has negative (id + 1)
    signal_mapper->setMapping(delete_button_.get(), -index - 1);
}

void SelectedGearType::RemoveSignalMappings(QSignalMapper *signal_mapper) {
    signal_mapper->removeMappings(Gear_select_.get());
    signal_mapper->removeMappings(delete_button_.get());
}

GearTypeFilter::GearTypeFilter(QLayout *parent) :
    signal_handler_(*this)
{
    Initialize(parent);
    QObject::connect(&signal_handler_, SIGNAL(SearchFormChanged()), 
        parent->parentWidget()->window(), SLOT(OnDelayedSearchFormChange()));
}

void GearTypeFilter::FromForm(FilterData *data) {
	auto &geartype_data = data->geartype_data;
	geartype_data.clear();
	for (auto &Gear : geartypes_)
		geartype_data.push_back(Gear.data());
}

void GearTypeFilter::ToForm(FilterData *data) {
    Clear();
	for (auto &Gear : data->geartype_data)
		geartypes_.push_back(SelectedGearType(Gear.Gear));
    Refill();
}

void GearTypeFilter::ResetForm() {
    Clear();
    Refill();
}

bool GearTypeFilter::Matches(const std::shared_ptr<Item> &item, FilterData *data) {
	for (auto &Gear : data->geartype_data) {
        if (Gear.Gear.empty())
            continue;
        const GearTable &Gear_table = item->gear_table();
        if (!Gear_table.count(Gear.Gear))
            return false;
    }
    return true;
}

void GearTypeFilter::Initialize(QLayout *parent) {
    layout_ = std::make_unique<QGridLayout>();
    add_button_ = std::make_unique<QPushButton>("Add Gear");
    QObject::connect(add_button_.get(), SIGNAL(clicked()), &signal_handler_, SLOT(OnAddButtonClicked()));
    Refill();

    auto widget = new QWidget;
    widget->setContentsMargins(0, 0, 0, 0);
    widget->setLayout(layout_.get());
    parent->addWidget(widget);

    QObject::connect(&signal_mapper_, SIGNAL(mapped(int)), &signal_handler_, SLOT(OnGearChanged(int)));
}

void GearTypeFilter::AddGear() {
    SelectedGearType Gear("");
	geartypes_.push_back(std::move(Gear));
    Refill();
}

void GearTypeFilter::UpdateGear(int id) {
	geartypes_[id].Update();
}

void GearTypeFilter::DeleteGear(int id) {
	geartypes_.erase(geartypes_.begin() + id);

    Refill();
}

void GearTypeFilter::ClearSignalMapper() {
	for (auto &Gear : geartypes_) {
        Gear.RemoveSignalMappings(&signal_mapper_);
    }
}

void GearTypeFilter::ClearLayout() {
    QLayoutItem *item;
    while ((item = layout_->takeAt(0))) {}
}

void GearTypeFilter::Clear() {
    ClearSignalMapper();
    ClearLayout();
	geartypes_.clear();
}

void GearTypeFilter::Refill() {
    ClearSignalMapper();
    ClearLayout();

    int i = 0;
	for (auto &Gear : geartypes_) {
        Gear.AddToLayout(layout_.get(), i);
        Gear.CreateSignalMappings(&signal_mapper_, i);

        ++i;
    }

	layout_->addWidget(add_button_.get(), 2 * geartypes_.size(), 0, 1, LayoutColumn::kColumnCount);
}

void GearTypeFilterSignalHandler::OnAddButtonClicked() {
    parent_.AddGear();
}

void GearTypeFilterSignalHandler::OnGearChanged(int id) {
    if (id < 0)
        parent_.DeleteGear(-id - 1);
    else
        parent_.UpdateGear(id - 1);
    emit SearchFormChanged();
}
