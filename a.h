#ifndef A_H
#define A_H

#include <QObject>

#include "dynamycmetaobject.h"

/**
 * @brief The A class - данный класс показывает взаимодействие динамического метаобъекта с статическим.
 */
class A : public QObject
{
    Q_OBJECT
public:
    friend class DynamycMetaObject;
    explicit A(QObject *parent = 0);
    ~A();
    
    //функция "устанавливает" в данный объект динамический метаобъект.
    void installDynamyc();
    
    //Просто добавляем что хотим)
    void addSignal(bool first = false );
    void addSlot( bool first = false );
    
    //Ну и естественно нам надо чтобы метаинформация не была пустая.
signals:
    void signal();
public slots:
    void slot(){}
};

#endif // A_H
