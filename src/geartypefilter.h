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

#pragma once

#include "filters.h"

#include <QGridLayout>
#include <QObject>
#include <QSignalMapper>
#include <vector>
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>
#include <QPushButton>

class SelectedGearType {
public:
    SelectedGearType(const std::string &name);
    SelectedGearType(const SelectedGearType&) = delete;
    SelectedGearType& operator=(const SelectedGearType&) = delete;
    SelectedGearType(SelectedGearType&& o) : 
        data_(std::move(o.data_)),
        Gear_select_(std::move(o.Gear_select_)),
        delete_button_(std::move(o.delete_button_))
    {}
    SelectedGearType& operator=(SelectedGearType&& o) {
        data_ = std::move(o.data_);
        Gear_select_ = std::move(o.Gear_select_);
        delete_button_ = std::move(o.delete_button_);
        return *this;
    }

    void Update();
    void AddToLayout(QGridLayout *layout, int index);
    void CreateSignalMappings(QSignalMapper *signal_mapper, int index);
    void RemoveSignalMappings(QSignalMapper *signal_mapper);
    const GearTypeFilterData &data() const { return data_; }
private:
    GearTypeFilterData data_;
    std::unique_ptr<QComboBox> Gear_select_;
    QCompleter *Gear_completer_;
    std::unique_ptr<QPushButton> delete_button_;
};

class GearTypeFilter;

class GearTypeFilterSignalHandler : public QObject {
    Q_OBJECT
public:
    GearTypeFilterSignalHandler(GearTypeFilter &parent) :
        parent_(parent)
    {}
signals:
    void SearchFormChanged();
private slots:
    void OnAddButtonClicked();
    void OnGearChanged(int id);
private:
    GearTypeFilter &parent_;
};

class GearTypeFilter : public Filter {
    friend class GearTypeFilterSignalHandler;
public:
    explicit GearTypeFilter(QLayout *parent);
    void FromForm(FilterData *data);
    void ToForm(FilterData *data);
    void ResetForm();
    bool Matches(const std::shared_ptr<Item> &item, FilterData *data);
private:
    void Clear();
    void ClearSignalMapper();
    void ClearLayout();
    void Initialize(QLayout *parent);
    void Refill();
    void AddGear();
    void UpdateGear(int id);
    void DeleteGear(int id);

    std::unique_ptr<QGridLayout> layout_;
    std::unique_ptr<QPushButton> add_button_;
    std::vector<SelectedGearType> geartypes_;
    GearTypeFilterSignalHandler signal_handler_;
    QSignalMapper signal_mapper_;
};
