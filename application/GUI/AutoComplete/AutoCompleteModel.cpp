/**
  * D-LAN - A decentralized LAN file sharing software.
  * Copyright (C) 2010-2012 Greg Burri <greg.burri@gmail.com>
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */
  
#include <AutoComplete/AutoCompleteModel.h>
using namespace GUI;

AutoCompleteModel::AutoCompleteModel()
{
}

AutoCompleteModel::~AutoCompleteModel()
{

}

void AutoCompleteModel::setFilter(const QString& pattern)
{

}

QModelIndex AutoCompleteModel::index(int row, int column, const QModelIndex& parent) const
{
   return QModelIndex();
}

QModelIndex AutoCompleteModel::parent(const QModelIndex& child) const
{
   return QModelIndex();
}

int AutoCompleteModel::rowCount(const QModelIndex& parent) const
{
   return 0;
}

int AutoCompleteModel::columnCount(const QModelIndex& parent) const
{
   return 0;
}

QVariant AutoCompleteModel::data(const QModelIndex& index, int role) const
{
   return QVariant();
}
