#ifndef ACCOUNT_SWITCHER_WIDGET_H
#define ACCOUNT_SWITCHER_WIDGET_H

#include <QWidget>

class MegaApplication;
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

private:
    QString selectedEmail() const;

    MegaApplication* mApp;
    QListWidget* mList;
    QPushButton* mSwitchButton;
    QPushButton* mAddButton;
    QPushButton* mRemoveButton;
};

#endif
