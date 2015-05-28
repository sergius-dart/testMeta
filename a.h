#ifndef A_H
#define A_H

#include <QObject>

#include "dynamycmetaobject.h"

class A : public QObject
{
    Q_OBJECT
public:
    friend class DynamycMetaObject;
    explicit A(QObject *parent = 0);
    ~A();
    
    void installDynamyc();
    
    void addSignal(bool first = false );
    void addSlot( bool first = false );
    
signals:
    void signal();
public slots:
    void slot(){}
};

#endif // A_H
