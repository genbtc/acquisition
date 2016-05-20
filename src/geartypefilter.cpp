#pragma region Header License
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
#pragma endregion


#include "geartypefilter.h"
#include "geartypelist.h"

#include <QComboBox>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include "QsLog.h"
#include "porting.h"


#pragma region Front End Heavy Lifting - UI

//Viewmodel constructor
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

//text updater
void SelectedGearType::Update() {
    data_.Gearname = Gear_select_->currentText().toStdString();
}

//insert into UI
void SelectedGearType::AddToLayout(QGridLayout *layout, int index) {;
    layout->addWidget(Gear_select_.get(), index, 0);
    layout->addWidget(delete_button_.get(), index, 1);
}
//event handler:
void SelectedGearType::CreateSignalMappings(QSignalMapper *signal_mapper, int index) {
    QObject::connect(Gear_select_.get(), SIGNAL(currentIndexChanged(int)), signal_mapper, SLOT(map()));
    QObject::connect(delete_button_.get(), SIGNAL(clicked()), signal_mapper, SLOT(map()));

    signal_mapper->setMapping(Gear_select_.get(), index + 1);
    // hack to distinguish update and delete, delete event has negative (id + 1)
    signal_mapper->setMapping(delete_button_.get(), -index - 1);
}
//event handler:
void SelectedGearType::RemoveSignalMappings(QSignalMapper *signal_mapper) {
    signal_mapper->removeMappings(Gear_select_.get());
    signal_mapper->removeMappings(delete_button_.get());
}

//Model Constructor
GearTypeFilter::GearTypeFilter(QLayout *parent) :
signal_handler_(*this)
{
    Initialize(parent);
    QObject::connect(&signal_handler_, SIGNAL(SearchFormChanged()),
        parent->parentWidget()->window(), SLOT(OnDelayedSearchFormChange()));
}

//Model initializer
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
#pragma endregion


#pragma region Data Filtering
//*1) FromForm: provided with a FilterData fill it with data from form
void GearTypeFilter::FromForm(FilterData *data) {
    auto &geartype_data = data->geartype_data;
    geartype_data.clear();
    for (auto &Gear : current_gear_filters_)
        geartype_data.push_back(Gear.data());
}
//*2) ToForm: provided with a FilterData fill form with data from it
void GearTypeFilter::ToForm(FilterData *data) {
    Clear();
    for (auto &Gear : data->geartype_data)
        current_gear_filters_.push_back(SelectedGearType(Gear.Gearname));
    Refill();
}

//*3) Matches: check if an item matches the filter provided with FilterData
bool GearTypeFilter::Matches(const std::shared_ptr<Item> &item, FilterData *data) {
    for (auto &Gear : data->geartype_data) {
        if (Gear.Gearname.empty())
            return true;
        for (auto props : item->text_properties()){
            if (props.name.find(Gear.Gearname) != std::string::npos)
                return true;
        }
        return false;
    }
    return true;
}
#pragma endregion


#pragma region Member Functions Back End - DataStructure
void GearTypeFilter::ResetForm() {
    Clear();
    Refill();
}

void GearTypeFilter::AddGear() {
    current_gear_filters_.push_back(std::move(SelectedGearType("")));
    Refill();
}

void GearTypeFilter::UpdateGear(int id) {
    current_gear_filters_[id].Update();
}

void GearTypeFilter::DeleteGear(int id) {
    current_gear_filters_.erase(current_gear_filters_.begin() + id);
    Refill();
}

void GearTypeFilter::ClearSignalMapper() {
    for (auto &Gear : current_gear_filters_) {
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
    current_gear_filters_.clear();
}

void GearTypeFilter::Refill() {
    ClearSignalMapper();
    ClearLayout();

    int i = 0;
    for (auto &Gear : current_gear_filters_) {
        Gear.AddToLayout(layout_.get(), i);
        Gear.CreateSignalMappings(&signal_mapper_, i);
        ++i;
    }
    layout_->addWidget(add_button_.get(), 2 * current_gear_filters_.size(), 0, 2 * current_gear_filters_.size(), 2);
    //the code says addWidget(row,column,rowspan,columnspan) HOWEVER, this was not the case.
    // in actuality it was addWidget(fromRow,fromColumn,toRow,toColumn)
    // 2 * current_gear_filters_.size() means 2 rows for however many filters are added)
}
#pragma endregion


#pragma region Signal handlers
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

#pragma endregion
