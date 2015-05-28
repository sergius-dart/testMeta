#include "dynamycmetaobject.h"

#include <private/qmetaobject_p.h>
#include <QDebug>
#include <QMetaMethod>


static inline const QMetaObjectPrivate *priv(const uint* data)
{ 
    return reinterpret_cast<const QMetaObjectPrivate*>(data);
}

static inline const QByteArray stringData(const QMetaObject *mo, int index)
{
    Q_ASSERT(priv(mo->d.data)->revision >= 7);
    const QByteArrayDataPtr data = { const_cast<QByteArrayData*>(&mo->d.stringdata[index]) };
    Q_ASSERT(data.ptr->ref.isStatic());
    Q_ASSERT(data.ptr->alloc == 0);
    Q_ASSERT(data.ptr->capacityReserved == 0);
    Q_ASSERT(data.ptr->size >= 0);
    return data;
}

static inline const char *rawStringData(const QMetaObject *mo, int index)
{
    return stringData(mo, index).data();
}

DynamycMetaObject::DynamycMetaObject(const QMetaObject *parent):
    QAbstractDynamicMetaObject(),
    injected( parent ),
    parentObj( 0 )
{
    d.stringdata = nullptr;
    //сразу надо создать метадату.
    makeMetaObject();
}

DynamycMetaObject::~DynamycMetaObject()
{
    
}

int DynamycMetaObject::getIdMethod(QByteArray signature)
    const
{
//    Q_ASSERT( signalList.length() + slotList.length() == methodCount() - methodOffset() );
    for( int indexMethod = methodOffset() ; indexMethod < methodCount() ; indexMethod++ )
    {
        if ( method(indexMethod).methodSignature() == signature )
            return indexMethod - methodOffset();
        indexMethod++;
    }
    return -1;
}


void DynamycMetaObject::addSignal(QByteArray signature)
{
    //проверяем есть ли такой метод?
    if ( getIdMethod( signature ) != -1 )
        return;
    //такого сигнала ещё нет добавляемс!  
    int signalIndex = priv(d.data)->signalCount ;
    auto makeSignalActivate = [ this, signalIndex]( void** args) mutable -> void
    {
        QMetaObject::activate( parentObj, this, signalIndex , args );
    };    
    
    MethodInfo mtInfo = buildInfoFromSignature( signature );

    mtInfo.p = new LambdaNativeCallHolder< decltype(makeSignalActivate) >( makeSignalActivate );
    
    signalList.push_back(mtInfo);
    
    makeMetaObject();
}

void DynamycMetaObject::addSlot( QByteArray signature, SlotFunc p )
{
    //проверяем есть ли такой метод?
    if ( getIdMethod( signature ) != -1 )
        return;
    MethodInfo mtInfo = buildInfoFromSignature( signature );
    mtInfo.p = p;
    
    slotList.push_back(mtInfo);
    makeMetaObject();
}



QAbstractDynamicMetaObject* DynamycMetaObject::toDynamicMetaObject(QObject *thisObj)
{ 
    //запоминаем объект, и начинаем обрабатывать )
    parentObj = thisObj;
//    d.superdata = &qptr->staticQtMetaObject;
//    d.superdata = dynamic_cast<testClass*>(qptr)->hackStaticVariables();
    return this;
}

/*!
 * \brief TestMetaObject::makeMetaObject - создает ВАЛИДНЫЙ метаобъект
 */
void DynamycMetaObject::makeMetaObject()
{
    clearMetaData();
    //правильное наследование!
    d.superdata = injected;

    //создаем остальные части метаинформации
    makeStringData();
    //создаем последнюю таблицу
    makeData();
}


QMetaObjectPrivate &DynamycMetaObject::getPrivateData() 
    const
{
    return *reinterpret_cast<QMetaObjectPrivate*>( const_cast<uint*>(d.data)) ;
}

DynamycMetaObject::DataMethodInfo &DynamycMetaObject::getDataMethodInfo( int id ) 
    const
{
    return *( reinterpret_cast<DynamycMetaObject::DataMethodInfo*>( 
                const_cast<uint*>(d.data + sizeof(QMetaObjectPrivate) / sizeof(uint) ) 
                ) + id );
}
/*!
 * \brief TestMetaObject::clearMetaData - очистка метаинформации. Освобождение всех выдеренных для метаинформации буферов.
 */
void DynamycMetaObject::clearMetaData()
{
    //если что-то не получиться, старая метаинформация будет все равно недоступна. Но все должно быть хорошо - мы ведь верим?
    if ( d.stringdata )
    {
        //удаляем сначало пул строк
        delete [] localVariables.allStringBuf;
        localVariables.allStringBuf = nullptr;
        //потом массив структур.
        delete [] const_cast<QArrayData*>(d.stringdata);
        d.stringdata = nullptr;
    }
    if ( d.data )
    {
        //удаляем основной массив метаинформации
        delete [] const_cast<uint*>(d.data);
        d.data = nullptr;
    }
}

