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
