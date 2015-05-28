#ifndef HOLDERS
#define HOLDERS


#ifndef _MSC_VER
#include <cxxabi.h>
#endif

#ifndef _MSC_VER
template <typename T> char* dumpType()
{
  int status;
  char * realname = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
  return realname;
}

#else
template <typename T> void dumpType()
{
  return typeid(T).name();
}
#endif

/*!
 * \brief The BaseHolder struct - структура для хранения указателей на функции. 
 */
struct BaseHolder{
    virtual ~BaseHolder(){}
    virtual void call( void** _a ) = 0;
};
/*!
 * \brief Данный класс по хорошему хранит указатель на функцию. По хорошему. Не тестировался.
 */
template <typename Func>
class FuncHolder:public BaseHolder{
public:
    typedef QtPrivate::FunctionPointer<Func> FuncType;

    FuncHolder(Func t){ _f = reinterpret_cast<void*>(&t); }
private:
    void* _f;
public:
    virtual void operator()( void** _a ){
        FuncType::template call< typename FuncType::Arguments, typename FuncType::ReturnType>( *reinterpret_cast<Func*>(_f),0, _a);
    }
};
/*!
 * \brief Так как у нас в основном придется хранить лямбды, то этот класс хранит любую лямбду.
 */
template <typename Func>
class LambdaSlotHolder:public BaseHolder{
public:
    typedef QtPrivate::FunctionPointer<decltype(&Func::operator())> FuncType;

    LambdaSlotHolder(const Func &t):_obj(t){}
private:
    Func _obj;
public:
    virtual void call( void** _a ){
        FuncType::template call< typename FuncType::Arguments, typename FuncType::ReturnType>( &Func::operator(), &_obj, _a);
    }
};

template <typename Func>
class LambdaNativeCallHolder:public BaseHolder{
public:
    typedef QtPrivate::FunctionPointer<decltype(&Func::operator())> FuncType;

    LambdaNativeCallHolder(const Func &t):_obj(t){}
private:
    Func _obj;
public:
    virtual void call( void** _a ){
        ((&_obj)->operator())(_a);
    }
};


#endif // HOLDERS

