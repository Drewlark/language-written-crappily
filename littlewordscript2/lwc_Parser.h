#include <iostream>
#include <string>
#include <queue>
#include <stack>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <deque>
#include <iterator>
#include <tuple>
#include <deque>
#include <memory>
#include "lwc_typedefs.h"
#include "lwc_builtins.h"
#ifndef H_PARSER
#define H_PARSER

namespace lwc {
	struct OperatorIdentity
	{
		lwc::builtin_func fnc;
		bool leftassoc = false;
		uint8_t precedence = 0;
		OperatorIdentity(lwc::builtin_func _fnc, bool _leftassoc, uint8_t _precedence) : fnc(_fnc), leftassoc(_leftassoc), precedence(_precedence) {};
	};

	std::unordered_map<std::string, OperatorIdentity> op_ids = {
		{"+", OperatorIdentity(lwc::add, 0, 0)},
		{"*", OperatorIdentity(lwc::mult, 0, 1)},
		{"-", OperatorIdentity(lwc::sub, 1, 0)},
		{"/", OperatorIdentity(lwc::div, 1, 1)}
	};

	bool is_num(const std::string& s)
	{
		for (char c : s) { if (!isdigit(c)) return false; }
	}

	enum class TokenType { num, op, name, lparen, rparen, func, comma };

	struct LAST;

	struct LAST_Variable;

	class ParseToken
	{
	public:
		std::string val;
		TokenType tt;
		int precedence; //meaningless if tt is not TokenType::op
		bool leftassoc = false;
		bool brace_start = false;
		bool brace_end = false;
		builtin_func opfunc = nullptr;
		ParseToken(std::string _val, TokenType _tt, int _precedence = 0, bool _leftassoc = 0) : val(_val), tt(_tt), precedence(_precedence), leftassoc(_leftassoc) {};
		ParseToken(OperatorIdentity oid) : tt(TokenType::op), precedence(oid.precedence), leftassoc(oid.leftassoc), opfunc(oid.fnc) {}; //No value is possible here because the string value is irrelevant to an op
		bool operator<(const ParseToken& pt) { return precedence < pt.precedence; }
		bool operator>(const ParseToken& pt) { return precedence > pt.precedence; }
		bool operator>=(const ParseToken& pt) { return precedence >= pt.precedence; }
		bool operator<=(const ParseToken& pt) { return precedence <= pt.precedence; }
	};

	class BaseFunction {
		bool is_builtin = true;
	public:
		BaseFunction(std::unordered_map<std::string, lwc::variable>& varmap, std::string name) {
			if (varmap.count(name) > 0) {
				is_builtin = false;
			}

		}
	};

	class TokenQueue { //TokenQueue is an object which represents the end result of a lexed line. The constructor is LWC's lexer
		static enum class QState { def, op, num };
		std::deque<lwc::ParseToken> data;
		bool brace_end = false;
		void add_unknown(std::string &unk, QState &qs) {
			if (unk.length() > 0) {
				if (qs == QState::num)
					data.push_back(ParseToken(unk, TokenType::num));
				else
					data.push_back(ParseToken(unk, TokenType::name));
				qs = QState::def;
				unk.clear();
			}
		}

		bool checkparens(std::string &tmp, char &c, QState &qs) {
			bool ret = false;
			if (c == ')')
			{
				add_unknown(tmp, qs);
				data.emplace_back(std::string(1, c), TokenType::rparen);
				ret = true;
			}
			else if (c == '(') {
				if (!tmp.empty()) {
					data.push_back(ParseToken(tmp, TokenType::func));
					tmp.clear();
				}
				data.emplace_back(std::string(1, c), TokenType::lparen);
				ret = true;
			}
			if (ret) qs = QState::def;
			return ret;
		}

	public:
		