/*!
 * \brief TestMetaObject::metaCall - вызывается только если мы прописались в QObject::d_ptr !!!
 * \param _c - что сделать
 * \param _id - индекс ( локальный )
 * \param _a - список аргументов
 * \return отрицательное число, если мы смогли обработать ( не можем обработать только если индекс не наш! )
 */
int DynamycMetaObject::metaCall(QObject * ptr, Call _c, int _id, void **_a)
{
    parentObj = ptr;
    int parserId = _id;
    //Проверяем сначало все что нам требуется.
    DynamycMetaObject * currentMeta = this;
    if ( currentMeta )
        return currentMeta->callMethod(_id - currentMeta->methodOffset() , _a );

    return  parentObj->qt_metacall(_c, parserId, _a);
}

int DynamycMetaObject::callMethod(int _id, void **_a)
{
    QVector<MethodInfo>* lst = nullptr;
    if ( _id < signalList.size() )
        lst = &signalList;
    else
    {
        _id -= signalList.length();
        lst = &slotList;
    }
    if ( lst && lst->size() > _id )
    {
        lst->at( _id ).p->call(_a);
        _id = -1;//раз мы нашли и запустили обработчик - больше нам делать ничего с этим вызовом ненадо. Возвращаем то что надо.
    }
    //мы обработали!
    return _id;
}

/*!
 * \brief TestMetaObject::makeStringData - создает валидную часть метаобъекта ( строки )
 */
void DynamycMetaObject::makeStringData()
{
    QByteArray infoData;
    QByteArray stringPool;
    
    /*Дальше идет описание методов и имен параметров. Если имя не задано задаем имя по умолчанию!*/
    //Вначале идут сигналы. Потом слоты А затем просто вызываемые методы.5
    
    int stringCount = 0;
    auto addString = [&stringCount, &infoData, &stringPool](QByteArray str )
    {
        //копируем саму строку
        str.append('\0');
        stringPool.append(str);

        //создаем структуру для хранения С-строк
        QArrayData tmpData {
            Q_REFCOUNT_INITIALIZE_STATIC,   //init ref !!!WARNING!!! - это хак для либ qt.
            str.length() - 1 ,              //size
            0,                              //alloc - подобрано
            0,                              //reversed - подобрано
            0                               //offset - инициализируется после
        };

        //добавляем в массив объектов наш временный объект. Добавляем полным копированием
        ///\todo ваще это плохая идея, но на безрыбье и рак помидор.
        infoData += QByteArray::fromRawData( reinterpret_cast<const char*>(&tmpData), sizeof(QArrayData));
        //считаем общее количество строк
        stringCount++;
    };
    addString("DynamycMetaObject");//add class name
    
    for( QVector<MethodInfo> list: {signalList, slotList/*, methodList*/} )
        Q_FOREACH( MethodInfo tmp, list )
        {
            addString(tmp.name.toLatin1());
            if ( !tmp.params.isEmpty() )
                for( ParamInfo param : tmp.params )
                    addString(param.name.toLatin1());
        }
    
    QArrayData * strdata = new QArrayData[stringCount];
    
    memcpy( reinterpret_cast<char*>(strdata) , infoData.constData(), sizeof(QArrayData) * stringCount );
    
    localVariables.allStringBuf = new char[ stringPool.length() ];
    memcpy( localVariables.allStringBuf, stringPool.constData(), stringPool.length() );
    
    //все скопировали - можно и ссылки поправить
    for( int i = 0, offset = 0 ; i < stringCount ; i++ )
    {
        qptrdiff localOffset = reinterpret_cast<qptrdiff>(localVariables.allStringBuf) - reinterpret_cast<qptrdiff>(&strdata[i]) + offset;
        offset+= strdata[i].size + 1;//учитываем \0 ещё в контейнерах ранее.
        strdata[i].offset = localOffset;
    }
    
    d.stringdata = strdata; //суем указатель на данные
}

/*!
 * \brief TestMetaObject::makeData - создает основной uint массив
 */
