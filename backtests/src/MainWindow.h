#ifndef TRADINGVIEW_SERVER_MAINWINDOW_H
#define TRADINGVIEW_SERVER_MAINWINDOW_H

#pragma once

#include <QMainWindow>
#include "backtester.h"
#include "StrategyManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

private slots:

    void on_compileBtn_clicked();

    void on_runBtn_clicked();

private:
    Ui::MainWindow *ui;
    StrategyManager stratMgr_;

    void showCompileLog(const QString &log);
};

#endif //TRADINGVIEW_SERVER_MAINWINDOW_H
