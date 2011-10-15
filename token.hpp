#ifndef __TOKEN_HPP_2011_10_11__
#define __TOKEN_HPP_2011_10_11__

#include <iterator>
#include <stdexcept>

namespace NReinventedWheels
{
    struct TNodeType
    {
        enum TType
        {
            Operation,
            Token
        };
    };

    struct INode
    {
        virtual inline ~INode()
        {
        }

        virtual TNodeType::TType GetNodeType() const = 0;
    };

    class TNodePtr
    {
        const INode* Node;

        TNodePtr(const TNodePtr&);
        TNodePtr& operator = (const TNodePtr&);

    public:
        inline TNodePtr(const INode* node = 0)
            : Node(node)
        {
        }

        inline TNodePtr(TNodePtr& ptr)
            : Node(ptr.Release())
        {
        }

        inline ~TNodePtr()
        {
            delete Node;
        }

        inline const INode* operator -> () const
        {
            return Node;
        }

        inline const INode* Release()
        {
            const INode* result = Node;
            Node = 0;
            return result;
        }

        inline void Set(const INode* node)
        {
            delete Node;
            Node = node;
        }

        inline const INode* Get() const
        {
            return Node;
        }
    };

    struct TOperationType
    {
        enum TType
        {
            Concatenation,
            Alternation,
            Closure
        };
    };

    struct IOperation: INode
    {
        const INode* Children[2];

        inline IOperation(const INode* left, const INode* right = 0)
        {
            Children[0] = left;
            Children[1] = right;
        }

        inline ~IOperation()
        {
            delete Children[0];
            delete Children[1];
        }

        virtual inline TNodeType::TType GetNodeType() const
        {
            return TNodeType::Operation;
        }

        virtual TOperationType::TType GetOperationType() const = 0;
    };

    struct TConcatenation: IOperation
    {
        inline TConcatenation(const INode* left, const INode* right)
            : IOperation(left, right)
        {
        }

        virtual inline TOperationType::TType GetOperationType() const
        {
            return TOperationType::Concatenation;
        }
    };

    struct TAlternation: IOperation
    {
        inline TAlternation(const INode* left, const INode* right)
            : IOperation(left, right)
        {
        }

        virtual inline TOperationType::TType GetOperationType() const
        {
            return TOperationType::Alternation;
        }
    };

    struct TClosure: IOperation
    {
        inline TClosure(const INode* node)
            : IOperation(node)
        {
        }

        virtual inline TOperationType::TType GetOperationType() const
        {
            return TOperationType::Closure;
        }
    };

    struct TTokenType
    {
        enum TType
        {
            Character,
            Empty
        };
    };

    struct IToken: INode
    {
        virtual inline TNodeType::TType GetNodeType() const
        {
            return TNodeType::Token;
        }

        virtual TTokenType::TType GetTokenType() const = 0;
    };

    // TODO: use NULL instead
    struct TEmpty: IToken
    {
        virtual inline TTokenType::TType GetTokenType() const
        {
            return TTokenType::Empty;
        }
    };

    template <class TChar>
    struct TCharacter: IToken
    {
        const TChar Character;

        inline TCharacter(TChar character)
            : Character(character)
        {
        }

        virtual inline TTokenType::TType GetTokenType() const
        {
            return TTokenType::Character;
        }
    };

    template <class TChar>
    inline TCharacter<TChar>* MakeCharacter(TChar character)
    {
        return new TCharacter<TChar>(character);
    }

    struct TSymbolType
    {
        enum TType {
            EOL,
            Asterisk,
            Pipe,
            OpeningBracket,
            ClosingBracket,
            Character,
            EscapedCharacter
        };
    };

    template <class TReverseIterator>
    inline TSymbolType::TType GetSymbolType(TReverseIterator rbegin,
        TReverseIterator rend)
    {
        if (rbegin == rend)
        {
            return TSymbolType::EOL;
        }
        else
        {
            TReverseIterator next(rbegin);
            if (++next != rend)
            {
                if (*next == '\\')
                {
                    return TSymbolType::EscapedCharacter;
                }
            }
            switch(*rbegin)
            {
                case '*':
                    return TSymbolType::Asterisk;

                case '|':
                    return TSymbolType::Pipe;

                case '(':
                    return TSymbolType::OpeningBracket;

                case ')':
                    return TSymbolType::ClosingBracket;

                case '\\':
                    throw std::logic_error("unterminated slash character");

                default:
                    return TSymbolType::Character;
            }
        }
    }

