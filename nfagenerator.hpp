#ifndef __NFAGENRATOR_HPP_2011_10_13__
#define __NFAGENRATOR_HPP_2011_10_13__

#include <algorithm>
#include <functional>
#include <utility>

#include "fsm.hpp"
#include "token.hpp"

namespace NReinventedWheels
{
    template <class TChar>
    class TNFAGenerator
    {
        typedef typename TNFA<TChar>::TState TState;
        typedef typename TNFA<TChar>::TStates TStates;
        typedef typename TNFA<TChar>::TAcceptStates TAcceptStates;

        // moves all states to the first list
        static void ConcatenateStates(TStates& first, TStates& second)
        {
            typename TStates::size_type size = first.size();
            first.insert(first.end(), second.size(), TState());
            for(typename TStates::iterator from = second.begin(),
                end = second.end(), to = first.begin() + size; from != end;
                ++from, ++to)
            {
                for(typename TState::iterator transition = from->begin(),
                    end = from->end(); transition != end; ++transition)
                {
                    transition->second += size;
                }
                to->swap(*from);
            }
        }

        static TNFA<TChar> ConcatenateNFAs(TNFA<TChar> first,
            TNFA<TChar> second)
        {
            // TODO: elide 'second' start state if there is no return to it
            typename TStates::size_type size = first.States.size();
            ConcatenateStates(first.States, second.States);

            TNFA<TChar> result;
            std::transform(second.AcceptStates.begin(),
                second.AcceptStates.end(),
                second.AcceptStates.begin(),
                std::bind1st(std::plus<unsigned>(), size));
            if(second.AcceptStates.front() == size)
            {
                result.AcceptStates = first.AcceptStates;
                result.AcceptStates.insert(result.AcceptStates.end(),
                    second.AcceptStates.begin(), second.AcceptStates.end());
            }
            else
            {
                result.AcceptStates.swap(second.AcceptStates);
            }

            result.States.swap(first.States);
            const TState& start = result.States[size];
            for(typename TAcceptStates::const_iterator state =
                first.AcceptStates.begin(), end = first.AcceptStates.end();
                state != end; ++state)
            {
                if (*state == 1)
                {
                    TState tmp;
                    tmp.swap(result.States[*state]);
                    tmp.insert(start.begin(), start.end());
                    result.States[*state].swap(tmp);
                }
                else
                {
                    result.States[*state].insert(start.begin(), start.end());
                }
            }
            return result;
        }

        static inline TNFA<TChar> AlternateNFAs(TNFA<TChar> first,
            TNFA<TChar> second)
        {
            // TODO: elide start state if there is no return to it
            TNFA<TChar> result;
            typename TStates::size_type size = first.States.size();
            result.States.reserve(1 + size + second.States.size());
            result.States.push_back(TState());
            ConcatenateStates(result.States, first.States);
            ConcatenateStates(result.States, second.States);
            const bool acceptStartState =
                !(first.AcceptStates.front() && second.AcceptStates.front());
            // automatically fills start state if required
            result.AcceptStates.resize(acceptStartState
                + first.AcceptStates.size() + second.AcceptStates.size());
            typename TAcceptStates::iterator acceptStatesEnd = std::transform(
                first.AcceptStates.begin(),
                first.AcceptStates.end(),
                result.AcceptStates.begin() + acceptStartState,
                std::bind1st(std::plus<unsigned>(), 1));
            std::transform(second.AcceptStates.begin(),
                second.AcceptStates.end(),
                acceptStatesEnd,
                std::bind1st(std::plus<unsigned>(), 1 + size));
            TState& start = result.States.front();
            start = result.States[1];
            const TState& secondStart = result.States[1 + size];
            start.insert(secondStart.begin(), secondStart.end());
            return result;
        }

        static TNFA<TChar> ClosureNFA(TNFA<TChar> nfa)
        {
            // TODO: elide start state if existing one already acceptable
            TNFA<TChar> result;
            typename TStates::size_type size = nfa.States.size();
            result.States.reserve(1 + size);
            result.States.push_back(TState());
            ConcatenateStates(result.States, nfa.States);
            // automatically fills start state
            result.AcceptStates.resize(1 + nfa.AcceptStates.size());
            std::transform(nfa.AcceptStates.begin(), nfa.AcceptStates.end(),
                result.AcceptStates.begin() + 1,
                std::bind1st(std::plus<unsigned>(), 1));
            const TState& start = result.States[1];
            result.States.front() = start;
            for(typename TAcceptStates::const_iterator state =
                result.AcceptStates.begin() + 1, end =
                result.AcceptStates.end(); state != end; ++state)
            {
                if (*state == 1)
                {
                    TState tmp;
                    tmp.swap(result.States[*state]);
                    tmp.insert(start.begin(), start.end());
                    result.States[*state].swap(tmp);
                }
                else
                {
                    result.States[*state].insert(start.begin(), start.end());
                }
            }
            return result;
        }

        static inline TNFA<TChar> CreateCharacterNFA(const TChar& character)
        {
            TNFA<TChar> result;
            result.States.resize(2);
            result.States.front().insert(std::make_pair(character, 1));
            result.AcceptStates.push_back(1);
            return result;
        }

        static inline TNFA<TChar> CreateEmptyNFA()
        {
            TNFA<TChar> result;
            result.States.push_back(TState());
            result.AcceptStates.push_back(0);
            return result;
        }

    public:
        static inline TNFA<TChar> CreateNFA(const INode* root)
        {
            if(root->GetNodeType() == TNodeType::Operation)
            {
                const IOperation* operation =
                    static_cast<const IOperation*>(root);
                switch(operation->GetOperationType())
                {
                    case TOperationType::Concatenation:
                        return ConcatenateNFAs(
                            CreateNFA(operation->Children[0]),
                            CreateNFA(operation->Children[1]));

                    case TOperationType::Alternation:
                        // TODO: provide ability to alternate vector of NFAs
                        return AlternateNFAs(
                            CreateNFA(operation->Children[0]),
                            CreateNFA(operation->Children[1]));

                    case TOperationType::Closure:
                        return ClosureNFA(CreateNFA(operation->Children[0]));

                    default:
                        throw std::logic_error("unknown operation type");
                }
            }
            else
            {
                const IToken* token = static_cast<const IToken*>(root);
                if(token->GetTokenType() == TTokenType::Character)
                {
                    const TCharacter<TChar>* character =
                        static_cast<const TCharacter<TChar>*>(token);
                    return CreateCharacterNFA(character->Character);
                }
                else
                {
                    return CreateEmptyNFA();
                }
            }
        }
    };
}

#endif

