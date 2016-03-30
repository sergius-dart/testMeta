#ifndef TESTMETAOBJECT_H
#define TESTMETAOBJECT_H

#include <QMetaObject>
#include <private/qobject_p.h>
#include <QObject>
#include <QMetaType>
#include "holders.h"

class QMetaObjectPrivate;
class DynamycMetaObject : public QAbstractDynamicMetaObject
{
private:
    ~DynamycMetaObject();
public:
    DynamycMetaObject(const QMetaObject *parent );
    
    ///инициализация динамического метаобъекта, и подстановка его в нужное место
    template < typename T >
    static void init(T * ptr)
    {
        if ( ptr->d_ptr->metaObject ) // уже есть динамический метаобъект. Тогда мы ничего не делаем.
        {
                qWarning() << "DynamycMetaObject is installed!" << ptr->d_ptr->metaObject << "\n" <<
                              "not installed now!";
                return;
        }
        else {
            ptr->d_ptr->metaObject = new DynamycMetaObject( &T::staticMetaObject );//инициализация динамического метаобъекта с помощью статической методаты нашего класса.
            //При использовании указателей на базовые классы получим очень неприятный эффект
        }
    }
    
    typedef BaseHolder* SlotFunc;
    
    ///получение Id метода по его сигнатуре
    int getIdMethod( QByteArray signature ) const;
    ///добавление сигнала по сигнатуре
    void addSignal( QByteArray signature);
    ///добавление слота по сигнатуре. 
    void addSlot ( QByteArray signature, SlotFunc p );
      
    ///"создаем динамический метаобъект"
    virtual QAbstractDynamicMetaObject *toDynamicMetaObject(QObject * thisObj);    
protected:
    
    /*!
     * \brief The dataMethodInfo struct - структура для основного uint массива.
     */
    struct DataMethodInfo{
        uint name;
        uint argsCount;
        uint argOffset;
        uint tag;
        uint flags;
    };
    ///создание основной метадаты
    void makeMetaObject();
    void clearMetaData();
    
protected: 
    /*Дополнительные функции*/
    QMetaObjectPrivate & getPrivateData() const;
    DataMethodInfo &getDataMethodInfo(int id) const;
    
    
private:
    const QMetaObject * injected;         ///< куда заинжектились. Надо для создания правильного метаобъекта
    QObject * parentObj;///указывает с каким объектом работаем
    struct ParamInfo{
        QString name;
        QMetaType::Type type;
    };
    
    
    ///для хранения 
    struct MethodInfo{
        QString                 name/*{"undef"}*/;      ///< имя метода
        QList<ParamInfo>        params/*{}*/;           ///< список аргументов
        QMetaType::Type         returned/*{QMetaType::Void}*/;
        SlotFunc                p/*{0}*/;               ///< указатель лямбду которая вызовет метод.
//        int                     flags;
    };
    
    
    QVector < MethodInfo > signalList;
    QVector < MethodInfo > slotList;

    struct {
        int stringCount{0};
        char* allStringBuf{nullptr};
    } localVariables;///"лишние" переменные, пока не придумал куда деть
    
//Перегрузка методов из QAbstractDynamicMetaObject
    ///требование вызова метода метаобъекта ( чаще всего, остальные варианты пока не рассматриваем )
    virtual int metaCall(QObject * obj, Call _c, int _id, void **_a);
    ///запускаем наш метод по внутреннему id
    virtual int callMethod(int _id, void **_a);
    
private:
    ///создание строковой структуры
    void makeStringData();
    ///создание основного uint массива
    void makeData();
    
    //вспомогательные функции
    MethodInfo buildInfoFromSignature( QByteArray signature ) const;///сигнатуры вида slot(Type1 name1,Type2 name2).
    ParamInfo buildParamInfo( QString src)const;

};


#endif // TESTMETAOBJECT_H
