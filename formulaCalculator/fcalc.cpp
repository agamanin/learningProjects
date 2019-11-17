#include <iostream>
#include <sstream>
#include "fcalc.h"

namespace fc
{
	IOperationPtr IOperation::buildOpWithoutAgrs(const RawToken::TokenType type)
	{
		IOperationPtr res;
		switch (type)
		{
			case RawToken::ttAdd:
			{
				res = std::make_shared<AddOp>();
				break;
			}
			case RawToken::ttSub:
			{
				res = std::make_shared<SubOp>();
				break;
			}
			case RawToken::ttMul:
			{
				res = std::make_shared<MulOp>();
				break;
			}
			case RawToken::ttDiv:
			{
				res = std::make_shared<DivOp>();
				break;
			}
			case RawToken::ttRem:
			{
				res = std::make_shared<RemOp>();
				break;
			}
			default:
			{
				assert(!"Unsupported operation type");
				break;
			}
		}

		return res;
	}

	IOperationPtr FormulaBuilder::buildExpression(const std::string& inputFormula)
	{
		auto tkChain = parseFormula(inputFormula);
		if (tkChain.empty())
		{
			std::cout << "Failed to tokenize string" << std::endl;
			return nullptr;
		}

		OperationsStack opStack;
		size_t pos = 0;
		auto res = buildAST(tkChain, pos, opStack);
		if (res && opStack.size() == 1)
		{
			return res;
		}
		return nullptr;
	}

	FormulaBuilder::TokensChain FormulaBuilder::parseFormula(const std::string& inputFormula)
	{
        TokensChain result;
        if (inputFormula.length() > 1024 * 1024 * 1024)
		{
			return result;
		}

        result.reserve(inputFormula.length());
	    std::string opStr;
        std::stringstream ss;
        ss << inputFormula;
        size_t nOpeningBraces = 0;
        size_t nClosingBraces = 0;

        try
        {
        	size_t dParsePos = 0;
	        while (std::getline(ss, opStr, ' '))
	        {
	        	// std::cout << "Next op: " << opStr << std::endl;
	        	if (opStr == "+")
	        	{
	                result.emplace_back(RawToken::ttAdd);
	        	}
	        	else if (opStr == "-")
	        	{
	                result.emplace_back(RawToken::ttSub);
	        	}
	        	else if (opStr == "*")
	        	{
	                result.emplace_back(RawToken::ttMul);
	        	}
	        	else if (opStr == "/")
	        	{
	                result.emplace_back(RawToken::ttDiv);
	        	}
	        	else if (opStr == "%")
	        	{
	                result.emplace_back(RawToken::ttRem);
	        	}
	        	else if (opStr == "(")
	        	{
	        		++nOpeningBraces;
	                result.emplace_back(RawToken::ttLeftBrace);
	        	}
	        	else if (opStr == ")")
	        	{
	        		++nClosingBraces;
	                result.emplace_back(RawToken::ttRightBrace);
	        	}
	        	else
	        	{
	        		auto val = std::stod(opStr, &dParsePos);
	        		if (dParsePos != opStr.length())
	        		{
	        			throw std::logic_error("Failed to parse double value");
	        		}
	        		result.emplace_back(RawToken::ttNumVal, val);
	        	}
	        }
	    }
	    catch (const std::exception& ex)
	    {
	    	std::cout << "Exception while tokenizing: " << ex.what() << std::endl;
	    	result.clear();
	    }
	    catch (...)
	    {
	    	result.clear();
	    }

	    if (nOpeningBraces != nClosingBraces)
	    {
	    	result.clear();
	    	std::cout << "Unmatched braces found" << std::endl;
	    }

        return result;
	}

