#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDropEvent>
#include <QTime>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    ~MainWindow() override;

private slots:
    void on_pushButton_clicked();

private:
    QTime startTime;  //Istante iniziale della prova memorizzata sul file
    int items,// numero di campi che scriverò nel file di uscita:
    lineCount;
    QString inFileName, outFileName, initialLabel3Text;
    //pari a quelli di input -2 in quanto gli ultimi 2 non li riporto
    Ui::MainWindow *ui;
    QByteArray processLine(QByteArray line_);
    void receiveFileName(QString name, bool dropped=false);

};
#endif // MAINWINDOW_H
