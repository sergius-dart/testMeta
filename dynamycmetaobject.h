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
    
    template < typename T >
    static void init(T * ptr)
    {
        if ( (ptr->d_ptr->metaObject) )
            if (DynamycMetaObject * ptrMeta = dynamic_cast<DynamycMetaObject*>(ptr->d_ptr->metaObject)) 
            {
                qWarning() << "DynamycMetaObject is installed!" << ptrMeta << "\n" <<
                              "not installed now!";
            } else {}
        else {
            ptr->d_ptr->metaObject = new DynamycMetaObject( &T::staticMetaObject );
        }
    }
    
    typedef BaseHolder* SlotFunc;
    
    int getIdMethod( QByteArray name ) const;
    void addSignal( QByteArray signature);
    void addSlot ( QByteArray signature, SlotFunc p );
      
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
    
    void makeMetaObject();
    
    QMetaObjectPrivate & getPrivateData() const;
    DataMethodInfo &getDataMethodInfo(int id) const;
    void clearMetaData();
    
    
private:
    const QMetaObject * injected;         ///< куда заинжектились. Надо для создания правильного метаобъекта
    QObject * parentObj;
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
    } localVariables;
    virtual int metaCall(QObject * obj, Call _c, int _id, void **_a);

    
    virtual int callMethod(int _id, void **_a);
    
    DynamycMetaObject * next {nullptr};
    
private:
    void makeStringData();
    void makeData();
    
    MethodInfo buildInfoFromSignature( QByteArray signature ) const;///сигнатуры вида slot(Type1 name1,Type2 name2).
    
    ParamInfo buildParamInfo( QString src)const;

};


#endif // TESTMETAOBJECT_H
