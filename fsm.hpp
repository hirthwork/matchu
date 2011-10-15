#ifndef __FSM_HPP_2011_10_11__
#define __FSM_HPP_2011_10_11__

#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace NReinventedWheels
{
    template <class TChar,
        template <class TKey,
            class TValue,
            class = std::less<TKey>,
            class = std::allocator<std::pair<const TKey, TValue> > >
        class TTransitionTable>
    struct TFA
    {
        typedef TTransitionTable<TChar, unsigned> TState;
        typedef typename std::vector<TState> TStates;
        // not empty
        TStates States;

        typedef std::vector<unsigned> TAcceptStates;
        // ordered, not empty
        TAcceptStates AcceptStates;
    };

    template <class TChar>
    struct TDFA : TFA<TChar, std::map>
    {
    };

    template <class TChar>
    struct TNFA : TFA<TChar, std::multimap>
    {
    };
}

#endif

