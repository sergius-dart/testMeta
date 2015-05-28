#include <QCoreApplication>
#include "dynamycmetaobject.h"
#include "a.h"
#include <QDebug>
#include <QMetaMethod>
void dumpMeta( QObject * ptr )
{
    const QMetaObject * meta = ptr->metaObject();
    qDebug() << "--------DUMP-META-OBJECT--------------------";
    qDebug() << " className     - " << meta->className() << '\n' <<
                "methodCount   - " << meta->methodCount() << '\n' <<
                "methodOffset  - " << meta->methodOffset() << '\n';
    qDebug() << "methods from TOP meta";
    for( int i = meta->methodOffset(); i < meta->methodCount() ; i++ )
    {
        QMetaMethod method = meta->method( i );
        qDebug() << QString( " I " + QString::number(i - meta->methodOffset() ) ).leftJustified(6) <<
                    QString( " RI# " + QString::number( i ) ).leftJustified(4) <<
                    method.methodSignature();
    }    
    qDebug() << "--------------------------------------------";
}



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
 
    qDebug() << "I = Relative Index\nRI = Real Index";
    
    A testClass;
    
    dumpMeta(&testClass);
    
    testClass.installDynamyc();

    dumpMeta(&testClass);        
    
    testClass.addSignal(true);
    testClass.addSlot(true);
    
    dumpMeta(&testClass);
    
    testClass.addSignal();
    testClass.addSlot();
    
    dumpMeta(&testClass);
    
    return /*a.exec()*/0;
}