void DynamycMetaObject::makeData()
{
    QMetaObjectPrivate header;
    //заполнение структуры.
    header.revision = 7;
    
    header.className = 0;//id строки имени нашего класса. В нашем случае всегда 0
    header.classInfoCount = 0;
    header.classInfoData = 0;
    
    header.methodCount = signalList.length() + slotList.length();
    header.methodData = sizeof(QMetaObjectPrivate);//наше смещение, откуда начинается информация о метадате
    
    header.signalCount = signalList.length();
    //все что ниже - мы пока не поддерживаем поэтому там всего 0
    header.propertyCount = 0;
    header.propertyData = 0;
    
    header.enumeratorCount = 0;
    header.enumeratorData = 0;
    
    header.constructorCount = 0;
    header.constructorData = 0;
    
    header.flags = 0;
    
    QByteArray dataMethod;///массив для собрания информации о методах
    QByteArray dataParametrs;///массив для собрания информации о параметрах методов.
    uint stringIndex = 0;//пропускаем 0-й, так как это className
    //предварительная компиляция метадаты ( собрание описания методов и описания параметров к методам.
    for( QVector<MethodInfo> list: {signalList, slotList/*, methodList*/} )
        Q_FOREACH( MethodInfo tmp, list )
        {
            
            DataMethodInfo dtInfo{
                ++stringIndex,                              //string index
                (uint)tmp.params.length(),                  //count arguments
                (uint)(dataParametrs.length() / sizeof(uint)),//args offset( uint !!!!)
                2,                                          //tag ?
                0x0a                                        //flag?
            };
            dataMethod += QByteArray::fromRawData( reinterpret_cast<const char* > (&dtInfo), sizeof( DataMethodInfo ) );
            
            //пишем что возвращаем
            dataParametrs.append( QByteArray::fromRawData( reinterpret_cast<char*>(&tmp.returned), sizeof( QMetaType::Type ) ) );
            //дописываем аргументы
            Q_FOREACH( ParamInfo tp , tmp.params )
                dataParametrs.append( QByteArray::fromRawData( reinterpret_cast<char*>(&tp.type), sizeof( QMetaType::Type ) ) );
            //имена аргументов
            Q_FOREACH( ParamInfo tp , tmp.params )
            {
                Q_UNUSED(tp)
                dataParametrs.append( QByteArray::fromRawData( reinterpret_cast<char*>(&(++stringIndex)), sizeof( QMetaType::Type ) ) );
            }
        }
    
    size_t sizeBuf = sizeof(QMetaObjectPrivate) + 
            dataMethod.length() + 
            dataParametrs.length() + 
            1  //не забываем о последнем \0!
            ;
    
    //выделяем буфер
    uint* buf = new uint[sizeBuf] ;
    //запоминаем старт разных мест.
    DataMethodInfo* startMethodInfo = reinterpret_cast<DataMethodInfo*>( buf + sizeof( QMetaObjectPrivate) );
    uint* startParamTypes = reinterpret_cast<uint*>(startMethodInfo) + dataMethod.length();
    //заполняем все 0-ми На всякий случай.
    memset( buf, 0, sizeBuf );
    //копируем header
    memcpy( buf, 
            &header,
            sizeof( QMetaObjectPrivate)  );
    //копируем инфу о методах
    memcpy( startMethodInfo, 
            dataMethod.constData(),
            dataMethod.length() );
    //копируем инфу о параметрах методов
    memcpy( startParamTypes, 
            dataParametrs.constData() ,
            dataParametrs.length() );
    
    //Теперь надо поправить ссылки
    for( DataMethodInfo* info = startMethodInfo ; 
            info < reinterpret_cast<DataMethodInfo*>(startParamTypes); //пока не дошли до следующего блока
            info++ )
    {
        info->argOffset += sizeof( QMetaObjectPrivate) + dataMethod.length();
    }
    d.data = buf;
}

DynamycMetaObject::MethodInfo DynamycMetaObject::buildInfoFromSignature(QByteArray signature)
    const
{
    //по дефолту у нас метод с такой сигнатурой void <name> ()
    MethodInfo mtInfo{
        signature.left( signature.indexOf("(") ),   ///вырезаем имя
        {},                                         ///никаких параметров пока не будет
        QMetaType::Void,                            ///ничего не возвращаем!
        0                                           ///функция которая вызовет метод.
        //2a                                          ///флаги
    };
    
    QRegExp rexp("\\((.*)\\)");
    //если неверный формат - выходим!
    if ( rexp.indexIn( signature ) == -1 )
        return mtInfo;
    //получаем список параметров без скобок
    QString params = rexp.cap(1);
    //если параметров нету - просто выходим
    if ( params.isEmpty() )
        return mtInfo;
    //иначе разбираем их по запятым.
    for( QString nameParam : params.split(",") )
        mtInfo.params.push_back( buildParamInfo(nameParam.toLatin1()) );
    return mtInfo;
}

DynamycMetaObject::ParamInfo DynamycMetaObject::buildParamInfo(QString src) 
    const
{
    ParamInfo result{
        "undefVar",
        QMetaType::QVariant
    };
    
    if ( src.split(" ").isEmpty() )
        result.name = src;//тип у нас по дефолту остается QVariant, а имя нам надо будет выхватить.
    else
    {
        result.type =(QMetaType::Type) QMetaType::type( src.split(" ").first().toLatin1().data() );//указываем тип
        result.name = src.split(" ").last();//выхватываем имя
    }
    
    return result;
}
