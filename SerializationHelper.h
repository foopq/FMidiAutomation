/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __SERIALIZATIONHELPER_H
#define __SERIALIZATIONHELPER_H

#include <map>
#include <memory>

#include <boost/serialization/split_free.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/any.hpp>

#include <iostream>

/*
struct SharedPtrMapSingletonBase
{
    virtual void ResetSharedPtrMapSingleton() = 0;
};//SharedPtrMapSingletonBase

void RegisterSharedPtrMapSingletonBase(SharedPtrMapSingletonBase *base);
void ResetSharedPtrMapSingletonMaps();

template <typename T>
struct SharedPtrMapSingleton : public virtual SharedPtrMapSingletonBase
{
    static SharedPtrMapSingleton<T> &Instance();
    virtual void ResetSharedPtrMapSingleton();

    SharedPtrMapSingleton()
    {
        RegisterSharedPtrMapSingletonBase(this);
    }//constructor

    virtual ~SharedPtrMapSingleton() {}

    static std::shared_ptr<T> GetSharedPtr(T *raw)
    {
        if (raw == NULL) {
            return std::shared_ptr<T>();
        }//if

        SharedPtrMapSingleton<T> &instance = Instance();

        auto ptrMapIter = instance.ptrMap.find(raw);    
        if (ptrMapIter != instance.ptrMap.end()) {
            return ptrMapIter->second;
        } else {
            std::shared_ptr<T> newPtr(raw);
            instance.ptrMap[raw] = newPtr;
            return newPtr;
        }//if
    }//GetSharedPtr

private:
    std::map<T*, std::shared_ptr<T> > ptrMap;
};//SharedPtrMapSingleton
*/

extern std::map<void *, boost::any> sharedPtrMapSingletonList;

inline void ResetSharedPtrMapSingletonList()
{
    sharedPtrMapSingletonList.clear();
}//ResetSharedPtrMapSingletonList

template <typename T>
struct SharedPtrMapSingleton
{
    static std::shared_ptr<T> GetSharedPtr(T *raw)
    {
        if (raw == NULL) {
            return std::shared_ptr<T>();
        }//if

        auto ptrMapIter = sharedPtrMapSingletonList.find(raw);    
        if (ptrMapIter != sharedPtrMapSingletonList.end()) {
            boost::any anyVal = ptrMapIter->second;
            std::shared_ptr<T> retPtr = boost::any_cast<std::shared_ptr<T> >(anyVal);
//            std::cout << "here1: " << sharedPtrMapSingletonList.size() << " - " << raw << std::endl;           
            return retPtr;
        } else {
//            std::cout << "here2: " << sharedPtrMapSingletonList.size() << "-" << raw << std::endl;
            
            std::shared_ptr<T> newPtr(raw);
            sharedPtrMapSingletonList[raw] = newPtr;
            return newPtr;
        }//if
    }//GetSharedPtr
};//SharedPtrMapSingleton



namespace boost {
namespace serialization{

template<class Archive, class T>
inline void save(Archive & ar, const std::shared_ptr<T> &t, const unsigned int file_version)
{
    unsigned int version = file_version;
    ar & BOOST_SERIALIZATION_NVP(version);

    const T *ptr = t.get();
    ar & BOOST_SERIALIZATION_NVP(ptr);
}//save

template<class Archive, class T>
inline void load(Archive & ar, std::shared_ptr<T> &t, const unsigned int file_version)
{
    unsigned int version;
    ar & BOOST_SERIALIZATION_NVP(version);

    T *ptr;
    ar & BOOST_SERIALIZATION_NVP(ptr);
    t = SharedPtrMapSingleton<T>::GetSharedPtr(ptr);
}//load

template<class Archive, class T>
inline void serialize(Archive & ar, std::shared_ptr<T> &t, const unsigned int file_version)
{
    boost::serialization::split_free(ar, t, file_version);
}//serialize

template<class Archive, class T>
inline void save(Archive & ar, const std::weak_ptr<T> &t, const unsigned int file_version)
{
    unsigned int version = file_version;
    ar & BOOST_SERIALIZATION_NVP(version);

    std::shared_ptr<T> ptr = t.lock();
    ar & BOOST_SERIALIZATION_NVP(ptr);
}//save

template<class Archive, class T>
inline void load(Archive & ar, std::weak_ptr<T> &t, const unsigned int file_version)
{
    unsigned int version;
    ar & BOOST_SERIALIZATION_NVP(version);

    std::shared_ptr<T> ptr;
    ar & BOOST_SERIALIZATION_NVP(ptr);
    t = ptr;
}//load

template<class Archive, class T>
inline void serialize(Archive & ar, std::weak_ptr<T> &t, const unsigned int file_version)
{
    boost::serialization::split_free(ar, t, file_version);
}//serialize

} // namespace serialization
} // namespace boost

#endif // STD_SERIALIZATION_SHARED_PTR_HPP
