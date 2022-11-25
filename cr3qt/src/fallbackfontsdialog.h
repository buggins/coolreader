/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2021 Aleksey Chernov <valexlin@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#ifndef FALLBACKFONTSDIALOG_H
#define FALLBACKFONTSDIALOG_H

#include <QDialog>
#include <QVector>
#include <QStringList>

namespace Ui {
	class FallbackFontsDialog;
}

class QHBoxLayout;
class QVBoxLayout;
class QComboBox;
class QToolButton;
class QSpacerItem;

class FallbackFontsDialog : public QDialog
{
	Q_OBJECT
private:
	struct FontControlItem {
		int pos;
		QHBoxLayout* layout;
		QComboBox* combobox;
		QToolButton* btnDel;
	};
public:
	explicit FallbackFontsDialog(QWidget *parent);
	explicit FallbackFontsDialog(QWidget *parent, const QStringList& availFaces);
	~FallbackFontsDialog();
	void setAvailableFaces(const QStringList& availFaces);
	QString fallbackFaces() {
		return m_fallbackFaces;
	}
	void setFallbackFaces(const QString& faces);
protected slots:
	void slot_delete_clicked();
	void slot_currectIndexChanged(int);
protected:
    FontControlItem* addFontItem(int pos, int face_idx);
    bool removeFontItem(int pos);
	void cleanupFontItems();
	void updateFallbackFaces();
private:
	QStringList m_availableFaces;
	QString m_fallbackFaces;
	Ui::FallbackFontsDialog *ui;
	QVBoxLayout* m_layout;
	QSpacerItem *m_spacer;
	// data
	QVector<FontControlItem*> m_items;
};

#endif // FALLBACKFONTSDIALOG_H
