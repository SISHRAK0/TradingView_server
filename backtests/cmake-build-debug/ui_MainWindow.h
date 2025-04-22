/********************************************************************************
** Form generated from reading UI file 'MainWindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *settingsTab;
    QFormLayout *formLayout;
    QLineEdit *commissionEdit;
    QLineEdit *balanceEdit;
    QLineEdit *symbolEdit;
    QComboBox *marketCombo;
    QSpinBox *leverageSpin;
    QLineEdit *intervalEdit;
    QDateEdit *fromDate;
    QDateEdit *toDate;
    QPushButton *compileBtn;
    QPushButton *runBtn;
    QWidget *strategyTab;
    QVBoxLayout *strategyLayout;
    QTextEdit *codeEdit;
    QLabel *errorLabel;
    QWidget *tradesTab;
    QVBoxLayout *tradesLayout;
    QTableWidget *tradesTable;
    QLabel *summaryLabel;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName("tabWidget");
        settingsTab = new QWidget();
        settingsTab->setObjectName("settingsTab");
        formLayout = new QFormLayout(settingsTab);
        formLayout->setObjectName("formLayout");
        commissionEdit = new QLineEdit(settingsTab);
        commissionEdit->setObjectName("commissionEdit");

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, commissionEdit);

        balanceEdit = new QLineEdit(settingsTab);
        balanceEdit->setObjectName("balanceEdit");

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, balanceEdit);

        symbolEdit = new QLineEdit(settingsTab);
        symbolEdit->setObjectName("symbolEdit");

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, symbolEdit);

        marketCombo = new QComboBox(settingsTab);
        marketCombo->setObjectName("marketCombo");

        formLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, marketCombo);

        leverageSpin = new QSpinBox(settingsTab);
        leverageSpin->setObjectName("leverageSpin");

        formLayout->setWidget(2, QFormLayout::ItemRole::LabelRole, leverageSpin);

        intervalEdit = new QLineEdit(settingsTab);
        intervalEdit->setObjectName("intervalEdit");

        formLayout->setWidget(2, QFormLayout::ItemRole::FieldRole, intervalEdit);

        fromDate = new QDateEdit(settingsTab);
        fromDate->setObjectName("fromDate");
        fromDate->setCalendarPopup(true);

        formLayout->setWidget(3, QFormLayout::ItemRole::LabelRole, fromDate);

        toDate = new QDateEdit(settingsTab);
        toDate->setObjectName("toDate");
        toDate->setCalendarPopup(true);

        formLayout->setWidget(3, QFormLayout::ItemRole::FieldRole, toDate);

        compileBtn = new QPushButton(settingsTab);
        compileBtn->setObjectName("compileBtn");

        formLayout->setWidget(4, QFormLayout::ItemRole::LabelRole, compileBtn);

        runBtn = new QPushButton(settingsTab);
        runBtn->setObjectName("runBtn");

        formLayout->setWidget(4, QFormLayout::ItemRole::FieldRole, runBtn);

        tabWidget->addTab(settingsTab, QString());
        strategyTab = new QWidget();
        strategyTab->setObjectName("strategyTab");
        strategyLayout = new QVBoxLayout(strategyTab);
        strategyLayout->setObjectName("strategyLayout");
        codeEdit = new QTextEdit(strategyTab);
        codeEdit->setObjectName("codeEdit");

        strategyLayout->addWidget(codeEdit);

        errorLabel = new QLabel(strategyTab);
        errorLabel->setObjectName("errorLabel");

        strategyLayout->addWidget(errorLabel);

        tabWidget->addTab(strategyTab, QString());
        tradesTab = new QWidget();
        tradesTab->setObjectName("tradesTab");
        tradesLayout = new QVBoxLayout(tradesTab);
        tradesLayout->setObjectName("tradesLayout");
        tradesTable = new QTableWidget(tradesTab);
        tradesTable->setObjectName("tradesTable");

        tradesLayout->addWidget(tradesTable);

        summaryLabel = new QLabel(tradesTab);
        summaryLabel->setObjectName("summaryLabel");

        tradesLayout->addWidget(summaryLabel);

        tabWidget->addTab(tradesTab, QString());

        verticalLayout->addWidget(tabWidget);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        tabWidget->setTabText(tabWidget->indexOf(settingsTab), QCoreApplication::translate("MainWindow", "Settings", nullptr));
        errorLabel->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(strategyTab), QCoreApplication::translate("MainWindow", "Strategy", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tradesTab), QCoreApplication::translate("MainWindow", "Trades", nullptr));
        (void)MainWindow;
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
