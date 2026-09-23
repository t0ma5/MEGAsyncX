#ifndef ACCOUNT_SWITCHER_WIDGET_H
#define ACCOUNT_SWITCHER_WIDGET_H

#include <QWidget>

class MegaApplication;
class QLineEdit;
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
    void onImport();
    void onExport();
    void onForgetAll();

private:
    QString selectedEmail() const;

    MegaApplication* mApp;
    QLineEdit* mFilter;
    QListWidget* mList;
    QPushButton* mSwitchButton;
    QPushButton* mAddButton;
    QPushButton* mRemoveButton;
    QPushButton* mImportButton;
    QPushButton* mExportButton;
    QPushButton* mForgetAllButton;
};

#endif
