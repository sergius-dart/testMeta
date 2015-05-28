#include "a.h"
#include "dynamycmetaobject.h"
#include "QDebug"

A::A(QObject *parent) : QObject(parent)
{
    
}

A::~A()
{
    
}

void A::installDynamyc()
{
    DynamycMetaObject::init(this);
}

void A::addSignal( bool first )
{
    static int indexSignal = 0;
    if ( first )
        indexSignal = 0;
    DynamycMetaObject* meta = static_cast<DynamycMetaObject*>(QObject::d_ptr->metaObject);
    if ( !meta )
    {
        qWarning() << "Dont init meta!";
        return;
    }
    meta->addSignal( QString("signal_" + QString::number(indexSignal) + "(QString a)").toLatin1() );
    indexSignal += 1;
}

void A::addSlot(bool first)
{
    static int indexSignal = 0;
    if ( first )
        indexSignal = 0;
    DynamycMetaObject* meta = static_cast<DynamycMetaObject*>(QObject::d_ptr->metaObject);
    if ( !meta )
    {
        qWarning() << "Dont init meta!";
        return;
    }
    auto slotMethod = [indexSignal](QString _arg){
        qDebug() << "called slot_" + QString::number(indexSignal) + " from arguments : " << _arg;
    };
    meta->addSlot( QString( "slot_" + QString::number(indexSignal) + "(QString a)").toLatin1(), new LambdaSlotHolder<decltype(slotMethod)>( slotMethod));
    indexSignal += 1;
}
