/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#ifndef __PAIRHELPER_H
#define __PAIRHELPER_H

#include <utility>

template <typename T1, typename T2>
struct fmaipair
{
    T1 first;
    T2 second;
    
    T1 begin() { return first; }
    T2 end() { return second; }

    fmaipair() : first(), second() {}

    fmaipair(const T1 &t1, const T2 &t2) : first(t1), second(t2) {}

    fmaipair(const std::pair<T1, T2> &&p) : first(p.first), second(p.second) {}

    fmaipair &operator=(const fmaipair& __p)
    {
        first = __p.first;
        second = __p.second;
        return *this;
    }

    fmaipair &operator=(fmaipair&& __p)
      {
        first = std::move(__p.first);
        second = std::move(__p.second);
        return *this;
      }

    template<class _U1, class _U2>
    fmaipair &operator=(const fmaipair<_U1, _U2>& __p)
    {
        first = __p.first;
        second = __p.second;
        return *this;
    }

    template<class _U1, class _U2>
    fmaipair &operator=(fmaipair<_U1, _U2>&& __p)
    {
        first = std::move(__p.first);
        second = std::move(__p.second);
        return *this;
    }

};//fmapair

template <typename T1, typename T2>
fmaipair<T1, T2> fmai_make_pair(T1 &&t1, T2 &&t2)
{
    return fmaipair<T1, T2>(std::forward<T1>(t1), std::forward<T2>(t2));
}//fmai_make_pair


#endif
