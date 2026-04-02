#ifndef MININGPAGE_H
#define MININGPAGE_H

#include <QWidget>
#include <QProcess>
#include <QTimer>

namespace Ui {
class MiningPage;
}

class MiningPage : public QWidget
{
    Q_OBJECT

public:
    explicit MiningPage(QWidget *parent = 0);
    ~MiningPage();

private Q_SLOTS:
    void startMining();
    void stopMining();
    void startStratum();
    void stopStratum();
    void startDaemon();
    void stopDaemon();
    void updateStats();
    void onMinerOutput();
    void onStratumOutput();
    void onDaemonOutput();
    void onThreadSliderChanged(int value);

private:
    Ui::MiningPage *ui;
    QProcess *minerProcess;
    QProcess *stratumProcess;
    QProcess *daemonProcess;
    QTimer *statsTimer;
    QString localIPStr;
    int acceptedShares;
    int connectedWorkers;
};

#endif // MININGPAGE_H
