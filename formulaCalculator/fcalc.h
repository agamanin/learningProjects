#ifndef _FCALC_H_
#define _FCALC_H_

#include <vector>
#include <stack>
#include <string>
#include <memory>
#include <cassert>

namespace fc
{

enum OpType
{
	otUndefined = -1,
	otGet = 0,
	otAdd = 1,
	otSub = 2,
	otMul = 3,
	otDiv = 4,
	otRem = 5
};

struct RawToken
{
	enum TokenType
	{
		ttUndefined = -1,
		ttNumVal = 0,
		ttAdd = 1,
		ttSub = 2,
		ttMul = 3,
		ttDiv = 4,
		ttLeftBrace = 5,
		ttRightBrace = 6,
		ttRem = 7
	};

	const TokenType m_type;
	const double m_val;
	RawToken(const TokenType type, const double val = 0.0)
	: m_type(type),
	  m_val(val)
	{}
};

class IOperation;
using IOperationPtr = std::shared_ptr<IOperation>;

class IOperation
{
public:
	virtual ~IOperation() = default;
	IOperation(const OpType type)
	: m_type(type)
	{}

	virtual double calc() const = 0;
	const OpType getType() const { return m_type; }
    virtual bool isTwoArgOp() const { return false; }
    virtual const size_t getPriority() const = 0;

    static IOperationPtr buildOpWithoutAgrs(const RawToken::TokenType type);

private:
	const OpType m_type;
};

class GetValueOp: public IOperation
{
public:
	GetValueOp() = delete;
	GetValueOp(const double val)
	: IOperation(otGet),
	  m_val(val) {}

	double calc() const override
	{
		return m_val;
	}

	const size_t getPriority() const override { return 0; }

private:
	const double m_val = 0;
};

class TwoArgOp: public IOperation
{
public:
	TwoArgOp() = delete;
	TwoArgOp(const OpType type)
	: IOperation(type)
	{}

	void setLeftArg(IOperationPtr&& arg)
	{
		m_leftArg = std::move(arg);
		assert(m_leftArg);
	}

	void setRightArg(IOperationPtr&& arg)
	{
		m_rightArg = std::move(arg);
		assert(m_rightArg);
	}

	bool isTwoArgOp() const override { return true; }

protected:
	IOperationPtr m_leftArg;
	IOperationPtr m_rightArg;
};

class AddOp: public TwoArgOp
{
public:
	AddOp()
	: TwoArgOp(otAdd)
	{}

	double calc() const override
	{
		return m_leftArg->calc() + m_rightArg->calc();
	}

	const size_t getPriority() const override { return 0; }
};

class SubOp: public TwoArgOp
{
public:
	SubOp()
	: TwoArgOp(otSub)
	{}

	double calc() const override
	{
		return m_leftArg->calc() - m_rightArg->calc();
	}

	const size_t getPriority() const override { return 0; }
};

class MulOp: public TwoArgOp
{
public:
	MulOp()
	: TwoArgOp(otMul)
	{}

	double calc() const override
	{
		return m_leftArg->calc() * m_rightArg->calc();
	}

	const size_t getPriority() const override { return 1; }
};

class DivOp: public TwoArgOp
{
public:
	DivOp()
	: TwoArgOp(otDiv)
	{}

	double calc() const override
	{
		return m_leftArg->calc() / m_rightArg->calc();
	}

	const size_t getPriority() const override { return 1; }
};

class RemOp: public TwoArgOp
{
public:
	RemOp()
	: TwoArgOp(otRem)
	{}

	double calc() const override
	{
		return (long)m_leftArg->calc() % (long)m_rightArg->calc();
	}

	const size_t getPriority() const override { return 1; }
};

class FormulaBuilder
{
public:
	static IOperationPtr buildExpression(const std::string& inputFormula);

private:
	using TokensChain = std::vector<RawToken>;
	using OperationsStack = std::stack<IOperationPtr>;
	static TokensChain parseFormula(const std::string& inputFormula);
	static IOperationPtr buildAST(const TokensChain& tkChain, size_t& currPos, OperationsStack& opStack);
	static IOperationPtr collapseStack(OperationsStack& opStack, const size_t lastOpPriority = 0);
};

} // end namespace fc

#endif