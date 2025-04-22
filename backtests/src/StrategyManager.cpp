#include "StrategyManager.h"
#include <QProcess>
#include <QLibrary>
#include <fstream>
#include <QCoreApplication>

using CreateFn = int (*)(const std::vector<double> &);

StrategyManager::StrategyManager() = default;

bool StrategyManager::compile(const QString &code, QString &log) {
    // Save strategy code to file in application directory
    QString projectDir = QCoreApplication::applicationDirPath();
    QString src = projectDir + "/user_strategy.cpp";
    std::ofstream f(src.toStdString());
    if (!f) {
        log = "Failed to open file for writing: " + src;
        return false;
    }
    f << code.toStdString();
    f.close();

    // Compile command
    QString so = projectDir + "/libuser_strategy.so";
    QProcess p;
    p.setProgram("g++");
    p.setArguments({"-shared", "-fPIC", src, "-o", so, "-std=c++17"});
    p.setWorkingDirectory(projectDir);
    p.start();
    if (!p.waitForStarted()) {
        log = "Failed to start compiler process.";
        return false;
    }
    // Wait max 10 seconds
    if (!p.waitForFinished(10000)) {
        p.kill();
        log = "Compiler timed out.";
        return false;
    }
    log = p.readAllStandardError() + p.readAllStandardOutput();
    soPath_ = so;
    return (p.exitCode() == 0);
}

int StrategyManager::user_signal(const std::vector<double> &prices) {
    QLibrary lib(soPath_);
    if (!lib.load()) return -1;
    auto fn = (CreateFn) lib.resolve("user_signal");
    if (!fn) return -1;
    return fn(prices);
}