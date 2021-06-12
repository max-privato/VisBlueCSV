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
    /* File che viene mandato in esecuzione iquando un file è droppato sulla finestra*/

    // Richiedo l'estensione "CSV"
    QString ext=name.mid(name.count()-3);
    if(ext.toLower()!="csv"){
        QMessageBox msgBox;
        msgBox.setText("The input file must have \"csv\" extension.");
        msgBox.exec();
        return;
    }
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
    if(ui->checkBox->isChecked()){
        outFileName.chop(1);
        outFileName= outFileName+"CMB.";
    }
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
/* In risposta alla pressione dell'ok qui effettuo la lettura e la trauzione
Faccio uso della funzione processLine(). */
{
    QList <QByteArray> names;
    QFile inFile(inFileName), inFile2(inFileName2);
    QByteArray line, line2;
    lineCount=0;
    offsetTime=0;
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
    if(ui->checkBox->isChecked()){
      if (!inFile2.open(QIODevice::ReadOnly | QIODevice::Text)){
         ui->txtName2Lbl->setText("Unable to open Second file for reading!");
         return;
      }
      //Per poter combinare i files la prima riga dev'essere identica
      // Leggo e interpreto la riga di intestazione
      line2 = inFile2.readLine();
      if(line!=line2){
        QMessageBox::warning(this, "VisBlueCSV",
         "Error: the two header lines do not match!");
       return;
      }
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
       outLine=processLine(line,false);
       if(outLine!="")
          out << outLine;
    }

    // qui, se richiesto, aggiungo le righe dal secondo file:
    if(ui->checkBox->isChecked()){
       while (!inFile2.atEnd()) {
         line = inFile2.readLine();
         lineCount++;
         QString outLine;
         outLine=processLine(line,true);
         if(outLine!="")
            out << outLine;
        }
    }
    outFile.close();
    ui->messageLbl->setText("File processed!");
    ui->pushButton->setEnabled(false);

}

QByteArray MainWindow::processLine(QByteArray line_, bool secondFile){
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

   static bool firstRow=true; //True se è la prima passata dentro processLine (quindi del primo file)
   static bool SecondFileIniTimeSet=false; //True se è stato selezionato l'istante iniziale del secondo file
   // Notare che i files di visblue partono tutti dalla mezzanotte, che per me diviene l'istante 0. Ciononostante il codice qui sotto è stato scritto per fare la differenza con l'istante iniziale, nell'ipotesi che possa non essere proprio la mezzanotte.
   QTime time;
   if(firstRow)
     startTime=QTime::fromString(timeByteArr);
   firstRow=false;
   if(secondFile && !SecondFileIniTimeSet){
      startTime=QTime::fromString(timeByteArr);
      SecondFileIniTimeSet=true;
   }
   time=QTime::fromString(timeByteArr);

   float timeSec=startTime.secsTo(time);
   // Aggiungo l'offset che è pari all'istante finale del primo file:
   if(secondFile)
     timeSec+=offsetTime;
   QByteArray timeSecBA;
   timeSecBA.setNum(timeSec);


   for (int i=0; i<items; i++){
     end=line_.indexOf(";",start);
     QByteArray field=(line_.mid(start,end-start));
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
   //la seguente riga serve per avere disponibile quando si fa la lettura del secondo file, il tempo da mettere come offset temporale
   if(!secondFile)
      offsetTime=timeSec;
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
