#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QMimeData>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    move(90,0);
    setAcceptDrops(true);
    ui->label_4->setText("");
    initialLabel3Text=ui->label_3->text();
    // valuto i parametri passati
    QStringList args;
    for(int i=1; i<QCoreApplication::arguments().count(); i++){
      args.append (QCoreApplication::arguments().at(i));
     }
    int iii=0;
    if(args.count()>0)
      receiveFileName(args[0]);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();//mette il puntatore in posizione di accettazione
                                  // puts the pointer in the accept position
}

void MainWindow::dropEvent(QDropEvent *event)
{
    /* Funzione per il caricamento dei files che vengono "droppati" sulla finestra*/
    /* Function for loading files that are "dropped" on the window */

    const QMimeData *mimeData = event->mimeData();
    receiveFileName(mimeData->urls().at(0).path(),true);
}

void MainWindow::receiveFileName(QString name, bool dropped){
    inFileName= name;
    // solo se il file è stato droppato dopo l'apertura va eliminato il primo carattere:
    if(dropped)
      inFileName.remove(0,1);
    outFileName=inFileName;
    outFileName.chop(3);
    outFileName=outFileName+"ADF";
    ui->label_4->setText(inFileName);
    ui->label_3->setText(initialLabel3Text);
    ui->label_3->setEnabled(true);
    ui->pushButton->setEnabled(true);

}

void MainWindow::on_pushButton_clicked()
{
    QList <QByteArray> names;

    QFile inFile(inFileName);
    QByteArray line;
    lineCount=0;
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)){
       ui->label_3->setText("Unable to open file for reading!");
       return;
    }
    QFile outFile(outFileName);
     if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)){
         ui->label_3->setText("Unable to open file for writing!");
        return;
     }
    QTextStream out(&outFile);

    // Leggo e interpreto la riga di intestazione
    line = inFile.readLine();
    // I primi due caratteri vanno esclusi: probabilmente contengono info tipo BOM
    line=line.mid(3);
    int startNameIdx=0, endNameIdx;

    /* C'è un errore nel CV di visblue: viene usato il carattere ';' all'interno del nome delle variabili degli allarmi. Questo rende il file illogico, in quanto questo carattere deve separare i campi. In attesa che l'errore sia risolto direttamente da Visblue, lo correggo qui con un workaround.
*/
    line.replace("[0=Ok;1=Warning;2=Ala","[0=Ok 1=Warning 2=Ala");


    do {
        if(startNameIdx!=0)
            startNameIdx++;
       endNameIdx=line.indexOf(";",startNameIdx);
       QByteArray name=(line.mid(startNameIdx,endNameIdx-startNameIdx));
       //Elimino gli spazi:
       int index=0;
       while((index=name.indexOf(' ',index))>=0)
           name[index]='_';
       names.append(name);
       startNameIdx=endNameIdx;
    }   while(startNameIdx>-1);

    // Nei files fornite ci sono 7 nomi in più nelle righe di intestazione rispetto ai valori che poi si trovano dopo:
//    items=names.count()-7;
    items=names.count();

    QByteArray header="";
    for (int i=0; i<items-1; i++){
      header+=names[i];
      header+=",";
    }
    header+=names[names.count()-1];
    out <<"\n"<< header;

    //Lettura e interpretazione righe di dati
    while (!inFile.atEnd()) {
       line = inFile.readLine();
       lineCount++;
       QString outLine;
       outLine=processLine(line);
       if(outLine!="")
//           out << outLine<<"\n";
          out << outLine;
    }
    outFile.close();
    ui->label_3->setText("File processed!");
}

QByteArray MainWindow::processLine(QByteArray line_){
  /* Questa funzione analizza la riga in ingresso, letta dall'output del Visblue
   * e determina la stringa contenente la corrispondente riga da scrivere poi sul file
   * di uscita.
  */
   int start=0, end;
   QByteArray ret, timeByteArr;
   QList <QByteArray> fields;

   //Individuo la stringa che contiene il tempo:
   end=line_.indexOf(";",start);
   //calcolo il tempo in secondi nell'ipotesi che per ogni prova non vi sia il cambio di data:
   start=line_.indexOf(" ",start)+1;
   timeByteArr=line_.mid(start,end-start);

   static bool firstRun=true;
   QTime time;
   if(firstRun)
     startTime=QTime::fromString(timeByteArr);
   firstRun=false;
   time=QTime::fromString(timeByteArr);

   float timeSec=startTime.secsTo(time);
   QByteArray timeSecBA;
   timeSecBA.setNum(timeSec);
   //Voglio mettere nel file di ouput il tempo in secondi ponendo pari a 0 l'inizio della prova, Pertanto devo fare la differenza fra il tempo corrente e il tempo iniziale:

   for (int i=0; i<items; i++){
     end=line_.indexOf(";",start);
     QByteArray field=(line_.mid(start,end-start));
     //Visblue usa icome separatore decimale la vigola, PlotXY il punto. QUindi faccio la conversione:
     int index;
     if((index=field.indexOf(','))>-1)
        field[index]='.';
     fields.append(field);
     start=end+1;
   }

   // Al posto del campo data-time, metto il tempo in secondi:


   fields.replace(0,timeSecBA);

   QString str;

   ret="";
   int iii= fields.count();
   //Nel seguente loop salto gli ultimi due campi che contengono dati che non sono di mio interesse
   for (int i=0; i<items-1; i++){
     ret+=fields[i];
     ret+=",";
   }
   ret+=fields[fields.count()-1];
   return ret;
}