		TokenQueue(std::string s) {
			std::string temp = "";
			QState qs = QState::def;
			for (char c : s) {
				if (isdigit(c)) {
					if (temp.size() > 0) {
						if (op_ids.count(temp) > 0) {
							data.emplace_back(op_ids.at(temp));
							temp = c;
							qs = QState::num;
						}
						else {
							temp += c;
						}
					}
					else {
						temp += c;
						qs = QState::num;
					}
				}
				else if (op_ids.count(temp) && !op_ids.count(temp + c)) {
					data.emplace_back(op_ids.at(temp));
					temp.clear();
					if (!checkparens(temp, c, qs)) {
						temp = c;
						qs = QState::def;
					}
				}
				else if (op_ids.count(std::string(1, c))) {
					add_unknown(temp, qs);
					temp = c;
					qs = QState::op;
				}
				else if (c == ',') {
					add_unknown(temp, qs);
					data.emplace_back(",", TokenType::comma);
					qs = QState::def;
				}
				else if (c == ' ') {
					continue;
				}
				else if (c == '{') {
					if (!data.empty() && data.back().tt == TokenType::rparen)
						data.back().brace_start = true;
				}
				else if (c == '}') {
					if (temp.size() > 0) {
						add_unknown(temp, qs);
						data.back().brace_end = true;
					}
					brace_end = true;
					qs = QState::def;
				}
				else
				{
					if(!checkparens(temp, c, qs))
					temp += c;
				}
			}
			add_unknown(temp, qs);
		}

		ParseToken pop()
		{
			ParseToken temp = data.front();
			data.pop_front();
			return temp;
		}

		bool empty() { return data.empty(); }
	};

	std::queue<ParseToken> shunting_yard(TokenQueue tq) //Actual Shunting-Yard algorithm. output is the output queue
	{
		std::queue<ParseToken> out_q; //output queue
		std::stack<ParseToken> op_stk; //operator stack

		while (!tq.empty()) {
			ParseToken pt = tq.pop();
			switch (pt.tt) {

			case TokenType::num:
				out_q.push(pt);
				break;

			case TokenType::name:
				out_q.push(pt);
				break;
			case TokenType::func:
				op_stk.push(pt);
				break;
			case TokenType::op:
				while (!op_stk.empty() && ((op_stk.top().tt == TokenType::op) &&
					(op_stk.top() > pt || (op_stk.top().precedence == pt.precedence && pt.leftassoc)))) {
					out_q.push(op_stk.top());
					op_stk.pop();
				}
				op_stk.push(pt);
				break;
			case TokenType::comma:
				while (op_stk.top().tt != TokenType::lparen) {
					out_q.push(op_stk.top());
					op_stk.pop();
				}
				break;
			case TokenType::lparen:
				op_stk.push(pt);
				break;

			case TokenType::rparen:
				while (op_stk.top().tt != TokenType::lparen) {
					out_q.push(op_stk.top());
					op_stk.pop();
				}
				if (op_stk.top().tt == TokenType::lparen) {
					op_stk.pop();
				}
				if (!op_stk.empty()  && op_stk.top().tt == TokenType::func) {
					out_q.push(op_stk.top());
					op_stk.pop();
				}
			}
		}
		if (!op_stk.empty()) {
			while (!tq.empty()) {
				out_q.push(tq.pop());
			}
		}
		if (tq.empty()) {
			while (!op_stk.empty()) {
				out_q.push(op_stk.top());
				op_stk.pop();
			}
		}
		return out_q;
	}

	struct TokenNode { //Node for our ParseTree
		ParseToken data;
		std::vector<TokenNode*> branches;
		TokenNode(ParseToken _data, std::vector<TokenNode*> _branches = {}) : data(_data), branches(_branches) {}
		~TokenNode() { for (TokenNode* tn : branches) { delete tn; } }
	};


	struct STLine { // test for new version of line
		lwc::builtin_func func;
		std::vector<lwc::variable> varset;
		lwc::variable leaf_var = nullptr;
		STLine(lwc::varset sv, lwc::builtin_func _func): varset(sv), func(_func) {}
	};

	struct LineNode {
		builtin_func func = nullptr;
		std::vector<LineNode*> branches;
		variable var = variable();
		bool is_leaf = false;
		std::vector<LAST> output_block;
		LineNode(builtin_func _func, std::vector<LineNode*> _branches = {}) : func(_func), branches(_branches) {}
		LineNode(variable _var) : var(_var) { is_leaf = true; }
		LineNode() { is_leaf = true; }
	};

	lwc::variable convert_symbol(const std::string& sym, std::unordered_map<std::string, lwc::variable>& varmap) {
		if (is_num(sym)) {
			return new lwc::NumVar(long(stol(sym)));
		}
		else {
			if (varmap.count(sym) == 0) {
				lwc::n_variable temp = new lwc::NumVar(long(0));
				varmap[sym] = temp;
				return temp;
			}
			else {
				return varmap[sym];
			}
		}
	}

