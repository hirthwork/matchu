#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "nfagenerator.hpp"
#include "token.hpp"
using namespace NReinventedWheels;

unsigned PrintGraph(const INode* root)
{
    static unsigned p = 0;
    unsigned curr = p++;
    std::ostringstream output;
    output << "\tn" << curr << " [label=\"";
    std::vector<unsigned> children;
    if(root->GetNodeType() == TNodeType::Operation)
    {
        const IOperation* op = static_cast<const IOperation*>(root);
        switch(op->GetOperationType())
        {
            case TOperationType::Concatenation:
                output << "Concatenation";
                children.push_back(PrintGraph(op->Children[0]));
                children.push_back(PrintGraph(op->Children[1]));
                break;
            case TOperationType::Alternation:
                output << "Alternation";
                children.push_back(PrintGraph(op->Children[0]));
                children.push_back(PrintGraph(op->Children[1]));
                break;
            case TOperationType::Closure:
                output << "Closure";
                children.push_back(PrintGraph(op->Children[0]));
                break;
        }
    }
    else
    {
        const IToken* token = static_cast<const IToken*>(root);
        if(token->GetTokenType() == TTokenType::Character)
        {
            const TCharacter<char>* c = static_cast<const TCharacter<char>*>(token);
            output << c->Character;
        }
        else
        {
            output << "ε";
        }
    }
    output << "\"];\n";
    for(std::vector<unsigned>::const_iterator iter = children.begin(),
        end = children.end(); iter != end; ++iter)
    {
        output << "\tn" << curr << " -- n" << *iter << ";\n";
    }
    std::cerr << output.str();
    return curr;
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " regexp" << std::endl;
        return 1;
    }
    const INode* root = Parse(argv[1], argv[1] + strlen(argv[1]));
    std::cerr << "graph tree {\n";
    PrintGraph(root);
    std::cerr << "}\n";
    TNFA<char> nfa = TNFAGenerator<char>::CreateNFA(root);
    std::cout << "digraph fsm {\n\trankdir=LR;\n\tnode [shape=doublecircle];";
    for(TNFA<char>::TAcceptStates::const_iterator iter =
        nfa.AcceptStates.begin(), end = nfa.AcceptStates.end(); iter != end;
        ++iter)
    {
        std::cout << " q" << *iter;
    }
    std::cout << ";\n\tnode [shape=circle];\n";
    for(TNFA<char>::TStates::const_iterator state = nfa.States.begin(),
        end = nfa.States.end(); state != end; ++state)
    {
        for(TNFA<char>::TState::const_iterator transition = state->begin(),
            end = state->end(); transition != end; ++transition)
        std::cout << "\tq" << (state - nfa.States.begin()) << " -> q"
            << transition->second << " [ label = \"" << transition->first
            << "\" ];\n";
    }
    std::cout << "}\n";
}

