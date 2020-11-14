#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QMimeData>
#include <QCoreApplication>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    move(90,0);
    setAcceptDrops(true);
    ui->name1Lbl->setText("");
    initialLabel3Text=ui->txtName2Lbl->text();
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
    // solo se il file è stato droppato dopo l'apertura va eliminato il primo carattere:
    if(dropped)
      name.remove(0,1);
    // se è stato selezionato combine il file che si sta ricevendo è il secondo (inFileName2) che va poi combinato col primo (inFIleName):
    if(ui->checkBox->isChecked())
       inFileName2= name;
    else
       inFileName= name;
    outFileName=inFileName;
    outFileName.chop(3);
    // Se sto combinando due CSV in un unico file di uscita aggiungo la strinca CMB per "combinato"
    if(ui->checkBox->isChecked())
        outFileName= outFileName+"CMB";
    outFileName=outFileName+"ADF";
    if(ui->checkBox->isChecked())
      ui->name2Lbl->setText(inFileName2);
    else
      ui->name1Lbl->setText(inFileName);
    ui->txtName2Lbl->setText(initialLabel3Text);
    ui->txtName2Lbl->setEnabled(true);
    ui->pushButton->setEnabled(true);
    ui->checkBox->setEnabled(true);
}

void MainWindow::on_pushButton_clicked()
{
    QList <QByteArray> names;

    QFile inFile(inFileName), inFile2(inFileName2);
    QByteArray line, line2;
    lineCount=0;
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)){
       ui->txtName2Lbl->setText("Unable to open file for reading!");
       return;
    }
    QFile outFile(outFileName);
     if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)){
         ui->txtName2Lbl->setText("Unable to open file for writing!");
        return;
     }
    QTextStream out(&outFile);

    // Leggo e interpreto la riga di intestazione
    line = inFile.readLine();
    // A questo punto, se sono in combine, leggo anche la prima riga del secondo file:
    if (!inFile2.open(QIODevice::ReadOnly | QIODevice::Text)){
       ui->txtName2Lbl->setText("Unable to open Second file for reading!");
       return;
    }
    //Per poter combinare i files la prima riga dev'essere identica
    // Leggo e interpreto la riga di intestazione
    line2 = inFile2.readLine();
    if(line!=line2){
        int ret = QMessageBox::warning(this, "VisBlueCSV",
              "Error: the two header lines do not match!");
      return;
    }

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
    ui->txtName2Lbl->setText("File processed!");
    ui->pushButton->setEnabled(false);

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
   //Nel seguente loop salto gli ultimi due campi che contengono dati che non sono di mio interesse
   for (int i=0; i<items-1; i++){
     ret+=fields[i];
     ret+=",";
   }
   ret+=fields[fields.count()-1];
   return ret;
}


void MainWindow::on_checkBox_clicked(bool checked)
{
  if (checked){
    ui->txtName2Lbl->setEnabled(true);
    ui->name2Lbl->setEnabled(true);
    ui->okLbl->setEnabled(false);
    ui->pushButton->setEnabled(false);
  }else{
    ui->txtName2Lbl->setEnabled(false);
    ui->name2Lbl->setEnabled(false);
    ui->okLbl->setEnabled(true);
    ui->pushButton->setEnabled(true);
  }
}