	std::unordered_map<std::string, lwc::variable> global; //HACK: eventually pass through global scope
	struct LAST { //"Line" abstract syntax tree
		LineNode* root;
		uint8_t block_ends = 0;
		uint8_t block_starts = 0;
		LineNode* block_node = nullptr;
		LAST(std::queue<ParseToken> tq, std::unordered_map<std::string, lwc::variable>& global) { //Turn a Shunting-Yard output queue into a tree of tokens. This is needed to actually evaluate the expression
			std::stack<LineNode*> pds; //any nodes not yet childed to an operator are pushed here
			while (!tq.empty()) {
				ParseToken pt = ParseToken(tq.front());
				tq.pop();
				if (pt.tt == TokenType::op) { //When we find an operator we must pop n tokens off of pds. n=amount of operands required by given operator or function
					std::vector<LineNode*> temp;
					for (int i = 0; i < 2; ++i) { //Operators always consume two tokens
						temp.push_back(pds.top());
						pds.pop();
					}
					std::reverse(std::begin(temp), std::end(temp)); //vector must be reversed in order for the variables to be in the 'right' order
					pds.push(new LineNode(pt.opfunc, temp)); //create and push operator node with operand children
				}
				else if (pt.tt == TokenType::func) { //Function handling code is currently irrelevant as function declaration is not yet implemented
					std::vector<LineNode*> temp;
					for (int i = 0; i < 3; ++i) { //TODO: let functions work for proper number of parameters
						temp.push_back(pds.top());
						pds.pop();
					}
					LineNode* fln = new LineNode(pt.opfunc, temp);
					if (pt.brace_start) {
						block_node = fln;
						block_starts += 1;
					}
					else if (pt.brace_end) {
						block_ends += 1;
					}
					std::reverse(std::begin(temp), std::end(temp)); //vector must be reversed in order for the variables to be in the 'right' order
					pds.push(fln); //create and push operator node with operand children
				}
				else {
					pds.push(new LineNode(convert_symbol(pt.val, global))); //if not an operator, push to pds
				}
			}
			root = pds.top(); //remaining node is the root
		}

		~LAST() {
			delete root;
		}
	};


	/*double eval_tree(const TokenNode* tn) {
		if (tn->data.tt == TokenType::op) {
			if (mathmap.count(tn->data.val)) {
				std::vector<double> res;
				for (TokenNode* t : tn->branches) {
					res.push_back(eval_tree(t));
				}
				return mathmap[tn->data.val](res);
			}
		}
		else if (tn->data.tt == TokenType::func) {
			std::vector<double> res;
			for (TokenNode* t : tn->branches) {
				res.push_back(eval_tree(t));
			}
			return foobar(res);
		}
		else {
			return atol(tn->data.val.c_str());
		}
	}*/

	lwc::BaseVariable* evaluate_line(lwc::LineNode* node) {
		if (!node->func) {
			std::cout << node->var->get();
			return node->var;
		}
		else {
			vector<BaseVariable*> vars;
			for (LineNode* ln : node->branches) {
				vars.push_back(evaluate_line(ln));
			}
			lwc::variable v(node->func(vars));
			return v;
		}
	}

	lwc::variable evaluate_lines(vector<lwc::LAST> lines) {
		lwc::variable var = new BaseVariable();
		for (LAST line : lines) {
			var = evaluate_line(line.root);
		}
		return var;
	}

	typedef std::vector<LAST> block_func;

	class CodeBlockVariable : public BaseVariable //variable wrapping a vector of type LAST
	{
		block_func code_block;
	public:
		CodeBlockVariable(block_func _code_block) : code_block(_code_block) {}
		long get()
		{
			return evaluate_lines(code_block)->get();
		}
	};


	
	block_func parse_from_slines(std::vector<std::string> slines) {
		std::stack<block_func> blockstack;
		block_func main_scope;
		blockstack.push(main_scope);
		for (std::string sline : slines) {
			TokenQueue tq(sline);
			LAST temp_last(shunting_yard(tq), global);
			blockstack.top().push_back(temp_last);
			LineNode* bnode = temp_last.block_node;
			if (bnode) {
				blockstack.emplace();
			}
			else if (temp_last.block_ends) {
				CodeBlockVariable *cbv = new CodeBlockVariable(blockstack.top());
				blockstack.pop();
				LineNode *ln = new LineNode(cbv);
				bnode->branches.push_back(ln);
			}
		}
		return main_scope;
	}


}



#endif