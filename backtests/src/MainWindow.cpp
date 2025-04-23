#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QTableWidgetItem>
#include <QDate>

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ui->tradesTable->setColumnCount(5);
    ui->tradesTable->setHorizontalHeaderLabels(
            QStringList{"Time", "Type", "Price", "Qty", "P/L"});
    // Set default date range: last month to today
    ui->fromDate->setDateTime(QDateTime(QDate::currentDate().addMonths(-1), QTime(0, 0)));
    ui->toDate->setDateTime(QDateTime(QDate::currentDate(), QTime(23, 59, 59)));


    // Initialize settings tab
    ui->marketCombo->addItems({"spot", "futures"});
    ui->commissionEdit->setPlaceholderText("Commission");
    ui->balanceEdit->setPlaceholderText("Initial Balance");
    ui->symbolEdit->setPlaceholderText("Symbol e.g. BTCUSDT");
    ui->intervalEdit->setPlaceholderText("1m,5m,1h");
    ui->leverageSpin->setRange(1, 100);
    ui->compileBtn->setText("Compile Strategy");
    ui->runBtn->setText("Run Backtest");


    // Set default strategy template
    static const char *defaultStrategyCode = R"(// Include necessary headers
#include <vector>

extern "C" int user_signal(const std::vector<double>& prices) {
    // Return 1 to BUY, 0 to SELL, -1 to HOLD
    if (prices.size() >= 20) {
        double sum = 0;
        for (size_t i = prices.size() - 20; i < prices.size(); ++i) sum += prices[i];
        double ma20 = sum / 20;
        return prices.back() > ma20 ? 1 : 0;
    }
    return -1;
}
)";
    ui->codeEdit->setPlainText(defaultStrategyCode);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_compileBtn_clicked() {
    QString log;
    if (!stratMgr_.compile(ui->codeEdit->toPlainText(), log)) {
        showCompileLog(log);
        QMessageBox::critical(this, "Compile Error", "Strategy compilation failed. See error below.");
    } else {
        showCompileLog(log);
        QMessageBox::information(this, "Compiled", "Strategy compiled successfully.");
    }
}

void MainWindow::showCompileLog(const QString &log) {
    ui->errorLabel->setText(log);
}

void MainWindow::on_runBtn_clicked() {
    ui->tradesTable->clearContents();
    ui->tradesTable->setRowCount(0);
    double com = ui->commissionEdit->text().toDouble();
    double bal = ui->balanceEdit->text().toDouble();
    bool fut = (ui->marketCombo->currentText() == "futures");
    int lev = ui->leverageSpin->value();
    QString sym = ui->symbolEdit->text();
    QString iv = ui->intervalEdit->text();
    QDateTime fromDT = ui->fromDate->dateTime();
    QDateTime toDT = ui->toDate->dateTime();
    int64_t fromMs = fromDT.toMSecsSinceEpoch();
    int64_t toMs = toDT.toMSecsSinceEpoch();

    Backtester bt(com, bal, fut, lev, fromMs, toMs, &stratMgr_);
    if (!bt.fetch_data(sym, iv)) {
        QMessageBox::warning(this, "Error", "Failed to fetch data for the given parameters.");
        return;
    }
    QMessageBox::information(this, "Info", QString("Loaded %1 bars").arg(bt.trades().size()));
    double final = bt.run();
    auto tr = bt.trades();
    ui->tradesTable->setRowCount(static_cast<int>(tr.size()));
    double totP = 0;
    for (int i = 0; i < static_cast<int>(tr.size()); ++i) {
        const auto &t = tr[i];
        ui->tradesTable->setItem(i, 0, new QTableWidgetItem(QDateTime::fromMSecsSinceEpoch(t.timestamp).toString()));
        ui->tradesTable->setItem(i, 1, new QTableWidgetItem(t.isBuy ? "BUY" : "SELL"));
        ui->tradesTable->setItem(i, 2, new QTableWidgetItem(QString::number(t.price)));
        ui->tradesTable->setItem(i, 3, new QTableWidgetItem(QString::number(t.qty)));
        ui->tradesTable->setItem(i, 4, new QTableWidgetItem(QString::number(t.pnl)));
        totP += t.pnl;
    }
    ui->summaryLabel->setText(QString("Final Bal: %1 | Total P/L: %2").arg(final).arg(totP));
    ui->tabWidget->setCurrentIndex(2);
}