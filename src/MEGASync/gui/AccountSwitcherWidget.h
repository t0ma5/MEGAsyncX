#ifndef ACCOUNT_SWITCHER_WIDGET_H
#define ACCOUNT_SWITCHER_WIDGET_H

#include <QStringList>
#include <QWidget>

class MegaApplication;
class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidgetItem;
class QListWidget;
class QPushButton;

class AccountSwitcherWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AccountSwitcherWidget(MegaApplication* app, QWidget* parent = nullptr);

private slots:
    void refresh();
    void onSwitch();
    void onAdd();
    void onRemove();
    void onRemoveAllInactive();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onContextMenu(const QPoint& position);
    void updateButtons();
    void onImport();
    void onExport();
    void onForgetAll();

private:
    QString selectedEmail() const;
    QStringList selectedEmails() const;
    void removeAccounts(const QStringList& emails);

    MegaApplication* mApp;
    QLabel* mHeading;
    QLineEdit* mFilter;
    QListWidget* mList;
    QPushButton* mSwitchButton;
    QPushButton* mAddButton;
    QPushButton* mRemoveButton;
    QPushButton* mRemoveAllButton;
    QPushButton* mImportButton;
    QPushButton* mExportButton;
    QPushButton* mForgetAllButton;
    QCheckBox* mConfirmSwitch;
    QCheckBox* mOpenFiles;
    QCheckBox* mSortByFreeSpace;
};

#endif