    // begin should point to the closing bracket
    // return value will point to the opening bracket
    template <class TReverseIterator>
    TReverseIterator FindOpeningBracket(TReverseIterator rbegin,
        TReverseIterator rend)
    {
        unsigned depth = 1;
        for(TSymbolType::TType type = GetSymbolType(++rbegin, rend);
            type != TSymbolType::EOL && depth;
            type = GetSymbolType(++rbegin, rend))
        {
            switch(type)
            {
                case TSymbolType::OpeningBracket:
                    --depth;
                    break;

                case TSymbolType::ClosingBracket:
                    ++depth;
                    break;

                default:
                    break;
            }
        }

        if(depth)
        {
            throw std::logic_error("no suitable closing bracket found");
        }
        return rbegin;
    }

    template <class TReverseIterator>
    const INode* ParseNode(TReverseIterator rbegin, TReverseIterator rend);

    template <class TReverseIterator>
    inline const INode* ParseLeftNode(TReverseIterator rbegin,
        TReverseIterator rend, TNodePtr right)
    {
        switch(GetSymbolType(rbegin, rend))
        {
            case TSymbolType::EOL:
            case TSymbolType::OpeningBracket:
                return right.Release();

            case TSymbolType::Asterisk:
            {
                TNodePtr left(ParseNode(++rbegin, rend));
                return new TConcatenation(new TClosure(left.Release()),
                    right.Release());
            }

            case TSymbolType::Pipe:
            {
                TNodePtr left(ParseNode(++rbegin, rend));
                return new TAlternation(left.Release(), right.Release());
            }

            case TSymbolType::ClosingBracket:
            case TSymbolType::Character:
            case TSymbolType::EscapedCharacter:
            {
                TNodePtr left(ParseNode(rbegin, rend));
                return new TConcatenation(left.Release(), right.Release());
            }

            default:
                throw std::logic_error("unknown symbol type");
        }
    }

    // TODO: provide ability to pass custom allocator
    template <class TReverseIterator>
    const INode* ParseNode(TReverseIterator rbegin, TReverseIterator rend)
    {
        switch(GetSymbolType(rbegin, rend))
        {
            case TSymbolType::EOL:
            case TSymbolType::OpeningBracket:
                return new TEmpty;

            case TSymbolType::Asterisk:
                return new TClosure(ParseNode(++rbegin, rend));

            case TSymbolType::Pipe:
            {
                TNodePtr empty(new TEmpty);
                return ParseLeftNode(rbegin, rend, empty);
            }

            case TSymbolType::ClosingBracket:
            {
                TReverseIterator pos = FindOpeningBracket(rbegin, rend);
                TNodePtr right(ParseNode(++rbegin, pos));
                if (pos == rend)
                {
                    return right.Release();
                }
                else
                {
                    return ParseLeftNode(pos, rend, right);
                }
            }

            case TSymbolType::Character:
            {
                TNodePtr right(MakeCharacter(*rbegin));
                return ParseLeftNode(++rbegin, rend, right);
            }

            case TSymbolType::EscapedCharacter:
            {
                TNodePtr right(MakeCharacter(*rbegin));
                return ParseLeftNode(++++rbegin, rend, right);
            }

            default:
                throw std::logic_error("unknown symbol type");
        }
    }

    template <class TBidirectionalIterator>
    inline std::reverse_iterator<TBidirectionalIterator> MakeReverseIterator(
        TBidirectionalIterator iterator)
    {
        return std::reverse_iterator<TBidirectionalIterator>(iterator);
    }

    template <class TBidirectionalIterator>
    inline const INode* Parse(TBidirectionalIterator begin,
        TBidirectionalIterator end)
    {
        return ParseNode(MakeReverseIterator(end),
            MakeReverseIterator(begin));
    }
}

#endif