	IOperationPtr FormulaBuilder::buildAST(const FormulaBuilder::TokensChain& tkChain, size_t& currPos,
		                                            FormulaBuilder::OperationsStack& opStack)
	{
		std::cout << "Entering buildAST\n";

		IOperation* prevOperation = nullptr;
		bool endOfSubExprFound = false;
		while (currPos < tkChain.size() && !endOfSubExprFound)
		{
			const auto& currToken = tkChain[currPos];

			std::cout << "Current pos: " << currPos << ", token type: " << currToken.m_type << ", stack size: " << opStack.size() << std::endl;

			switch (currToken.m_type)
			{
				case RawToken::ttNumVal:
				{
					opStack.emplace(IOperationPtr(new GetValueOp(currToken.m_val)));
					break;
				}
				case RawToken::ttAdd:
				case RawToken::ttSub:
				case RawToken::ttMul:
				case RawToken::ttDiv:
				case RawToken::ttRem:
				{
					auto newOp = IOperation::buildOpWithoutAgrs(currToken.m_type);
					assert(newOp);
					if (prevOperation && prevOperation->getPriority() >= newOp->getPriority())
					{
						auto collapseRes = collapseStack(opStack, newOp->getPriority());
						if (!collapseRes)
						{
							return nullptr;
						}
						prevOperation = nullptr;
					}
					opStack.emplace(std::move(newOp));
					prevOperation = opStack.top().get();
					break;
				}
				case RawToken::ttLeftBrace:
				{
					// std::cout << "Opening brace detected " << currToken.m_type << std::endl;
					OperationsStack subExprStack;
					auto subexpr = buildAST(tkChain, ++currPos, subExprStack);
					if (subexpr)
					{
						std::cout << "Adding collapsed sub-expression: " << subexpr->calc() << std::endl;
						opStack.emplace(std::move(subexpr));
					}
					else
					{
						return nullptr;
					}
					break;
				}
				case RawToken::ttRightBrace:
				{
					// std::cout << "Closing brace detected " << currToken.m_type << std::endl;
					endOfSubExprFound = true;
					break;
				}
				default:
				{
					std::cout << "Unsupported token type " << currToken.m_type << std::endl;
					return nullptr;
				}
			}

			if (endOfSubExprFound)
			{
				break;
			}
			++currPos;
		}

        auto exprResult = collapseStack(opStack);
		if (!exprResult)
		{
			std::cout << "Failed to collapse stack for expression\n";
		}

		return exprResult;
	}

	IOperationPtr FormulaBuilder::collapseStack(OperationsStack& opStack, const size_t lastOpPriority)
	{
		std::cout << "Collapsing stack of size " << opStack.size() << std::endl;
		// Try to unwind the stack:
		if (opStack.size() < 3 && opStack.size() != 1)
		{
			std::cout << "Unexpected stack size while collapsing: " << opStack.size() << std::endl;
			return nullptr;
		}

		if (opStack.size() == 1)
		{
			std::cout << "Result after collapsing: " << opStack.top()->calc() << std::endl;
			return opStack.top();
		}

		auto rArg = std::move(opStack.top());
		opStack.pop();
		auto op = std::move(opStack.top());
		opStack.pop();
		auto lArg = std::move(opStack.top());
		opStack.pop();
		if (!rArg || !op || !rArg)
		{
			std::cout << "Invalid operations found on stack" << std::endl;
			return nullptr;
		}

		assert(op->isTwoArgOp());
		if (op->getPriority() < lastOpPriority)
		{
			std::cout << "Stopping stack collapsing due to low priority operations found" << std::endl;
			opStack.emplace(std::move(lArg));
			opStack.emplace(std::move(op));
			opStack.emplace(std::move(rArg));
			std::cout << "Result after collapsing: " << opStack.top()->calc() << std::endl;
			return opStack.top();
		}

		auto& twoArgOp = static_cast<TwoArgOp&>(*op);
		twoArgOp.setLeftArg(std::move(lArg));
		twoArgOp.setRightArg(std::move(rArg));

        opStack.emplace(op);
		return collapseStack(opStack, lastOpPriority);
	}
} // end ns fc

int main(int argc, char* argv[])
{
    std::string input;
    while (true)
    {
    	std::cout << "Please put the expression:" << std::endl;
		std::getline(std::cin, input);
		if (input == "bye")
		{
			std::cout << "BYE" << std::endl;
			break;
		}

		std::cout << "Calculating..." << std::endl;

		auto expr = fc::FormulaBuilder::buildExpression(input);
		if (!expr)
		{
			std::cout << "Failed to calculate the expression" << std::endl;
		}
		else
		{
			std::cout << "\nResult: " << expr->calc() << std::endl;
		}
		std::cout << std::endl;
	}

	return 0;
}