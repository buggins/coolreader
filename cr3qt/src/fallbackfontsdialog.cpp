#include "fallbackfontsdialog.h"
#include "ui_fallbackfontsdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QToolButton>
#include <QSpacerItem>

FallbackFontsDialog::FallbackFontsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FallbackFontsDialog)
{
    ui->setupUi(this);
    m_layout = new QVBoxLayout;
    m_spacer = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_layout->addItem(m_spacer);
    ui->frame->setLayout(m_layout);
}

FallbackFontsDialog::FallbackFontsDialog(QWidget* parent, const QStringList& availFaces) :
    QDialog(parent),
    ui(new Ui::FallbackFontsDialog)
{
    ui->setupUi(this);
    m_layout = new QVBoxLayout;
    m_spacer = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_layout->addItem(m_spacer);
    ui->frame->setLayout(m_layout);
    setAvailableFaces(availFaces);
}

FallbackFontsDialog::~FallbackFontsDialog()
{
    cleanupFontItems();
    delete ui;
}

void FallbackFontsDialog::setAvailableFaces(const QStringList& availFaces)
{
    m_availableFaces = availFaces;
}

void FallbackFontsDialog::setFallbackFaces(const QString& faces)
{
    m_fallbackFaces = faces;
    QStringList list;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    list = faces.split(";", Qt::SkipEmptyParts);
#else
    // TODO: for Qt older than 5.14
#endif
    cleanupFontItems();
    m_layout->removeItem(m_spacer);
    QStringList::const_iterator it;
    int item_idx = 0;
    FontControlItem* item;
    for (it = list.begin(); it != list.end(); ++it) {
        QString face = (*it).trimmed();
        int face_idx = m_availableFaces.indexOf(face);
        if (face_idx >= 0) {
            item = addFontItem(item_idx, face_idx);
            m_layout->addLayout(item->layout);
            item_idx++;
        }
    }
    // add empty item
    item = addFontItem(item_idx, -1);
    m_layout->addLayout(item->layout);
    m_layout->addItem(m_spacer);
}

void FallbackFontsDialog::slot_delete_clicked()
{
    int item_idx = -1;
    QVariant itemIdxProp;
    QObject* signalSender = sender();
    if (signalSender)
        itemIdxProp = signalSender->property("ITEMIDX");
    if (itemIdxProp.isValid()) {
        bool ok = false;
        int tmp = itemIdxProp.toInt(&ok);
        if (ok)
            item_idx = tmp;
    }
    if (item_idx >= 0 && item_idx < m_items.size()) {
        bool res = removeFontItem(item_idx);
        if (res) {
            if (m_items.size() > 0) {
                FontControlItem* lastItem = m_items[m_items.size() - 1];
                if (lastItem && lastItem->btnDel)
                    lastItem->btnDel->setEnabled(false);
            }
            updateFallbackFaces();
        }
    }
}

void FallbackFontsDialog::slot_currectIndexChanged(int index)
{
    int item_idx = -1;
    QVariant itemIdxProp;
    QObject* signalSender = sender();
    if (signalSender)
        itemIdxProp = signalSender->property("ITEMIDX");
    if (itemIdxProp.isValid()) {
        bool ok = false;
        int tmp = itemIdxProp.toInt(&ok);
        if (ok)
            item_idx = tmp;
    }
    if (item_idx >= 0 && item_idx < m_items.size()) {
        FontControlItem* item = m_items[item_idx];
        if (item_idx == m_items.size() - 1) {
            if (item && item->combobox && !item->combobox->currentText().isEmpty()) {
                item->btnDel->setEnabled(true);
                // last empty item changed
                FontControlItem* newItem = addFontItem(m_items.size(), -1);
                m_layout->removeItem(m_spacer);
                m_layout->addLayout(newItem->layout);
                m_layout->addItem(m_spacer);
            }
        } else if (item_idx == m_items.size() - 2) {
            if (item && item->combobox && item->combobox->currentText().isEmpty()) {
                FontControlItem* lastItem = m_items[m_items.size() - 1];
                if (lastItem && lastItem->combobox && lastItem->combobox->currentText().isEmpty()) {
                    // remove empty last item
                    removeFontItem(m_items.size() - 1);
                    item->btnDel->setEnabled(false);
                }
            }
        }
        updateFallbackFaces();
    }
}

FallbackFontsDialog::FontControlItem *FallbackFontsDialog::addFontItem(int pos, int face_idx)
{
    FontControlItem* item = new FontControlItem;
    item->pos = pos;
    item->layout = new QHBoxLayout;
    item->combobox = new QComboBox(ui->frame);
    item->combobox->setProperty("ITEMIDX", pos);
    item->combobox->addItem("");
    item->combobox->addItems(m_availableFaces);
    if (face_idx >= 0)
        item->combobox->setCurrentIndex(face_idx + 1);
    item->btnDel = new QToolButton(ui->frame);
    item->btnDel->setProperty("ITEMIDX", pos);
    item->btnDel->setEnabled(face_idx >= 0);
    item->btnDel->setIcon(QIcon(":/icons/action/icons/fileclose.png"));
    item->btnDel->setToolTip(tr("Remove this fallback font"));
    item->layout->addWidget(item->combobox, 10);
    item->layout->addWidget(item->btnDel, 0);
    connect(item->btnDel, SIGNAL(clicked()), this, SLOT(slot_delete_clicked()));
    connect(item->combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_currectIndexChanged(int)));
    m_items.append(item);
    return item;
}

bool FallbackFontsDialog::removeFontItem(int pos)
{
    if (pos >= 0 && pos < m_items.size()) {
        FontControlItem* item = m_items[pos];
        if (item) {
            if (item->layout) {
                m_layout->removeItem(item->layout);
                if (item->btnDel) {
                    item->btnDel->disconnect();
                    item->layout->removeWidget(item->btnDel);
                    delete item->btnDel;
                }
                if (item->combobox) {
                    item->combobox->disconnect();
                    item->layout->removeWidget(item->combobox);
                    delete item->combobox;
                }
                delete item->layout;
            }
            delete item;
            m_items.remove(pos);
            // update item's position
            for (int i = pos; i < m_items.size(); i++) {
                FontControlItem* item = m_items[i];
                item->btnDel->setProperty("ITEMIDX", i);
                item->combobox->setProperty("ITEMIDX", i);
                item->pos = i;
            }
        }
        return true;
    }
    return false;
}

void FallbackFontsDialog::cleanupFontItems()
{
    QVector<FontControlItem*>::const_iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it) {
        const FontControlItem* item = *it;
        if (item) {
            if (item->layout) {
                if (item->btnDel) {
                    item->btnDel->disconnect();
                    item->layout->removeWidget(item->btnDel);
                    delete item->btnDel;
                }
                if (item->combobox) {
                    item->combobox->disconnect();
                    item->layout->removeWidget(item->combobox);
                    delete item->combobox;
                }
                m_layout->removeItem(item->layout);
                delete item->layout;
            }
            delete item;
        }
    }
    m_items.clear();
}

void FallbackFontsDialog::updateFallbackFaces()
{
    m_fallbackFaces = QString();
    for (int i = 0; i < m_items.size() - 1; i++) {
        FontControlItem* item = m_items[i];
        if (item) {
            if (item->combobox) {
                QString text = item->combobox->currentText();
                if (!text.isEmpty()) {
                    m_fallbackFaces.append(text);
                    if (i < m_items.size() - 2) {
                        m_fallbackFaces.append("; ");
                    }
                }
            }
        }
    }
}
